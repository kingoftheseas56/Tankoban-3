#include "engine/MpvGlWidget.h"
#include "engine/MpvController.h"

#include <QOpenGLContext>
#include <QMetaObject>
#include <QShowEvent>

#include <mpv/client.h>
#include <mpv/render_gl.h>

MpvGlWidget::MpvGlWidget(MpvController* controller, QWidget* parent)
    : QOpenGLWidget(parent), m_controller(controller) {}

void* MpvGlWidget::getProcAddress(void* /*ctx*/, const char* name)
{
    QOpenGLContext* gl = QOpenGLContext::currentContext();
    return gl ? reinterpret_cast<void*>(gl->getProcAddress(name)) : nullptr;
}

void MpvGlWidget::onMpvUpdate(void* ctx)
{
    // Runs on an arbitrary mpv thread. Coalesce, then queue update() onto the GUI
    // thread via a functor (QWidget::update is NOT invokable by name).
    auto* self = static_cast<MpvGlWidget*>(ctx);
    bool expected = false;
    if (self->m_redrawPending.compare_exchange_strong(expected, true))
        QMetaObject::invokeMethod(self, [self]{
            // Suppress GL repaints while the player page is hidden (pre-created before
            // show, or backgrounded after Back). The flag stays pending so showEvent()
            // flushes the held frame when the page is next exposed — without this the
            // pre-warmed widget would render into an unexposed surface.
            if (self->isVisible()) self->update();
        }, Qt::QueuedConnection);
}

void MpvGlWidget::showEvent(QShowEvent* e)
{
    QOpenGLWidget::showEvent(e);
    if (m_redrawPending.load())
        update();   // a redraw arrived while hidden — paint it now that we're exposed
}

void MpvGlWidget::initializeGL()
{
    mpv_opengl_init_params glInit{ &MpvGlWidget::getProcAddress, this };
    mpv_render_param params[] = {
        { MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL) },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    if (mpv_render_context_create(&m_mpvGl, m_controller->handle(), params) < 0)
        qFatal("mpv_render_context_create failed");
    mpv_render_context_set_update_callback(m_mpvGl, &MpvGlWidget::onMpvUpdate, this);
}

void MpvGlWidget::paintGL()
{
    m_redrawPending.store(false);
    if (!m_mpvGl) return;
    const qreal dpr = devicePixelRatioF();
    mpv_opengl_fbo fbo{ static_cast<int>(defaultFramebufferObject()),   // never 0 (§2.2)
                        int(width() * dpr), int(height() * dpr), 0 };
    int flipY = 1;                                                       // Qt FBO is y-flipped vs mpv
    mpv_render_param p[] = {
        { MPV_RENDER_PARAM_OPENGL_FBO, &fbo },
        { MPV_RENDER_PARAM_FLIP_Y, &flipY },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    mpv_render_context_render(m_mpvGl, p);
}

MpvGlWidget::~MpvGlWidget()
{
    // Free the render context on the GL thread while the context is still valid.
    makeCurrent();
    if (m_mpvGl) { mpv_render_context_free(m_mpvGl); m_mpvGl = nullptr; }
    doneCurrent();
}

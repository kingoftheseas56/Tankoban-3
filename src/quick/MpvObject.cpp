#include "MpvObject.h"

#include "QuickPlayerBridge.h"

#include <mpv/render_gl.h>

#include <QByteArray>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

namespace {
void* getProcAddress(void*, const char* name)
{
    QOpenGLContext* gl = QOpenGLContext::currentContext();
    return gl ? reinterpret_cast<void*>(gl->getProcAddress(QByteArray(name))) : nullptr;
}
} // namespace

class MpvRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit MpvRenderer(MpvObject* obj) : m_obj(obj) {}

    ~MpvRenderer() override
    {
        if (m_mpvGl)
            mpv_render_context_free(m_mpvGl);
    }

    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override
    {
        if (!m_mpvGl && m_obj->mpvHandle()) {
            mpv_opengl_init_params glInit{ &getProcAddress, nullptr };
            mpv_render_param params[] = {
                { MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL) },
                { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit },
                { MPV_RENDER_PARAM_INVALID, nullptr }
            };
            if (mpv_render_context_create(&m_mpvGl, m_obj->mpvHandle(), params) >= 0) {
                mpv_render_context_set_update_callback(m_mpvGl, &MpvRenderer::onMpvUpdate, this);
                QMetaObject::invokeMethod(m_obj, "onRenderContextReady", Qt::QueuedConnection);
            }
        }

        return QQuickFramebufferObject::Renderer::createFramebufferObject(size);
    }

    void render() override
    {
        if (!m_mpvGl)
            return;

        QOpenGLFramebufferObject* fbo = framebufferObject();
        mpv_opengl_fbo mpfbo{ static_cast<int>(fbo->handle()), fbo->width(), fbo->height(), 0 };
        int flipY = 0;
        mpv_render_param params[] = {
            { MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo },
            { MPV_RENDER_PARAM_FLIP_Y, &flipY },
            { MPV_RENDER_PARAM_INVALID, nullptr }
        };
        mpv_render_context_render(m_mpvGl, params);
    }

private:
    static void onMpvUpdate(void* ctx)
    {
        auto* self = static_cast<MpvRenderer*>(ctx);
        QMetaObject::invokeMethod(self->m_obj, "doUpdate", Qt::QueuedConnection);
    }

    MpvObject* m_obj = nullptr;
    mpv_render_context* m_mpvGl = nullptr;
};

MpvObject::MpvObject(QQuickItem* parent) : QQuickFramebufferObject(parent)
{
}

MpvObject::~MpvObject() = default;

QQuickFramebufferObject::Renderer* MpvObject::createRenderer() const
{
    return new MpvRenderer(const_cast<MpvObject*>(this));
}

void MpvObject::setPlayer(QuickPlayerBridge* player)
{
    if (m_player == player)
        return;
    m_player = player;
    emit playerChanged();
    update();
}

void MpvObject::doUpdate()
{
    update();
}

void MpvObject::onRenderContextReady()
{
    if (m_renderReady)
        return;
    m_renderReady = true;
    if (m_player)
        m_player->setRenderReady();
}

mpv_handle* MpvObject::mpvHandle() const
{
    return m_player ? m_player->handle() : nullptr;
}

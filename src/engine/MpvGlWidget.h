#pragma once
// MpvGlWidget — a QOpenGLWidget that hosts libmpv's render context
// (MPV_RENDER_API_TYPE_OPENGL). Mirrors Harbor's Mac/Linux render path (spec §2):
// mpv draws video into THIS widget's framebuffer; Qt composites chrome on top.
// No D3D11 interop, no `wid` — Qt owns the GL surface directly.
#include <QOpenGLWidget>
#include <atomic>

struct mpv_render_context;
class MpvController;

class MpvGlWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit MpvGlWidget(MpvController* controller, QWidget* parent = nullptr);
    ~MpvGlWidget() override;

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    static void* getProcAddress(void* ctx, const char* name);
    static void  onMpvUpdate(void* ctx);   // mpv thread -> coalesced GUI repaint

    MpvController* m_controller;
    mpv_render_context* m_mpvGl = nullptr;
    std::atomic<bool> m_redrawPending{false};
};

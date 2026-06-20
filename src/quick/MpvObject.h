#pragma once

#include "QuickPlayerBridge.h"

#include <QtQuick/QQuickFramebufferObject>
#include <QtQmlIntegration/qqmlintegration.h>

struct mpv_handle;

class MpvObject : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QuickPlayerBridge* player READ player WRITE setPlayer NOTIFY playerChanged)

public:
    explicit MpvObject(QQuickItem* parent = nullptr);
    ~MpvObject() override;

    Renderer* createRenderer() const override;

    QuickPlayerBridge* player() const { return m_player; }
    void setPlayer(QuickPlayerBridge* player);

signals:
    void playerChanged();

public slots:
    void doUpdate();
    void onRenderContextReady();

private:
    friend class MpvRenderer;
    mpv_handle* mpvHandle() const;

    QuickPlayerBridge* m_player = nullptr;
    bool m_renderReady = false;
};

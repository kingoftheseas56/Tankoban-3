// Tankoban 3 — VideoPlayerPage. See VideoPlayerPage.h.

#include "ui/VideoPlayerPage.h"

#include "engine/MpvController.h"
#include "player/PlayerView.h"

#include <QResizeEvent>

namespace tankoban {

VideoPlayerPage::VideoPlayerPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("VideoPlayerPage"));
    setStyleSheet(QStringLiteral("background:#000;"));

    m_player = new PlayerView(this);
    m_player->setGeometry(rect());
    // PlayerView is back-safe now (see PlayerView::escapePressed / backRequested):
    // it asks to go back instead of closing the top-level window. Forward that up.
    connect(m_player, &PlayerView::backRequested, this, &VideoPlayerPage::backRequested);
}

void VideoPlayerPage::play(const PlayerRequest& request)
{
    m_player->setTitleInfo(request.title, request.subtitle);
    m_player->play(request.url, request.startSec);
}

void VideoPlayerPage::stop()
{
    if (m_player && m_player->controller())
        m_player->controller()->pause();
}

void VideoPlayerPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_player)
        m_player->setGeometry(rect());
}

} // namespace tankoban

// Tankoban 3 — PosterCard (Step 3). See PosterCard.h.

#include "ui/PosterCard.h"

#include <QEnterEvent>
#include <QImage>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPointer>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace tankoban {

namespace {
// App-lifetime shared manager for cover images.
QNetworkAccessManager* imageNam()
{
    static QNetworkAccessManager* nam = new QNetworkAccessManager();
    return nam;
}
} // namespace

PosterCard::PosterCard(const MetaItem& item, QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("PosterCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(kPosterW);
    setCursor(Qt::PointingHandCursor);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(8);

    m_poster = new QLabel(this);
    m_poster->setObjectName(QStringLiteral("Poster"));
    m_poster->setFixedSize(kPosterW, kPosterH);
    m_poster->setAlignment(Qt::AlignCenter);
    col->addWidget(m_poster);

    m_title = new QLabel(item.name, this);
    m_title->setObjectName(QStringLiteral("PosterTitle"));
    m_title->setFixedWidth(kPosterW);
    m_title->setWordWrap(false);
    col->addWidget(m_title);

    loadPoster(item.poster);
}

void PosterCard::loadPoster(const QString& url)
{
    if (url.isEmpty())
        return;
    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));

    QNetworkReply* reply = imageNam()->get(req);
    QPointer<PosterCard> self(this);
    connect(reply, &QNetworkReply::finished, reply, [self, reply]() {
        reply->deleteLater();
        if (!self)
            return;
        if (reply->error() != QNetworkReply::NoError)
            return;
        QImage img;
        if (!img.loadFromData(reply->readAll()))
            return;

        // FULL-QUALITY, DPR-aware: scale to physical pixels, smooth, set DPR.
        const qreal dpr = self->devicePixelRatioF();
        const QSize physical(int(kPosterW * dpr), int(kPosterH * dpr));
        QImage scaled = img.scaled(physical, Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);
        // center-crop to the exact target box
        const int x = qMax(0, (scaled.width() - physical.width()) / 2);
        const int y = qMax(0, (scaled.height() - physical.height()) / 2);
        scaled = scaled.copy(x, y, qMin(physical.width(), scaled.width()),
                             qMin(physical.height(), scaled.height()));

        QPixmap pm = QPixmap::fromImage(scaled);
        pm.setDevicePixelRatio(dpr);
        self->m_poster->setPixmap(pm);
    });
}

void PosterCard::enterEvent(QEnterEvent*)
{
    setProperty("hover", true);
    style()->unpolish(this);
    style()->polish(this);
}

void PosterCard::leaveEvent(QEvent*)
{
    setProperty("hover", false);
    style()->unpolish(this);
    style()->polish(this);
}

} // namespace tankoban

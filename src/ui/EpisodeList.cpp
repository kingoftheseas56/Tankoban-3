// Tankoban 3 — EpisodeList (Detail path). See EpisodeList.h.

#include "ui/EpisodeList.h"

#include <algorithm>
#include <functional>

#include <QAction>
#include <QDate>
#include <QDateTime>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

namespace tankoban {

namespace {
constexpr int kThumbW = 200;
constexpr int kThumbH = 112; // ~16:9

QNetworkAccessManager* thumbNam()
{
    static QNetworkAccessManager* nam = [] {
        auto* m = new QNetworkAccessManager();
        auto* cache = new QNetworkDiskCache(m);
        cache->setCacheDirectory(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/thumb-cache"));
        cache->setMaximumCacheSize(qint64(96) * 1024 * 1024);
        m->setCache(cache);
        return m;
    }();
    return nam;
}

QString formatAired(const QString& iso)
{
    if (iso.isEmpty())
        return {};
    QDate d = QDateTime::fromString(iso, Qt::ISODate).date();
    if (!d.isValid())
        d = QDate::fromString(iso.left(10), QStringLiteral("yyyy-MM-dd"));
    return d.isValid() ? d.toString(QStringLiteral("MMM d, yyyy")) : QString();
}

// A clickable episode row (plain QWidget; EpisodeList owns the signal). Hover is QSS.
class EpisodeRow : public QWidget {
public:
    EpisodeRow(const EpisodeItem& ep, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("EpisodeRow"));
        setAttribute(Qt::WA_StyledBackground, true);
        setCursor(Qt::PointingHandCursor);

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(12, 12, 12, 12);
        row->setSpacing(18);

        m_thumb = new QLabel(this);
        m_thumb->setObjectName(QStringLiteral("EpThumb"));
        m_thumb->setFixedSize(kThumbW, kThumbH);
        row->addWidget(m_thumb);

        auto* textCol = new QVBoxLayout();
        textCol->setContentsMargins(0, 0, 0, 0);
        textCol->setSpacing(6);
        const int epNo = ep.episode > 0 ? ep.episode : 1;
        QString title = ep.name;
        if (title.isEmpty())
            title = QObject::tr("Episode %1").arg(epNo);
        auto* titleLabel = new QLabel(title, this);
        titleLabel->setObjectName(QStringLiteral("EpTitle"));
        titleLabel->setWordWrap(false);
        QStringList metaParts;
        if (ep.season > 0)
            metaParts << QStringLiteral("S%1 E%2").arg(ep.season).arg(epNo);
        const QString aired = formatAired(ep.released);
        if (!aired.isEmpty())
            metaParts << aired;
        auto* metaLabel = new QLabel(metaParts.join(QStringLiteral("   ·   ")), this);
        metaLabel->setObjectName(QStringLiteral("EpMeta"));
        textCol->addStretch();
        textCol->addWidget(titleLabel);
        textCol->addWidget(metaLabel);
        textCol->addStretch();
        row->addLayout(textCol, 1);

        if (!ep.thumbnail.isEmpty())
            loadThumb(ep.thumbnail);
    }

    std::function<void()> onClick;

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton && onClick)
            onClick();
    }

private:
    void loadThumb(const QString& url)
    {
        QNetworkRequest req((QUrl(url)));
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Tankoban3/0.1"));
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::PreferCache);
        QNetworkReply* reply = thumbNam()->get(req);
        QPointer<EpisodeRow> self(this);
        connect(reply, &QNetworkReply::finished, reply, [self, reply]() {
            reply->deleteLater();
            if (!self)
                return;
            if (reply->error() != QNetworkReply::NoError)
                return;
            QImage img;
            if (!img.loadFromData(reply->readAll()))
                return;
            const qreal dpr = self->devicePixelRatioF();
            const QSize phys(int(kThumbW * dpr), int(kThumbH * dpr));
            QImage scaled =
                img.scaled(phys, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            const int x = qMax(0, (scaled.width() - phys.width()) / 2);
            const int y = qMax(0, (scaled.height() - phys.height()) / 2);
            scaled = scaled.copy(x, y, qMin(phys.width(), scaled.width()),
                                 qMin(phys.height(), scaled.height()));
            QPixmap pm = QPixmap::fromImage(scaled);
            pm.setDevicePixelRatio(dpr);
            self->m_thumb->setPixmap(pm);
        });
    }

    QLabel* m_thumb = nullptr;
};
} // namespace

EpisodeList::EpisodeList(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("EpisodeList"));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(14);

    auto* headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel(tr("Episodes"), this);
    title->setObjectName(QStringLiteral("DetailSectionTitle"));
    headerRow->addWidget(title);
    headerRow->addStretch();

    // Custom Harbor-style season selector: a pill trigger + a slick dark popup menu.
    m_seasonBtn = new QPushButton(this);
    m_seasonBtn->setObjectName(QStringLiteral("SeasonTrigger"));
    m_seasonBtn->setCursor(Qt::PointingHandCursor);
    m_seasonMenu = new QMenu(this);
    m_seasonMenu->setObjectName(QStringLiteral("SeasonMenu"));
    connect(m_seasonBtn, &QPushButton::clicked, this, [this]() {
        m_seasonMenu->setMinimumWidth(m_seasonBtn->width());
        m_seasonMenu->popup(m_seasonBtn->mapToGlobal(QPoint(0, m_seasonBtn->height() + 6)));
    });
    headerRow->addWidget(m_seasonBtn);
    col->addLayout(headerRow);

    m_count = new QLabel(this);
    m_count->setObjectName(QStringLiteral("DetailEpCount"));
    col->addWidget(m_count);

    m_rows = new QWidget(this);
    m_rows->setAttribute(Qt::WA_TranslucentBackground, true);
    m_rowsLayout = new QVBoxLayout(m_rows);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(4);
    col->addWidget(m_rows);
}

void EpisodeList::setEpisodes(const QVector<EpisodeItem>& videos)
{
    m_bySeason.clear();
    for (const EpisodeItem& e : videos)
        m_bySeason[e.season].push_back(e);
    for (auto it = m_bySeason.begin(); it != m_bySeason.end(); ++it)
        std::sort(it->begin(), it->end(),
                  [](const EpisodeItem& a, const EpisodeItem& b) { return a.episode < b.episode; });

    m_seasonMenu->clear();
    for (auto it = m_bySeason.constBegin(); it != m_bySeason.constEnd(); ++it) {
        const int s = it.key();
        const QString label = s <= 0 ? tr("Specials") : tr("Season %1").arg(s);
        QAction* a = m_seasonMenu->addAction(label);
        a->setCheckable(true);
        a->setData(s);
        connect(a, &QAction::triggered, this, [this, s]() { showSeason(s); });
    }
    m_seasonBtn->setVisible(m_bySeason.size() > 1);

    // Default to the latest real season (>0), else the last key present.
    int def = m_bySeason.isEmpty() ? 1 : m_bySeason.lastKey();
    for (auto it = m_bySeason.constBegin(); it != m_bySeason.constEnd(); ++it)
        if (it.key() > 0)
            def = it.key();
    showSeason(def);
}

void EpisodeList::showSeason(int season)
{
    const QString label = season <= 0 ? tr("Specials") : tr("Season %1").arg(season);
    m_seasonBtn->setText(label + QStringLiteral("   ⌄"));
    for (QAction* a : m_seasonMenu->actions())
        a->setChecked(a->data().toInt() == season);

    while (QLayoutItem* it = m_rowsLayout->takeAt(0)) {
        if (it->widget())
            it->widget()->deleteLater();
        delete it;
    }
    const QVector<EpisodeItem> eps = m_bySeason.value(season);
    m_count->setText(eps.size() == 1 ? tr("%1 episode").arg(eps.size())
                                     : tr("%1 episodes").arg(eps.size()));
    for (const EpisodeItem& ep : eps) {
        auto* row = new EpisodeRow(ep, m_rows);
        const EpisodeItem captured = ep;
        row->onClick = [this, captured]() { emit episodeActivated(captured); };
        m_rowsLayout->addWidget(row);
    }
}

} // namespace tankoban

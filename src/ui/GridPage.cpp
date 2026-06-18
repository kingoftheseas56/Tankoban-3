// Tankoban 3 — GridPage. See GridPage.h.

#include "ui/GridPage.h"

#include "core/JikanClient.h"
#include "ui/BackToTop.h"
#include "ui/FlowLayout.h"
#include "ui/Icons.h"
#include "ui/PosterCard.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>

namespace tankoban {

GridPage::GridPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ContentPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("HomeScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outer->addWidget(m_scroll);

    m_content = new QWidget(m_scroll);
    m_content->setObjectName(QStringLiteral("ContentPage"));
    m_content->setAttribute(Qt::WA_StyledBackground, true);

    auto* col = new QVBoxLayout(m_content);
    col->setContentsMargins(48, 28, 48, 48);
    col->setSpacing(28);

    // Header: round back button + big title + "{N} titles" count (Harbor grid.tsx).
    auto* header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(16);

    auto* back = new QPushButton(m_content);
    back->setObjectName(QStringLiteral("GridBack"));
    back->setCursor(Qt::PointingHandCursor);
    back->setFixedSize(40, 40);
    back->setIcon(navIcon(QStringLiteral("chev-left"), QColor(QStringLiteral("#c9ced6")), 18));
    back->setIconSize(QSize(18, 18));
    back->setStyleSheet(QStringLiteral(
        "#GridBack{border:none;border-radius:20px;background:rgba(255,255,255,0.06);}"
        "#GridBack:hover{background:rgba(255,255,255,0.12);}"));
    connect(back, &QPushButton::clicked, this, &GridPage::backRequested);
    header->addWidget(back);

    m_title = new QLabel(m_content);
    m_title->setObjectName(QStringLiteral("GridTitle"));
    m_title->setStyleSheet(QStringLiteral(
        "#GridTitle{font-size:28px;font-weight:600;color:#f3f1ea;letter-spacing:-0.5px;}"));
    header->addWidget(m_title);

    m_count = new QLabel(m_content);
    m_count->setObjectName(QStringLiteral("GridCount"));
    m_count->setStyleSheet(QStringLiteral("#GridCount{font-size:14px;color:#8b8f99;}"));
    header->addWidget(m_count);
    header->addStretch();
    col->addLayout(header);

    // Responsive poster grid (Harbor minmax(150px) / gap-x 16 / gap-y 32).
    m_grid = new QWidget(m_content);
    m_grid->setObjectName(QStringLiteral("GridTrack"));
    m_flow = new FlowLayout(m_grid, /*margin=*/0, /*hSpacing=*/16, /*vSpacing=*/32);
    col->addWidget(m_grid);

    // Empty state (Harbor grid.tsx: "Nothing here yet." when done and empty).
    m_empty = new QLabel(QStringLiteral("Nothing here yet."), m_content);
    m_empty->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_empty->setStyleSheet(QStringLiteral("color:#8b8f99;font-size:14px;padding:80px 0;"));
    m_empty->hide();
    col->addWidget(m_empty);

    col->addStretch(1);

    m_scroll->setWidget(m_content);

    new BackToTop(m_scroll, this); // Harbor grid.tsx renders <BackToTop>

    // Grid pager (Harbor grid fetcher): its own JikanClient so View-all infinite-scrolls
    // independent of the row; shares the static Jikan cache so loaded pages are instant.
    m_jikan = new JikanClient(this);
    connect(m_jikan, &JikanClient::rowReady, this, &GridPage::onGridPage);
    connect(m_jikan, &JikanClient::rowFailed, this, [this](const QString& k, const QString&) {
        if (k == QLatin1String("__grid__")) {
            m_pageLoading = false;
            m_pageDone = true; // stop paging on error (Harbor catch -> done)
        }
    });

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() { updateVisible(); checkLoadMore(); });
}

void GridPage::setCatalog(const QString& title, const QVector<MetaItem>& items)
{
    setPagedCatalog(title, items, QString(), 0); // no template -> no infinite paging
}

void GridPage::setPagedCatalog(const QString& title, const QVector<MetaItem>& initial,
                               const QString& jikanPathTemplate, int startPage)
{
    m_title->setText(title);
    m_pagePath = jikanPathTemplate;
    m_page = startPage;
    m_pageLoading = false;
    m_pageDone = jikanPathTemplate.isEmpty();

    m_seenIds.clear();
    clearCards();
    appendCards(initial);
    if (m_empty)
        m_empty->setVisible(m_cards.isEmpty() && m_pageDone);

    // New category: jump to the top, then lazy-load the first viewport + maybe page more.
    m_scroll->verticalScrollBar()->setValue(0);
    QTimer::singleShot(0, this, [this]() { updateVisible(); checkLoadMore(); });
}

void GridPage::appendCards(const QVector<MetaItem>& items)
{
    for (const MetaItem& m : items) {
        if (m.id.isEmpty() || m_seenIds.contains(m.id))
            continue;
        m_seenIds.insert(m.id);
        auto* card = new PosterCard(m, m_grid);
        connect(card, &PosterCard::activated, this, &GridPage::openDetailRequested);
        m_flow->addWidget(card);
        m_cards.push_back(card);
    }
    m_count->setText(QString::number(m_cards.size())
                     + (m_cards.size() == 1 ? QStringLiteral(" title") : QStringLiteral(" titles")));
}

void GridPage::clearCards()
{
    QLayoutItem* item = nullptr;
    while ((item = m_flow->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    m_cards.clear();
}

void GridPage::updateVisible()
{
    if (m_cards.isEmpty())
        return;
    const int sy = m_scroll->verticalScrollBar()->value();
    const int vh = m_scroll->viewport()->height();
    const int margin = 600;
    const int lo = sy - margin;
    const int hi = sy + vh + margin;
    for (PosterCard* c : m_cards) {
        const int cy = c->mapTo(m_content, QPoint(0, 0)).y();
        if (cy + c->height() >= lo && cy <= hi)
            c->ensureLoaded();
    }
}

void GridPage::checkLoadMore()
{
    if (m_pageDone || m_pageLoading || m_pagePath.isEmpty() || m_page >= kPageCap)
        return;
    auto* sb = m_scroll->verticalScrollBar();
    // Near the bottom (Harbor IntersectionObserver rootMargin ~900px); also fires when the
    // content is shorter than the viewport, so short categories keep filling toward the cap.
    if (sb->maximum() - sb->value() >= 900)
        return;
    m_pageLoading = true;
    QString path = m_pagePath;
    path.replace(QStringLiteral("page=1"), QStringLiteral("page=") + QString::number(m_page + 1));
    m_jikan->fetchRow(QStringLiteral("__grid__"), path);
}

void GridPage::onGridPage(const QString& key, const QVector<MetaItem>& items)
{
    if (key != QLatin1String("__grid__"))
        return;
    m_pageLoading = false;
    if (items.isEmpty()) {
        m_pageDone = true;
        return;
    }
    const int before = m_cards.size();
    appendCards(items);
    m_page += 1;
    if (m_cards.size() == before || m_page >= kPageCap)
        m_pageDone = true; // no fresh items, or hit the page cap
    QTimer::singleShot(0, this, [this]() { updateVisible(); checkLoadMore(); });
}

void GridPage::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    QTimer::singleShot(0, this, [this]() { updateVisible(); checkLoadMore(); });
}

void GridPage::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    updateVisible();
    checkLoadMore();
}

} // namespace tankoban

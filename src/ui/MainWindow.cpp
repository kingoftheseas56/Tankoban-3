// Tankoban 3 — MainWindow (Step 2 + Detail + Addons). See MainWindow.h.

#include "ui/MainWindow.h"

#include "core/AddonRegistry.h"
#include "core/MetaDetail.h"
#include "ui/AddonsPage.h"
#include "ui/DetailPage.h"
#include "ui/HomePage.h"
#include "ui/Sidebar.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace tankoban {

namespace {
struct ViewDef {
    const char* id;
    const char* title;
};
// Faithful to Harbor's nav, minus the cut IPTV cluster (Live / Playlists / Multiview).
const ViewDef kViews[] = {
    {"home", "Home"},         {"discover", "Discover"}, {"movies", "Movies"},
    {"shows", "Shows"},       {"anime", "Anime"},
    {"library", "Library"},   {"downloads", "Downloads"}, {"addons", "Addons"},
    {"settings", "Settings"},
};
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Root"));
    setWindowTitle(QStringLiteral("Tankoban 3"));
    resize(1280, 800);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_sidebar = new Sidebar(this);
    row->addWidget(m_sidebar);

    m_content = new QStackedWidget(this);
    m_content->setObjectName(QStringLiteral("Content"));
    m_content->setAttribute(Qt::WA_StyledBackground, true); // opaque backstop (no page bleed)
    row->addWidget(m_content, 1);

    m_registry = new AddonRegistry(this);
    m_registry->load();

    for (const auto& v : kViews) {
        const QString id = QString::fromLatin1(v.id);
        QWidget* page = nullptr;
        if (id == QLatin1String("home")) {
            auto* home = new HomePage(this);
            connect(home, &HomePage::openDetailRequested, this, &MainWindow::openDetail);
            page = home;
        } else if (id == QLatin1String("addons")) {
            page = new AddonsPage(m_registry, this);
        } else {
            page = makePlaceholder(QString::fromLatin1(v.title));
        }
        const int idx = m_content->addWidget(page);
        m_pageIndex.insert(id, idx);
    }

    // Detail is a pushed frame over the shell (sidebar stays visible, Harbor-style).
    m_detailPage = new DetailPage(this);
    m_detailIndex = m_content->addWidget(m_detailPage);
    connect(m_detailPage, &DetailPage::backRequested, this,
            [this]() { m_content->setCurrentIndex(m_returnIndex); });
    connect(m_detailPage, &DetailPage::playRequested, this, [](const MetaItem& m) {
        qInfo() << "[detail] Play -> Play Picker (next slice):" << m.name << m.id;
    });
    connect(m_detailPage, &DetailPage::episodeRequested, this,
            [](const MetaItem& m, const EpisodeItem& ep) {
                qInfo() << "[detail] Episode -> Play Picker (next slice):" << m.name
                        << "S" << ep.season << "E" << ep.episode;
            });

    connect(m_sidebar, &Sidebar::viewActivated, this, [this](const QString& id) {
        if (m_pageIndex.contains(id))
            m_content->setCurrentIndex(m_pageIndex.value(id));
        m_sidebar->setActive(id);
    });

    m_content->setCurrentIndex(m_pageIndex.value(QStringLiteral("home")));
}

void MainWindow::openDetail(const MetaItem& meta)
{
    if (!m_detailPage)
        return;
    m_detailPage->load(meta);
    m_returnIndex = m_content->currentIndex();
    m_content->setCurrentIndex(m_detailIndex);
}

QWidget* MainWindow::makePlaceholder(const QString& title)
{
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("ContentPage"));
    page->setAttribute(Qt::WA_StyledBackground, true);

    auto* col = new QVBoxLayout(page);
    col->addStretch();

    auto* heading = new QLabel(title, page);
    heading->setObjectName(QStringLiteral("PlaceholderTitle"));
    heading->setAlignment(Qt::AlignCenter);

    auto* sub = new QLabel(QStringLiteral("COMING SOON — STEP BY STEP"), page);
    sub->setObjectName(QStringLiteral("PlaceholderSub"));
    sub->setAlignment(Qt::AlignCenter);

    col->addWidget(heading);
    col->addSpacing(8);
    col->addWidget(sub);
    col->addStretch();
    return page;
}

} // namespace tankoban

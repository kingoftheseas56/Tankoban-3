// Tankoban 3 — MainWindow (Step 2). See MainWindow.h.

#include "ui/MainWindow.h"

#include "ui/Sidebar.h"

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
    row->addWidget(m_content, 1);

    for (const auto& v : kViews) {
        const int idx = m_content->addWidget(makePlaceholder(QString::fromLatin1(v.title)));
        m_pageIndex.insert(QString::fromLatin1(v.id), idx);
    }

    connect(m_sidebar, &Sidebar::viewActivated, this, [this](const QString& id) {
        if (m_pageIndex.contains(id))
            m_content->setCurrentIndex(m_pageIndex.value(id));
        m_sidebar->setActive(id);
    });

    m_content->setCurrentIndex(m_pageIndex.value(QStringLiteral("home")));
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

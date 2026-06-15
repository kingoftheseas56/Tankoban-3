// Tankoban 3 — Sidebar (Step 2). See Sidebar.h.

#include "ui/Sidebar.h"

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace tankoban {

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Sidebar"));
    setAttribute(Qt::WA_StyledBackground, true); // let the #Sidebar QSS background paint
    setFixedWidth(240);                          // Harbor expanded width (collapse: later)

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 0, 16, 16);
    root->setSpacing(0);

    // Brand wordmark header (Harbor: h-20 / 80px, Fraunces serif).
    auto* brand = new QLabel(QStringLiteral("Tankoban"), this);
    brand->setObjectName(QStringLiteral("Brand"));
    brand->setFixedHeight(80);
    brand->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    root->addWidget(brand);

    // PRIMARY group (Harbor minus the cut IPTV cluster: Live/Playlists).
    auto* primary = new QVBoxLayout();
    primary->setSpacing(6);
    addNavItem(primary, QStringLiteral("home"),     QStringLiteral("Home"));
    addNavItem(primary, QStringLiteral("discover"), QStringLiteral("Discover"));
    addNavItem(primary, QStringLiteral("movies"),   QStringLiteral("Movies"));
    addNavItem(primary, QStringLiteral("shows"),    QStringLiteral("Shows"));
    addNavItem(primary, QStringLiteral("anime"),    QStringLiteral("Anime"));
    root->addLayout(primary);

    // Hairline divider between the two groups.
    root->addSpacing(12);
    auto* divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("NavDivider"));
    divider->setFrameShape(QFrame::NoFrame);
    divider->setFixedHeight(1);
    root->addWidget(divider);
    root->addSpacing(12);

    // COLLECTIONS group.
    auto* collections = new QVBoxLayout();
    collections->setSpacing(6);
    addNavItem(collections, QStringLiteral("library"),   QStringLiteral("Library"));
    addNavItem(collections, QStringLiteral("downloads"), QStringLiteral("Downloads"));
    addNavItem(collections, QStringLiteral("addons"),    QStringLiteral("Addons"));
    addNavItem(collections, QStringLiteral("settings"),  QStringLiteral("Settings"));
    root->addLayout(collections);

    root->addStretch();

    setActive(QStringLiteral("home"));
}

void Sidebar::addNavItem(QVBoxLayout* group, const QString& id, const QString& label)
{
    auto* btn = new QPushButton(label, this);
    btn->setObjectName(QStringLiteral("NavItem"));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedHeight(50);
    btn->setProperty("active", false);
    connect(btn, &QPushButton::clicked, this, [this, id]() { emit viewActivated(id); });
    group->addWidget(btn);
    m_items.insert(id, btn);
}

void Sidebar::setActive(const QString& id)
{
    auto repolish = [](QPushButton* b) {
        b->style()->unpolish(b);
        b->style()->polish(b);
    };
    if (auto* prev = m_items.value(m_active, nullptr)) {
        prev->setProperty("active", false);
        repolish(prev);
    }
    m_active = id;
    if (auto* cur = m_items.value(id, nullptr)) {
        cur->setProperty("active", true);
        repolish(cur);
    }
}

} // namespace tankoban

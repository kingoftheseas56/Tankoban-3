// Tankoban 3 — Sidebar (Step 2 + collapse). See Sidebar.h.

#include "ui/Sidebar.h"

#include "ui/Icons.h"

#include <QColor>
#include <QEasingCurve>
#include <QFrame>
#include <QLabel>
#include <QPointF>
#include <QPushButton>
#include <QSettings>
#include <QSize>
#include <QStyle>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace tankoban {

namespace {
const QColor kIconMuted(QStringLiteral("#aab1bd"));
const QColor kIconActive(QStringLiteral("#f3f1ea"));
const QColor kIconAccent(QStringLiteral("#e8b923")); // collapsed active = gold icon
constexpr int kIconPx = 22;

QSettings appSettings()
{
    return QSettings(QStringLiteral("Tankoban"), QStringLiteral("Tankoban3"));
}

// Harbor's "pull" easing: cubic-bezier(0.32, 0.72, 0.24, 1).
QEasingCurve pullEase()
{
    QEasingCurve c(QEasingCurve::BezierSpline);
    c.addCubicBezierSegment(QPointF(0.32, 0.72), QPointF(0.24, 1.0), QPointF(1.0, 1.0));
    return c;
}
} // namespace

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Sidebar"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 0, 16, 16);
    root->setSpacing(0);

    m_brand = new QLabel(QStringLiteral("Tankoban"), this);
    m_brand->setObjectName(QStringLiteral("Brand"));
    m_brand->setFixedHeight(80);
    m_brand->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    root->addWidget(m_brand);

    // PRIMARY group (Harbor minus the cut IPTV cluster).
    auto* primary = new QVBoxLayout();
    primary->setSpacing(6);
    addNavItem(primary, QStringLiteral("home"),     QStringLiteral("Home"));
    addNavItem(primary, QStringLiteral("discover"), QStringLiteral("Discover"));
    addNavItem(primary, QStringLiteral("movies"),   QStringLiteral("Movies"));
    addNavItem(primary, QStringLiteral("shows"),    QStringLiteral("Shows"));
    addNavItem(primary, QStringLiteral("anime"),    QStringLiteral("Anime"));
    root->addLayout(primary);

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

    // Bottom: hairline + the collapse toggle (Harbor's CollapseToggle).
    auto* footDivider = new QFrame(this);
    footDivider->setObjectName(QStringLiteral("NavDivider"));
    footDivider->setFrameShape(QFrame::NoFrame);
    footDivider->setFixedHeight(1);
    root->addWidget(footDivider);
    root->addSpacing(10);

    m_toggle = new QPushButton(QStringLiteral("Collapse"), this);
    m_toggle->setObjectName(QStringLiteral("CollapseToggle"));
    m_toggle->setCursor(Qt::PointingHandCursor);
    m_toggle->setFixedHeight(36);
    m_toggle->setIcon(navIcon(QStringLiteral("panel-collapse"), kIconMuted, 18));
    m_toggle->setIconSize(QSize(18, 18));
    connect(m_toggle, &QPushButton::clicked, this, [this]() {
        setCollapsed(!m_collapsed, true);
        appSettings().setValue(QStringLiteral("sidebarCollapsed"), m_collapsed);
    });
    root->addWidget(m_toggle);

    // Restore the persisted collapse state (no animation on first paint).
    m_collapsed = appSettings().value(QStringLiteral("sidebarCollapsed"), false).toBool();
    setCollapsed(m_collapsed, false);
    setActive(QStringLiteral("home"));
}

void Sidebar::addNavItem(QVBoxLayout* group, const QString& id, const QString& label)
{
    auto* btn = new QPushButton(label, this);
    btn->setObjectName(QStringLiteral("NavItem"));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedHeight(50);
    btn->setIcon(navIcon(id, kIconMuted, kIconPx));
    btn->setIconSize(QSize(kIconPx, kIconPx));
    btn->setProperty("active", false);
    btn->setProperty("navlabel", label); // restored when expanding
    btn->setToolTip(label);              // shown when collapsed (icon-only)
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
        prev->setIcon(navIcon(m_active, kIconMuted, kIconPx));
        repolish(prev);
    }
    m_active = id;
    if (auto* cur = m_items.value(id, nullptr)) {
        cur->setProperty("active", true);
        // Harbor: collapsed active is a gold icon (no pill); expanded is the elevated pill.
        cur->setIcon(navIcon(id, m_collapsed ? kIconAccent : kIconActive, kIconPx));
        repolish(cur);
    }
}

void Sidebar::applyCollapsedVisual(bool collapsed)
{
    auto repolish = [](QWidget* w) {
        w->style()->unpolish(w);
        w->style()->polish(w);
    };

    m_brand->setText(collapsed ? QStringLiteral("T") : QStringLiteral("Tankoban"));
    m_brand->setAlignment(collapsed ? (Qt::AlignVCenter | Qt::AlignHCenter)
                                    : (Qt::AlignVCenter | Qt::AlignLeft));

    for (QPushButton* btn : m_items) {
        btn->setProperty("collapsed", collapsed);
        btn->setText(collapsed ? QString() : btn->property("navlabel").toString());
        repolish(btn);
    }

    m_toggle->setProperty("collapsed", collapsed);
    m_toggle->setText(collapsed ? QString() : QStringLiteral("Collapse"));
    m_toggle->setIcon(navIcon(collapsed ? QStringLiteral("panel-expand")
                                        : QStringLiteral("panel-collapse"),
                              kIconMuted, 18));
    m_toggle->setToolTip(collapsed ? QStringLiteral("Expand sidebar")
                                   : QStringLiteral("Collapse sidebar"));
    repolish(m_toggle);

    // Refresh the active item's icon tint for the new mode (gold when collapsed).
    setActive(m_active);
}

void Sidebar::setCollapsed(bool collapsed, bool animate)
{
    m_collapsed = collapsed;
    applyCollapsedVisual(collapsed);

    const int target = collapsed ? kCollapsedW : kExpandedW;
    if (animate && isVisible()) {
        auto* a = new QVariantAnimation(this);
        a->setStartValue(width());
        a->setEndValue(target);
        a->setDuration(320); // Harbor duration-[320ms]
        a->setEasingCurve(pullEase());
        connect(a, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
            const int w = v.toInt();
            setMinimumWidth(w);
            setMaximumWidth(w);
        });
        a->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        setMinimumWidth(target);
        setMaximumWidth(target);
    }
}

} // namespace tankoban

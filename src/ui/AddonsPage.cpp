// Tankoban 3 — AddonsPage (Addons store). See AddonsPage.h.

#include "ui/AddonsPage.h"

#include "core/AddonRegistry.h"
#include "core/StremioAddonsProvider.h"
#include "ui/BrowsePane.h"
#include "ui/ToggleSwitch.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace tankoban {

namespace {
QString resourceLabel(const QString& n)
{
    if (n == QLatin1String("stream"))
        return QStringLiteral("Streams");
    if (n == QLatin1String("catalog"))
        return QStringLiteral("Catalogs");
    if (n == QLatin1String("meta"))
        return QStringLiteral("Metadata");
    if (n == QLatin1String("subtitles"))
        return QStringLiteral("Subtitles");
    return n;
}

QString subtitleFor(const Manifest& m)
{
    QStringList parts;
    for (const AddonResource& r : m.resources) {
        const QString l = resourceLabel(r.name);
        if (!parts.contains(l))
            parts << l;
    }
    if (!parts.isEmpty())
        return parts.join(QStringLiteral("  ·  "));
    if (!m.description.isEmpty())
        return m.description.left(80);
    return m.id;
}
} // namespace

AddonsPage::AddonsPage(AddonRegistry* registry, QWidget* parent)
    : QWidget(parent)
    , m_registry(registry)
{
    setObjectName(QStringLiteral("ContentPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    m_provider = new StremioAddonsProvider(this);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(40, 28, 40, 16);
    outer->setSpacing(16);

    // Header: tabs + marketplace search + paste-install + adult toggle.
    auto* hdr = new QHBoxLayout();
    hdr->setSpacing(10);
    const char* labels[3] = {"Discover", "Browse", "Installed"};
    for (int i = 0; i < 3; ++i) {
        auto* b = new QPushButton(tr(labels[i]), this);
        b->setObjectName(QStringLiteral("AddonTab"));
        b->setCursor(Qt::PointingHandCursor);
        b->setCheckable(true);
        connect(b, &QPushButton::clicked, this, [this, i]() { showTab(i); });
        m_tabBtns[i] = b;
        hdr->addWidget(b);
    }
    hdr->addSpacing(16);
    m_search = new QLineEdit(this);
    m_search->setObjectName(QStringLiteral("AddonSearchInput"));
    m_search->setPlaceholderText(tr("Search addons"));
    m_search->setClearButtonEnabled(true);
    m_search->setFixedHeight(40);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& q) {
        if (m_browse)
            m_browse->setSearch(q.trimmed());
        if (!q.trimmed().isEmpty())
            showTab(1);
    });
    hdr->addWidget(m_search, 1);

    m_urlInput = new QLineEdit(this);
    m_urlInput->setObjectName(QStringLiteral("AddonUrlInput"));
    m_urlInput->setPlaceholderText(tr("Paste manifest URL or stremio:// link"));
    m_urlInput->setClearButtonEnabled(true);
    m_urlInput->setFixedHeight(40);
    connect(m_urlInput, &QLineEdit::returnPressed, this, &AddonsPage::doInstall);
    hdr->addWidget(m_urlInput, 1);

    m_adultBtn = new QPushButton(tr("Adult"), this);
    m_adultBtn->setObjectName(QStringLiteral("AddonAdultBtn"));
    m_adultBtn->setCheckable(true);
    m_adultBtn->setCursor(Qt::PointingHandCursor);
    m_adultBtn->setFixedHeight(40);
    connect(m_adultBtn, &QPushButton::toggled, this, [this](bool on) {
        m_adult = on;
        if (m_browse)
            m_browse->setIncludeAdult(on);
    });
    hdr->addWidget(m_adultBtn);
    outer->addLayout(hdr);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("AddonStatus"));
    m_status->setVisible(false);
    outer->addWidget(m_status);

    // Panes.
    m_panes = new QStackedWidget(this);

    // 0: Discover (featured landing — built next).
    auto* discover = new QWidget(m_panes);
    discover->setObjectName(QStringLiteral("ContentPage"));
    discover->setAttribute(Qt::WA_StyledBackground, true);
    auto* dc = new QVBoxLayout(discover);
    dc->addStretch();
    auto* dl = new QLabel(tr("Discover — featured + curated rails coming next.\n"
                             "Use Browse for the full marketplace."),
                          discover);
    dl->setObjectName(QStringLiteral("AddonPanePlaceholder"));
    dl->setAlignment(Qt::AlignCenter);
    dc->addWidget(dl);
    dc->addStretch();
    m_panes->addWidget(discover);

    // 1: Browse (live marketplace).
    m_browse = new BrowsePane(m_provider, m_registry, m_panes);
    m_panes->addWidget(m_browse);

    // 2: Installed.
    auto* installedScroll = new QScrollArea(m_panes);
    installedScroll->setObjectName(QStringLiteral("AddonScroll"));
    installedScroll->setWidgetResizable(true);
    installedScroll->setFrameShape(QFrame::NoFrame);
    installedScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_installedHost = new QWidget(installedScroll);
    m_installedHost->setObjectName(QStringLiteral("ContentPage"));
    m_installedHost->setAttribute(Qt::WA_StyledBackground, true);
    m_installedLayout = new QVBoxLayout(m_installedHost);
    m_installedLayout->setContentsMargins(0, 4, 0, 4);
    m_installedLayout->setSpacing(10);
    m_installedLayout->addStretch();
    installedScroll->setWidget(m_installedHost);
    m_panes->addWidget(installedScroll);

    outer->addWidget(m_panes, 1);

    connect(m_registry, &AddonRegistry::addonsChanged, this, [this]() {
        refreshInstalled();
        if (m_browse)
            m_browse->refreshInstalledState();
    });
    connect(m_registry, &AddonRegistry::installSucceeded, this,
            [this](const QString& name, bool replaced) {
                flashStatus((replaced ? tr("Updated %1") : tr("Installed %1")).arg(name), false);
                m_urlInput->clear();
            });
    connect(m_registry, &AddonRegistry::installFailed, this,
            [this](const QString&, const QString& err) { flashStatus(err, true); });

    refreshInstalled();
    showTab(1); // land on the live marketplace (Browse) until Discover's landing is built
}

void AddonsPage::showTab(int idx)
{
    m_panes->setCurrentIndex(idx);
    for (int i = 0; i < 3; ++i) {
        m_tabBtns[i]->setChecked(i == idx);
        m_tabBtns[i]->setProperty("active", i == idx);
        m_tabBtns[i]->style()->unpolish(m_tabBtns[i]);
        m_tabBtns[i]->style()->polish(m_tabBtns[i]);
    }
    if (idx == 1 && m_browse)
        m_browse->primeIfEmpty();
}

void AddonsPage::doInstall()
{
    const QString text = m_urlInput->text().trimmed();
    if (text.isEmpty())
        return;
    m_registry->installFromUrl(text);
}

void AddonsPage::flashStatus(const QString& text, bool error)
{
    m_status->setText((error ? QStringLiteral("✗  ") : QStringLiteral("✓  ")) + text);
    m_status->setProperty("err", error);
    m_status->style()->unpolish(m_status);
    m_status->style()->polish(m_status);
    m_status->setVisible(true);
    QPointer<QLabel> s(m_status);
    QTimer::singleShot(4500, this, [s]() {
        if (s)
            s->setVisible(false);
    });
}

void AddonsPage::refreshInstalled()
{
    while (m_installedLayout->count() > 1) {
        QLayoutItem* it = m_installedLayout->takeAt(0);
        if (it->widget())
            it->widget()->deleteLater();
        delete it;
    }

    const QVector<InstalledAddon> installed = m_registry->installed();
    if (installed.isEmpty()) {
        auto* empty = new QWidget(m_installedHost);
        empty->setObjectName(QStringLiteral("AddonEmpty"));
        empty->setAttribute(Qt::WA_StyledBackground, true);
        auto* ec = new QVBoxLayout(empty);
        ec->setContentsMargins(40, 48, 40, 48);
        ec->setSpacing(8);
        auto* t = new QLabel(tr("No addons installed yet"), empty);
        t->setObjectName(QStringLiteral("AddonEmptyTitle"));
        t->setAlignment(Qt::AlignCenter);
        auto* sub = new QLabel(
            tr("Head to Browse to install from the marketplace, or paste a manifest URL above."),
            empty);
        sub->setObjectName(QStringLiteral("AddonEmptyText"));
        sub->setAlignment(Qt::AlignCenter);
        sub->setWordWrap(true);
        ec->addWidget(t);
        ec->addWidget(sub);
        m_installedLayout->insertWidget(0, empty);
        return;
    }

    for (const InstalledAddon& a : installed) {
        auto* row = new QWidget(m_installedHost);
        row->setObjectName(QStringLiteral("AddonInstalledRow"));
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(14, 12, 14, 12);
        h->setSpacing(14);

        auto* avatar = new QLabel(row);
        avatar->setObjectName(QStringLiteral("AddonAvatar"));
        avatar->setFixedSize(44, 44);
        avatar->setAlignment(Qt::AlignCenter);
        const QString nm = a.manifest.name;
        avatar->setText(nm.isEmpty() ? QStringLiteral("?") : nm.left(1).toUpper());
        h->addWidget(avatar);

        auto* textCol = new QVBoxLayout();
        textCol->setContentsMargins(0, 0, 0, 0);
        textCol->setSpacing(2);
        auto* name = new QLabel(nm, row);
        name->setObjectName(QStringLiteral("AddonName"));
        auto* subLabel = new QLabel(subtitleFor(a.manifest), row);
        subLabel->setObjectName(QStringLiteral("AddonSub"));
        textCol->addWidget(name);
        textCol->addWidget(subLabel);
        h->addLayout(textCol, 1);

        auto* toggle = new ToggleSwitch(row);
        toggle->setOn(a.enabled);
        const QString url = a.manifestUrl;
        connect(toggle, &ToggleSwitch::toggled, this,
                [this, url](bool on) { m_registry->setEnabled(url, on); });
        h->addWidget(toggle);

        auto* remove = new QPushButton(tr("Remove"), row);
        remove->setObjectName(QStringLiteral("AddonRemoveBtn"));
        remove->setCursor(Qt::PointingHandCursor);
        connect(remove, &QPushButton::clicked, this,
                [this, url]() { m_registry->uninstall(url); });
        h->addWidget(remove);

        m_installedLayout->insertWidget(m_installedLayout->count() - 1, row);
    }
}

} // namespace tankoban

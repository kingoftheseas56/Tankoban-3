// Tankoban 3 - Play Picker source list (M6). See StreamList.h.

#include "ui/StreamList.h"

#include "ui/SourceFilterProxy.h"
#include "ui/SourceListModel.h"
#include "ui/SourceListView.h"
#include "ui/SourceRowDelegate.h"
#include "ui/StreamRowPaint.h"

#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSvgRenderer>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace tankoban {

namespace {

QIcon strokeIcon(const QString& inner, const QColor& color, int px)
{
    const QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' "
        "stroke='%1' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>%2</svg>")
        .arg(color.name(), inner);
    QSvgRenderer renderer(svg.toUtf8());
    const int hp = px * 2;
    QPixmap pm(hp, hp);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p);
    p.end();
    pm.setDevicePixelRatio(2.0);
    return QIcon(pm);
}

const QString kChevronDown = QStringLiteral("<path d='m6 9 6 6 6-6'/>");
const QString kGrid = QStringLiteral(
    "<rect width='7' height='7' x='3' y='3' rx='1'/><rect width='7' height='7' x='14' y='3' rx='1'/>"
    "<rect width='7' height='7' x='3' y='14' rx='1'/><rect width='7' height='7' x='14' y='14' rx='1'/>");

struct AddonOpt {
    QString key;
    QString name;
    int count = 0;
};

// First-appearance order (reflects the StreamScorer ranking), deduped by instance key.
QVector<AddonOpt> buildAddonOptions(const QVector<ScoredStream>& streams)
{
    QVector<AddonOpt> opts;
    QHash<QString, int> index;
    for (const ScoredStream& s : streams) {
        const QString key = addonInstanceKey(s);
        auto it = index.find(key);
        if (it != index.end()) {
            opts[it.value()].count += 1;
        } else {
            index.insert(key, opts.size());
            opts.push_back({key, !s.addonName.isEmpty() ? s.addonName : s.addonId, 1});
        }
    }
    return opts;
}

const QStringList kQualityOrder = {QStringLiteral("4K"), QStringLiteral("1080p"),
                                   QStringLiteral("720p"), QStringLiteral("SD")};

// A tiny indeterminate spinner (no Q_OBJECT needed: connects QTimer to a lambda).
class MiniSpinner final : public QWidget {
public:
    explicit MiniSpinner(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(14, 14);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        m_timer.setInterval(90);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_step = (m_step + 1) % 12;
            update();
        });
        m_timer.start();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QPointF c(width() / 2.0, height() / 2.0);
        const qreal radius = 5.0;
        for (int i = 0; i < 12; ++i) {
            const int age = (i - m_step + 12) % 12;
            const int alpha = 40 + (11 - age) * 16;
            const qreal a = (i * 30.0 - 90.0) * M_PI / 180.0;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(170, 177, 189, qBound(40, alpha, 220)));
            p.drawEllipse(QPointF(c.x() + qCos(a) * radius, c.y() + qSin(a) * radius), 1.4, 1.4);
        }
    }

private:
    QTimer m_timer;
    int m_step = 0;
};

const QString kListQss = QStringLiteral(R"qss(
QPushButton#AddonFilter { background: rgba(26,29,36,0.60); border: 1px solid rgba(255,255,255,0.06); border-radius: 16px; }
QPushButton#AddonFilter:hover { background: #1a1d24; }
QLabel#AddonCircle { background: #1a1d24; border: 1px solid rgba(255,255,255,0.06); border-radius: 18px; color:#aab1bd; font-family:"Inter","Segoe UI",sans-serif; font-size:13px; font-weight:700; }
QLabel#AddonLabel { color:#f3f1ea; font-family:"Inter","Segoe UI",sans-serif; font-size:15px; font-weight:500; }
QPushButton#QualPill { background: rgba(26,29,36,0.50); color:#aab1bd; border:1px solid rgba(255,255,255,0.06); border-radius: 13px; font-family:"Inter","Segoe UI",sans-serif; font-size:12px; font-weight:600; padding: 5px 12px; }
QPushButton#QualPill:hover { background:#1a1d24; color:#f3f1ea; }
QPushButton#QualPill:checked { background:#f3f1ea; color:#121317; border:1px solid #f3f1ea; }
QWidget#PendingPill { background: rgba(26,29,36,0.95); border:1px solid rgba(255,255,255,0.06); border-radius: 14px; }
QLabel#PendingText { color:#aab1bd; font-family:"Inter","Segoe UI",sans-serif; font-size:12px; }
QMenu#AddonMenu { background:#1a1d24; border:1px solid rgba(255,255,255,0.10); border-radius: 12px; padding:6px; }
QMenu#AddonMenu::item { color:#aab1bd; padding:8px 14px; border-radius:8px; }
QMenu#AddonMenu::item:selected { background: rgba(255,255,255,0.07); color:#f3f1ea; }
)qss");

} // namespace

StreamList::StreamList(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("StreamList"));
    setStyleSheet(kListQss);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(12);

    // Addon / source filter button.
    m_addonButton = new QPushButton(this);
    m_addonButton->setObjectName(QStringLiteral("AddonFilter"));
    m_addonButton->setMinimumHeight(52);
    m_addonButton->setCursor(Qt::PointingHandCursor);
    auto* bl = new QHBoxLayout(m_addonButton);
    bl->setContentsMargins(12, 8, 16, 8);
    bl->setSpacing(12);
    m_addonCircle = new QLabel(m_addonButton);
    m_addonCircle->setObjectName(QStringLiteral("AddonCircle"));
    m_addonCircle->setFixedSize(36, 36);
    m_addonCircle->setAlignment(Qt::AlignCenter);
    m_addonCircle->setAttribute(Qt::WA_TransparentForMouseEvents);
    bl->addWidget(m_addonCircle);
    m_addonLabel = new QLabel(QStringLiteral("All"), m_addonButton);
    m_addonLabel->setObjectName(QStringLiteral("AddonLabel"));
    m_addonLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    bl->addWidget(m_addonLabel);
    bl->addStretch();
    auto* chev = new QLabel(m_addonButton);
    chev->setAttribute(Qt::WA_TransparentForMouseEvents);
    chev->setPixmap(strokeIcon(kChevronDown, QColor(0xaa, 0xb1, 0xbd), 18).pixmap(18, 18));
    bl->addWidget(chev);
    connect(m_addonButton, &QPushButton::clicked, this, &StreamList::showAddonMenu);
    col->addWidget(m_addonButton);

    // Quality filter bar.
    m_qualityBar = new QWidget(this);
    m_qualityLayout = new QHBoxLayout(m_qualityBar);
    m_qualityLayout->setContentsMargins(0, 0, 0, 0);
    m_qualityLayout->setSpacing(6);
    col->addWidget(m_qualityBar);

    // Source rows: native model/view stack (data -> ranked proxy -> virtualized view).
    // The QListView owns the source-list scrolling viewport, so only visible rows paint.
    m_model = new SourceListModel(this);
    m_proxy = new SourceFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_delegate = new SourceRowDelegate(this);
    m_view = new SourceListView(this);
    m_view->setModel(m_proxy);
    m_view->setItemDelegate(m_delegate);
    col->addWidget(m_view, 1);

    connect(m_view, &SourceListView::playActivated, this, [this](const QModelIndex& i) {
        emit streamActivated(i.data(SourceListModel::StreamRole).value<ScoredStream>());
    });
    connect(m_view, &SourceListView::copyActivated, this, [this](const QModelIndex& i) {
        const auto s = i.data(SourceListModel::StreamRole).value<ScoredStream>();
        const QString link = (!s.url.isEmpty() && s.url != QStringLiteral("#")) ? s.url : s.externalUrl;
        if (!link.isEmpty()) {
            QGuiApplication::clipboard()->setText(link);
            m_delegate->setCopiedKey(streamRowKey(s));
            m_view->viewport()->update();
            QTimer::singleShot(1200, this, [this] {
                m_delegate->setCopiedKey(QString());
                m_view->viewport()->update();
            });
        }
    });

    // Pending-addons pill (centered).
    auto* pendingRow = new QWidget(this);
    auto* pendingRowLay = new QHBoxLayout(pendingRow);
    pendingRowLay->setContentsMargins(0, 6, 0, 0);
    pendingRowLay->addStretch();
    m_pending = new QWidget(pendingRow);
    m_pending->setObjectName(QStringLiteral("PendingPill"));
    auto* pl = new QHBoxLayout(m_pending);
    pl->setContentsMargins(12, 6, 14, 6);
    pl->setSpacing(9);
    pl->addWidget(new MiniSpinner(m_pending), 0, Qt::AlignVCenter);
    m_pendingLabel = new QLabel(QStringLiteral("Waiting for sources..."), m_pending);
    m_pendingLabel->setObjectName(QStringLiteral("PendingText"));
    pl->addWidget(m_pendingLabel, 0, Qt::AlignVCenter);
    pendingRowLay->addWidget(m_pending);
    pendingRowLay->addStretch();
    col->addWidget(pendingRow);

    refreshAddonButton();
    rebuildQualityBar();
    updatePending();
}

void StreamList::setStreams(const QVector<ScoredStream>& streams, const Options& options)
{
    m_streams = streams;
    m_options = options;

    // Preserve the user's filter selection across (M9) partial updates, but drop it
    // if the chosen addon/quality is no longer present.
    if (m_addonFilter != QStringLiteral("all")) {
        bool found = false;
        for (const ScoredStream& s : m_streams) {
            if (addonInstanceKey(s) == m_addonFilter) {
                found = true;
                break;
            }
        }
        if (!found)
            m_addonFilter = QStringLiteral("all");
    }

    refreshAddonButton();
    rebuildQualityBar(); // validates m_qualityFilter against current groups
    m_model->setStreams(streams);               // incremental key-merge; repaints visible rows only
    m_proxy->setAddonFilter(m_addonFilter);
    m_proxy->setQualityFilter(m_qualityFilter);
    updatePending();
}

void StreamList::clearStreams()
{
    m_streams.clear();
    m_addonFilter = QStringLiteral("all");
    m_qualityFilter = QStringLiteral("all");
    refreshAddonButton();
    rebuildQualityBar();
    m_model->clear();
    m_proxy->setAddonFilter(m_addonFilter);
    m_proxy->setQualityFilter(m_qualityFilter);
    updatePending();
}

void StreamList::refreshAddonButton()
{
    if (m_addonFilter == QStringLiteral("all")) {
        m_addonLabel->setText(QStringLiteral("All"));
        m_addonCircle->setText(QString());
        m_addonCircle->setPixmap(strokeIcon(kGrid, QColor(0xaa, 0xb1, 0xbd), 16).pixmap(16, 16));
        return;
    }
    QString name = m_addonFilter;
    for (const AddonOpt& o : buildAddonOptions(m_streams)) {
        if (o.key == m_addonFilter) {
            name = o.name;
            break;
        }
    }
    m_addonLabel->setText(name);
    m_addonCircle->setPixmap(QPixmap());
    const QString initial = name.trimmed().isEmpty() ? QStringLiteral("?")
                                                      : name.trimmed().left(1).toUpper();
    m_addonCircle->setText(initial);
}

void StreamList::showAddonMenu()
{
    QMenu menu(this);
    menu.setObjectName(QStringLiteral("AddonMenu"));

    QAction* all = menu.addAction(QStringLiteral("All sources  ·  %1").arg(m_streams.size()));
    all->setData(QStringLiteral("all"));
    for (const AddonOpt& o : buildAddonOptions(m_streams)) {
        QAction* a = menu.addAction(QStringLiteral("%1  ·  %2").arg(o.name).arg(o.count));
        a->setData(o.key);
    }

    QAction* chosen = menu.exec(m_addonButton->mapToGlobal(QPoint(0, m_addonButton->height() + 4)));
    if (!chosen)
        return;
    const QString key = chosen->data().toString();
    if (key == m_addonFilter)
        return;
    m_addonFilter = key;
    refreshAddonButton();
    rebuildQualityBar(); // groups depend on the addon-filtered set
    m_proxy->setAddonFilter(m_addonFilter);
    m_proxy->setQualityFilter(m_qualityFilter); // rebuildQualityBar may have reset it to "all"
}

void StreamList::rebuildQualityBar()
{
    QLayoutItem* item = nullptr;
    while ((item = m_qualityLayout->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    // Quality groups of the addon-filtered set, in tier order.
    QHash<QString, int> counts;
    int total = 0;
    for (const ScoredStream& s : m_streams) {
        if (m_addonFilter != QStringLiteral("all") && addonInstanceKey(s) != m_addonFilter)
            continue;
        counts[qualityGroupOf(s)] += 1;
        ++total;
    }
    QStringList groups;
    for (const QString& g : kQualityOrder) {
        if (counts.contains(g))
            groups << g;
    }

    // Show only when there are at least 2 quality groups (Harbor).
    if (groups.size() < 2) {
        if (m_qualityFilter != QStringLiteral("all"))
            m_qualityFilter = QStringLiteral("all");
        m_qualityBar->hide();
        return;
    }
    m_qualityBar->show();

    if (m_qualityFilter != QStringLiteral("all") && !groups.contains(m_qualityFilter))
        m_qualityFilter = QStringLiteral("all");

    auto addPill = [this](const QString& key, const QString& label, int count) {
        auto* pill = new QPushButton(QStringLiteral("%1  %2").arg(label).arg(count), m_qualityBar);
        pill->setObjectName(QStringLiteral("QualPill"));
        pill->setCheckable(true);
        pill->setCursor(Qt::PointingHandCursor);
        pill->setChecked(m_qualityFilter == key);
        connect(pill, &QPushButton::clicked, this, [this, key]() {
            if (m_qualityFilter == key)
                return;
            m_qualityFilter = key;
            rebuildQualityBar();
            m_proxy->setAddonFilter(m_addonFilter);
            m_proxy->setQualityFilter(m_qualityFilter);
        });
        m_qualityLayout->addWidget(pill);
    };

    addPill(QStringLiteral("all"), QStringLiteral("All"), total);
    for (const QString& g : groups)
        addPill(g, g, counts.value(g));
    m_qualityLayout->addStretch();
}

void StreamList::updatePending()
{
    const bool show = !m_options.pipelineDone || m_options.loadingAddonCount > 0;
    if (m_options.loadingAddonCount > 0) {
        m_pendingLabel->setText(m_options.loadingAddonCount == 1
                                    ? QStringLiteral("Waiting for 1 addon...")
                                    : QStringLiteral("Waiting for %1 addons...")
                                          .arg(m_options.loadingAddonCount));
    } else {
        m_pendingLabel->setText(QStringLiteral("Waiting for sources..."));
    }
    m_pending->parentWidget()->setVisible(show);
}

} // namespace tankoban

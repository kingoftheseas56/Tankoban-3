// Tankoban 3 - Play Picker page shell (M5). See PlayPickerPage.h.

#include "ui/PlayPickerPage.h"

#include "core/MetaService.h"
#include "ui/BackdropLayer.h"
#include "ui/Icons.h"
#include "ui/PickerHeader.h"
#include "ui/StreamList.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace tankoban {

namespace {
class LoadingSpinner final : public QWidget {
public:
    explicit LoadingSpinner(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(24, 24);
        m_timer.setInterval(80);
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
        const QPointF center(width() / 2.0, height() / 2.0);
        const qreal radius = 8.0;

        for (int i = 0; i < 12; ++i) {
            const int age = (i - m_step + 12) % 12;
            const int alpha = 52 + (11 - age) * 17;
            const qreal angle = (i * 30.0 - 90.0) * M_PI / 180.0;
            const QPointF dot(center.x() + qCos(angle) * radius,
                              center.y() + qSin(angle) * radius);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(243, 241, 234, qBound(52, alpha, 230)));
            p.drawEllipse(dot, 2.1, 2.1);
        }
    }

private:
    QTimer m_timer;
    int m_step = 0;
};
} // namespace

PlayPickerPage::PlayPickerPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("PlayPickerPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(R"qss(
QWidget#PlayPickerPage { background: #121317; }
QPushButton#PlayPickerBack {
    background: rgba(18,19,23,0.80);
    border: 1px solid rgba(255,255,255,0.08);
    border-radius: 22px;
}
QPushButton#PlayPickerBack:hover { background: #1a1d24; }
QScrollArea#PlayPickerScroll { background: transparent; border: none; }
QScrollArea#PlayPickerScroll > QWidget { background: transparent; }
QFrame#LoadingPanel {
    background: rgba(26,29,36,0.62);
    border: 1px solid rgba(255,255,255,0.05);
    border-radius: 16px;
}
QLabel#LoadingText {
    color: #aab1bd;
    font-family: "Inter", "Segoe UI", sans-serif;
    font-size: 14px;
    font-weight: 600;
}
)qss"));

    m_backdrop = new BackdropLayer(this);
    m_backdrop->setGeometry(rect());
    m_backdrop->lower();

    m_meta = new MetaService(this);
    connect(m_meta, &MetaService::detailReady, this,
            [this](const QString& id, const MetaDetail& detail) {
                if (id != m_currentId)
                    return;

                const bool episodeHasThumbnail =
                    m_currentEpisode && !m_currentEpisode->thumbnail.isEmpty();
                if (!episodeHasThumbnail) {
                    const QString refreshedBackdrop =
                        !detail.background.isEmpty() ? detail.background : detail.poster;
                    m_backdrop->setSource(refreshedBackdrop);
                }

                if (!m_currentEpisode)
                    m_header->setMeta(detail.name, detail.releaseInfo, detail.genres);
            });

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* body = new QWidget(this);
    body->setAttribute(Qt::WA_TranslucentBackground);
    body->setAutoFillBackground(false);

    auto* outer = new QVBoxLayout(body);
    outer->setContentsMargins(48, 128, 48, 128);
    outer->setSpacing(48);

    auto* inner = new QWidget(body);
    inner->setAttribute(Qt::WA_TranslucentBackground);
    inner->setMaximumWidth(1024);
    inner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    auto* col = new QVBoxLayout(inner);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(48);

    m_header = new PickerHeader(inner);
    col->addWidget(m_header);

    m_loadingPanel = new QFrame(inner);
    m_loadingPanel->setObjectName(QStringLiteral("LoadingPanel"));
    m_loadingPanel->setMinimumHeight(96);
    auto* loadingLayout = new QHBoxLayout(m_loadingPanel);
    loadingLayout->setContentsMargins(24, 22, 24, 22);
    loadingLayout->setSpacing(14);
    m_spinner = new LoadingSpinner(m_loadingPanel);
    loadingLayout->addWidget(m_spinner, 0, Qt::AlignVCenter);
    m_loading = new QLabel(QStringLiteral("Loading streams..."), m_loadingPanel);
    m_loading->setObjectName(QStringLiteral("LoadingText"));
    loadingLayout->addWidget(m_loading, 0, Qt::AlignVCenter);
    loadingLayout->addStretch();
    col->addWidget(m_loadingPanel);

    // M6 source list — hidden until setPicker(...) feeds ranked streams (M9 wires real data).
    m_streamList = new StreamList(inner);
    m_streamList->hide();
    connect(m_streamList, &StreamList::streamActivated, this, &PlayPickerPage::streamSelected);
    col->addWidget(m_streamList);

    col->addStretch();

    auto* centeredRow = new QWidget(body);
    centeredRow->setAttribute(Qt::WA_TranslucentBackground);
    auto* centeredLayout = new QHBoxLayout(centeredRow);
    centeredLayout->setContentsMargins(0, 0, 0, 0);
    centeredLayout->setSpacing(0);
    centeredLayout->addStretch();
    centeredLayout->addWidget(inner, 1);
    centeredLayout->addStretch();
    outer->addWidget(centeredRow);
    outer->addStretch();

    // Harbor's picker root is `overflow-y-auto`: header + content scroll over a FIXED
    // backdrop. The backdrop stays a lowered sibling (NOT inside the viewport); the
    // viewport is WA_TranslucentBackground so it shows through AND recomposites when the
    // backdrop's async image lands (the M5 striping was a missing-translucent viewport
    // that never repainted on backdrop update).
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("PlayPickerScroll"));
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setAttribute(Qt::WA_TranslucentBackground, true);
    m_scroll->viewport()->setAutoFillBackground(false);
    m_scroll->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    m_scroll->setWidget(body);
    root->addWidget(m_scroll);

    m_backdrop->lower();

    m_back = new QPushButton(this);
    m_back->setObjectName(QStringLiteral("PlayPickerBack"));
    m_back->setIcon(navIcon(QStringLiteral("chev-left"), QColor(0xf3, 0xf1, 0xea), 22));
    m_back->setIconSize(QSize(22, 22));
    m_back->setCursor(Qt::PointingHandCursor);
    m_back->setFixedSize(44, 44);
    connect(m_back, &QPushButton::clicked, this, [this]() { emit backRequested(); });

    hide();
}

void PlayPickerPage::open(const MetaDetail& meta, std::optional<EpisodeItem> episode)
{
    m_currentId = meta.id;
    m_currentEpisode = episode;

    const QString backdrop = episode && !episode->thumbnail.isEmpty()
        ? episode->thumbnail
        : (!meta.background.isEmpty() ? meta.background : meta.poster);
    m_backdrop->setSource(backdrop);

    if (episode) {
        m_header->setEpisode(meta.name,
                             episode->season,
                             episode->episode,
                             episode->name,
                             episode->overview);
    } else {
        m_header->setMeta(meta.name, meta.releaseInfo, meta.genres);
    }

    // M6: every open starts on the M5 loading placeholder; M9 calls setPicker(...) with data.
    m_loadingPanel->show();
    if (m_streamList) {
        m_streamList->clearStreams();
        m_streamList->hide();
    }

    setGeometry(parentWidget() ? parentWidget()->rect() : QRect(QPoint(0, 0), size()));
    show();
    raise();
    positionBackButton();

    if (m_meta && !meta.id.isEmpty() && !meta.type.isEmpty())
        m_meta->fetchDetail(meta.type, meta.id);
}

void PlayPickerPage::setPicker(const RankedPicker& picker, bool pipelineDone, int loadingAddonCount)
{
    if (m_loadingPanel)
        m_loadingPanel->hide();
    StreamList::Options opts;
    opts.pipelineDone = pipelineDone;
    opts.loadingAddonCount = loadingAddonCount;
    opts.preserveOrder = true;
    m_streamList->setStreams(picker.all, opts);
    m_streamList->show();
}

void PlayPickerPage::setLoading(const QString& message)
{
    if (m_loading)
        m_loading->setText(message);
    if (m_streamList) {
        m_streamList->clearStreams();
        m_streamList->hide();
    }
    if (m_loadingPanel)
        m_loadingPanel->show();
}

void PlayPickerPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_backdrop)
        m_backdrop->setGeometry(rect());
    positionBackButton();
}

void PlayPickerPage::positionBackButton()
{
    if (!m_back)
        return;
    m_back->move(20, 20);
    m_back->raise();
}

} // namespace tankoban

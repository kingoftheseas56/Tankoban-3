// Tankoban 3 — MainWindow (Step 2 + Detail + Addons). See MainWindow.h.

#include "ui/MainWindow.h"

#include "core/AddonRegistry.h"
#include "core/MetaDetail.h"
#include "core/StreamParser.h"
#include "core/StreamScorer.h"
#include "core/StreamService.h"
#include "ui/AddonsPage.h"
#include "ui/DetailPage.h"
#include "ui/HomePage.h"
#include "ui/RowPage.h"
#include "ui/GridPage.h"
#include "ui/ShowsPage.h"
#include "ui/PlayPickerPage.h"
#include "ui/Sidebar.h"
#include "ui/VideoPlayerPage.h"

#include <QCursor>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStackedWidget>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM
#  include <dwmapi.h>     // DwmSetWindowAttribute (dark window frame)
#  pragma comment(lib, "dwmapi.lib")
#endif

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

// Hand-drawn window-control glyphs (Harbor topbar.tsx parity): minimize = flat
// line, maximize = rounded square, restore = two offset squares, close = X.
QIcon chromeIcon(const QString& kind)
{
    QPixmap pm(13, 13);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(0xc9, 0xce, 0xd6));
    pen.setWidthF(1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    if (kind == QLatin1String("min")) {
        p.drawLine(QPointF(3, 6.5), QPointF(10, 6.5));
    } else if (kind == QLatin1String("max")) {
        p.drawRoundedRect(QRectF(3, 3, 7, 7), 1.2, 1.2);
    } else if (kind == QLatin1String("restore")) {
        p.drawRoundedRect(QRectF(2.5, 4.5, 6, 6), 1.0, 1.0);
        QPainterPath back;
        back.moveTo(5, 4.5);
        back.lineTo(5, 3);
        back.lineTo(10, 3);
        back.lineTo(10, 8);
        back.lineTo(9, 8);
        p.drawPath(back);
    } else { // close
        p.drawLine(QPointF(3.5, 3.5), QPointF(9.5, 9.5));
        p.drawLine(QPointF(9.5, 3.5), QPointF(3.5, 9.5));
    }
    return QIcon(pm);
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Root"));
    setWindowTitle(QStringLiteral("Tankoban 3"));
    resize(1280, 800);
    // Frameless, Harbor-faithful: no OS title bar — min/max/close live in our own
    // top bar. Aero snap / resize edges / drop shadow are restored in
    // applyFramelessWin32Style(); empty top-bar zones become the drag handle.
    setWindowFlag(Qt::FramelessWindowHint, true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    buildTopBar();
    outer->addWidget(m_topBar);

    auto* body = new QWidget(this);
    body->setObjectName(QStringLiteral("ShellBody"));
    auto* row = new QHBoxLayout(body);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_sidebar = new Sidebar(this);
    row->addWidget(m_sidebar);

    m_content = new QStackedWidget(this);
    m_content->setObjectName(QStringLiteral("Content"));
    m_content->setAttribute(Qt::WA_StyledBackground, true); // opaque backstop (no page bleed)
    row->addWidget(m_content, 1);

    outer->addWidget(body, 1);

    m_registry = new AddonRegistry(this);
    m_registry->load();
    m_registry->seedDefaultsIfNeeded();   // first run: install the curated default addons

    for (const auto& v : kViews) {
        const QString id = QString::fromLatin1(v.id);
        QWidget* page = nullptr;
        if (id == QLatin1String("home")) {
            auto* home = new HomePage(this);
            connect(home, &HomePage::openDetailRequested, this, &MainWindow::openDetail);
            connect(home, &HomePage::openGridRequested, this, &MainWindow::openGrid);
            page = home;
        } else if (id == QLatin1String("addons")) {
            page = new AddonsPage(m_registry, this);
        } else if (id == QLatin1String("movies")) {
            auto* rowPage = new RowPage(id, this);
            connect(rowPage, &RowPage::openDetailRequested, this, &MainWindow::openDetail);
            connect(rowPage, &RowPage::openGridRequested, this, &MainWindow::openGrid);
            connect(rowPage, &RowPage::playRequested, this,
                    [this](const MetaItem& m) { openPlayPicker(m, std::nullopt); });
            page = rowPage;
        } else if (id == QLatin1String("shows")) {
            auto* showsPage = new ShowsPage(this);
            connect(showsPage, &ShowsPage::openDetailRequested, this, &MainWindow::openDetail);
            connect(showsPage, &ShowsPage::openGridRequested, this, &MainWindow::openGrid);
            connect(showsPage, &ShowsPage::playRequested, this,
                    [this](const MetaItem& m) { openPlayPicker(m, std::nullopt); });
            page = showsPage;
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
    connect(m_detailPage, &DetailPage::playRequested, this,
            [this](const MetaItem& m) { openPlayPicker(m, std::nullopt); });
    connect(m_detailPage, &DetailPage::episodeRequested, this,
            [this](const MetaItem& m, const EpisodeItem& ep) { openPlayPicker(m, ep); });

    // "View all" category grid is a pushed frame too (sidebar stays visible, Harbor-style);
    // back returns to whatever route/page opened it; a poster opens Detail over the grid.
    m_gridPage = new GridPage(this);
    m_gridIndex = m_content->addWidget(m_gridPage);
    connect(m_gridPage, &GridPage::backRequested, this,
            [this]() { m_content->setCurrentIndex(m_gridReturnIndex); });
    connect(m_gridPage, &GridPage::openDetailRequested, this, &MainWindow::openDetail);

    m_playPicker = new PlayPickerPage(this);
    m_playPicker->setGeometry(rect());
    m_playPicker->hide();
    connect(m_playPicker, &PlayPickerPage::backRequested, this, [this]() {
        if (m_playPicker)
            m_playPicker->hide();
    });

    // Stream pipeline: on picker open we fetch real addon streams, rank them through
    // StreamParser/StreamScorer, and feed the StremioRows. Picking a DIRECT row opens
    // the in-app player. (Direct links only this slice — torrents/debrid deferred.)
    m_streamNetwork = new QNetworkAccessManager(this);
    m_streamService = new StreamService(m_registry, m_streamNetwork, this);
    connect(m_streamService, &StreamService::streamsPartial, this, [this](QVector<Stream> streams) {
        if (m_playPicker)
            m_playPicker->setPicker(buildPicker(streams), /*pipelineDone=*/false, 0);
    });
    connect(m_streamService, &StreamService::streamsReady, this, [this](QVector<Stream> streams) {
        if (m_playPicker)
            m_playPicker->setPicker(buildPicker(streams), /*pipelineDone=*/true, 0);
    });
    connect(m_streamService, &StreamService::streamsFailed, this, [this](const QString&) {
        if (m_playPicker)
            m_playPicker->setLoading(
                QStringLiteral("No stream addons configured. Install one in Addons."));
    });
    connect(m_playPicker, &PlayPickerPage::streamSelected, this, &MainWindow::openDirectPlayer);

    connect(m_sidebar, &Sidebar::viewActivated, this, [this](const QString& id) {
        if (m_pageIndex.contains(id))
            m_content->setCurrentIndex(m_pageIndex.value(id));
        m_sidebar->setActive(id);
    });

    m_content->setCurrentIndex(m_pageIndex.value(QStringLiteral("home")));

    // winId() forces native HWND creation so the Win32 style re-add has a handle.
    applyFramelessWin32Style();
    updateMaxRestoreIcon();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_playPicker)
        m_playPicker->setGeometry(rect());
    // m_player is a content-stack page now — the layout sizes it, no manual geometry.
}

void MainWindow::openDetail(const MetaItem& meta)
{
    if (!m_detailPage)
        return;
    m_detailPage->load(meta);
    m_returnIndex = m_content->currentIndex();
    m_content->setCurrentIndex(m_detailIndex);
}

void MainWindow::openGrid(const QString& title, const QVector<MetaItem>& items)
{
    if (!m_gridPage)
        return;
    m_gridPage->setCatalog(title, items);
    m_gridReturnIndex = m_content->currentIndex();
    m_content->setCurrentIndex(m_gridIndex);
}

void MainWindow::openPlayPicker(const MetaItem& meta, std::optional<EpisodeItem> episode)
{
    if (!m_playPicker || !m_streamService)
        return;
    m_currentPickerMeta = detailFromMetaItem(meta);
    m_currentPickerEpisode = episode;
    m_playPicker->setGeometry(rect());
    m_playPicker->open(m_currentPickerMeta, episode);
    m_playPicker->setLoading(QStringLiteral("Loading sources..."));
    // Pointer into the member optional (stable lifetime) — never borrow from the param.
    m_streamService->fetchStreams(
        m_currentPickerMeta, m_currentPickerEpisode ? &*m_currentPickerEpisode : nullptr);
}

RankedPicker MainWindow::buildPicker(const QVector<Stream>& streams) const
{
    QVector<ParsedStream> parsed;
    parsed.reserve(streams.size());
    for (const Stream& s : streams)
        parsed.push_back(StreamParser::parseStream(s));

    ScoreOptions opts;
    opts.mediaKind = m_currentPickerMeta.type;   // "movie" / "series"

    const CorpusStats corpus = StreamScorer::computeCorpusStats(parsed, opts);

    QVector<ScoredStream> scored;
    scored.reserve(parsed.size());
    for (int i = 0; i < parsed.size(); ++i) {
        ScoredStream s = StreamScorer::scoreStream(parsed[i], opts, &corpus);
        if (s.nativeIdx < 0)
            s.nativeIdx = i;
        scored.push_back(s);
    }

    return StreamScorer::rankAndPick(scored, /*activeDebrids=*/{}, /*preferAac=*/false,
                                     opts.respectAddonOrder);
}

void MainWindow::openDirectPlayer(const ScoredStream& stream)
{
    // Direct links only this slice. StremioRow withholds the play signal for
    // non-direct rows already; guard defensively at the seam too.
    const QString url = stream.url;
    if (url.isEmpty() || url == QLatin1String("#"))
        return;

    if (!m_player) {
        m_player = new VideoPlayerPage(this);
        m_playerIndex = m_content->addWidget(m_player);   // a real page in the shell
        connect(m_player, &VideoPlayerPage::backRequested, this, [this] { closePlayer(); });
    }

    PlayerRequest req;
    req.url = url;
    req.title = m_currentPickerMeta.name.isEmpty() ? stream.parsedTitle : m_currentPickerMeta.name;
    if (m_currentPickerEpisode) {
        const EpisodeItem& ep = *m_currentPickerEpisode;
        req.subtitle = ep.name.isEmpty()
            ? QStringLiteral("S%1 · E%2").arg(ep.season).arg(ep.episode)
            : QStringLiteral("S%1 · E%2 · %3").arg(ep.season).arg(ep.episode).arg(ep.name);
    }

    // Immersive, in-app: hide the picker overlay + shell chrome (sidebar / top bar)
    // and swap the content stack to the player page so it fills the window. Switch
    // BEFORE play() so the GL surface is exposed when the first mpv frame lands —
    // otherwise the render-update coalescing flag can stick and freeze the frame.
    if (m_playPicker) m_playPicker->hide();
    if (m_sidebar) m_sidebar->hide();
    if (m_topBar) m_topBar->hide();
    m_content->setCurrentIndex(m_playerIndex);
    m_player->play(req);
}

void MainWindow::closePlayer()
{
    if (m_player) m_player->stop();
    if (m_sidebar) m_sidebar->show();
    if (m_topBar) m_topBar->show();
    m_content->setCurrentIndex(m_detailIndex >= 0 ? m_detailIndex : 0);
    // Return to the picker we came from — it still holds the ranked streams.
    if (m_playPicker) {
        m_playPicker->setGeometry(rect());
        m_playPicker->show();
        m_playPicker->raise();
    }
}

MetaDetail MainWindow::detailFromMetaItem(const MetaItem& meta) const
{
    MetaDetail detail;
    detail.id = meta.id;
    detail.type = meta.type;
    detail.name = meta.name;
    detail.poster = meta.poster;
    detail.background = meta.background;
    detail.description = meta.description;
    detail.releaseInfo = meta.releaseInfo;
    detail.imdbRating = meta.imdbRating;
    detail.runtime = meta.runtime;
    return detail;
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

// ── Frameless window chrome (Harbor-style: no OS title bar) ──────────────────

void MainWindow::buildTopBar()
{
    m_topBar = new QWidget(this);
    m_topBar->setObjectName(QStringLiteral("TopBar"));
    m_topBar->setAttribute(Qt::WA_StyledBackground, true);
    m_topBar->setFixedHeight(40);
    m_topBar->setStyleSheet(QStringLiteral(R"qss(
QWidget#TopBar { background: #0f1116; }
QWidget#TopBar QPushButton { background: transparent; border: none; border-radius: 7px; }
QWidget#TopBar QPushButton:hover { background: rgba(255,255,255,0.10); }
)qss"));

    auto* lay = new QHBoxLayout(m_topBar);
    lay->setContentsMargins(10, 4, 8, 4);
    lay->setSpacing(2);
    lay->addStretch(1);

    const auto makeBtn = [this](const QString& kind, const QString& name) {
        auto* b = new QPushButton(m_topBar);
        b->setObjectName(name);
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(42, 30);
        b->setIconSize(QSize(13, 13));
        b->setIcon(chromeIcon(kind));
        return b;
    };

    m_btnMin = makeBtn(QStringLiteral("min"), QStringLiteral("ChromeMin"));
    m_btnMax = makeBtn(QStringLiteral("max"), QStringLiteral("ChromeMax"));
    m_btnClose = makeBtn(QStringLiteral("close"), QStringLiteral("ChromeClose"));
    lay->addWidget(m_btnMin);
    lay->addWidget(m_btnMax);
    lay->addWidget(m_btnClose);

    connect(m_btnMin, &QPushButton::clicked, this, [this] { showMinimized(); });
    connect(m_btnMax, &QPushButton::clicked, this, &MainWindow::toggleMaximizeRestore);
    connect(m_btnClose, &QPushButton::clicked, this, [this] { close(); });
}

void MainWindow::toggleMaximizeRestore()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
}

void MainWindow::updateMaxRestoreIcon()
{
    if (m_btnMax)
        m_btnMax->setIcon(chromeIcon(isMaximized() ? QStringLiteral("restore")
                                                   : QStringLiteral("max")));
}

void MainWindow::applyFramelessWin32Style()
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd) {
        // Re-add WS_THICKFRAME / max / min / caption so Aero snap, Win+Arrow,
        // taskbar previews, resize cursors, and the drop shadow stay native —
        // Qt::FramelessWindowHint strips them. (Tankoban 2's proven technique.)
        LONG style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE,
                        style | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION);
        ::SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                       SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        // Win11 paints a 1px window border OUTSIDE the client (light in light mode)
        // — that's the white rim around the player. Force the immersive dark frame
        // AND remove the border color outright so no white edge remains.
        const BOOL dark = TRUE;
        ::DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
        const COLORREF noBorder = 0xFFFFFFFE; // DWMWA_COLOR_NONE
        ::DwmSetWindowAttribute(hwnd, 34 /*DWMWA_BORDER_COLOR*/, &noBorder, sizeof(noBorder));
    }
#endif
}

void MainWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    // Re-assert the frameless/DWM chrome once the window is actually shown — the
    // constructor-time call can be too early for the DWM attributes to stick.
    if (!m_chromeApplied) {
        m_chromeApplied = true;
        applyFramelessWin32Style();
        updateMaxRestoreIcon();
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        updateMaxRestoreIcon();
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    if (eventType != "windows_generic_MSG")
        return QWidget::nativeEvent(eventType, message, result);

    auto* msg = static_cast<MSG*>(message);

    // WM_NCCALCSIZE: client area covers the full window (no reserved title strip).
    // When maximized, inset by the frame metrics so content isn't clipped off-screen.
    if (msg->message == WM_NCCALCSIZE && msg->wParam == TRUE) {
        // Client covers the full window rect (no OS title strip). When maximized,
        // WM_GETMINMAXINFO (below) constrains the window itself to the monitor work
        // area, so the client equals the work area too — taskbar stays visible and
        // nothing is clipped.
        *result = 0;
        return true;
    }

    // WM_GETMINMAXINFO: a frameless window otherwise maximizes over the WHOLE
    // monitor and covers the taskbar (the leftover non-client strip showed white
    // over it). Constrain the maximized size/position to the monitor WORK AREA so
    // the real taskbar stays visible — the correct fix, not a client clamp.
    if (msg->message == WM_GETMINMAXINFO) {
        HMONITOR mon = ::MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (::GetMonitorInfo(mon, &mi)) {
            const RECT rw = mi.rcWork;
            const RECT rm = mi.rcMonitor;
            auto* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
            mmi->ptMaxPosition.x = rw.left - rm.left;
            mmi->ptMaxPosition.y = rw.top - rm.top;
            mmi->ptMaxSize.x = rw.right - rw.left;
            mmi->ptMaxSize.y = rw.bottom - rw.top;
            mmi->ptMaxTrackSize.x = rw.right - rw.left;
            mmi->ptMaxTrackSize.y = rw.bottom - rw.top;
        }
        *result = 0;
        return true;
    }

    // WM_NCHITTEST: 6px edges -> resize handles; empty top-bar surface -> HTCAPTION
    // (native drag + double-click-maximize + right-click system menu); else client.
    if (msg->message == WM_NCHITTEST) {
        // QCursor::pos() is in Qt LOGICAL global coords (DPI-correct). The raw
        // lParam is in PHYSICAL pixels, which mismatches Qt's logical coords on
        // scaled displays and dumped the control buttons into the caption (drag)
        // zone — making their clicks vanish. Map from the cursor instead.
        const QPoint local = mapFromGlobal(QCursor::pos());

        const int margin = 6;
        const QRect r = rect();
        const bool onLeft = local.x() < margin;
        const bool onRight = local.x() >= r.width() - margin;
        const bool onTop = local.y() < margin;
        const bool onBottom = local.y() >= r.height() - margin;
        if (onTop && onLeft) { *result = HTTOPLEFT; return true; }
        if (onTop && onRight) { *result = HTTOPRIGHT; return true; }
        if (onBottom && onLeft) { *result = HTBOTTOMLEFT; return true; }
        if (onBottom && onRight) { *result = HTBOTTOMRIGHT; return true; }
        if (onLeft) { *result = HTLEFT; return true; }
        if (onRight) { *result = HTRIGHT; return true; }
        if (onTop) { *result = HTTOP; return true; }
        if (onBottom) { *result = HTBOTTOM; return true; }

        // Only the EMPTY top-bar surface drags the window. childAt() across the
        // whole window means control buttons (and any overlay on top, e.g. the
        // picker's own back button) keep their clicks instead of being hijacked.
        if (m_topBar && local.y() < m_topBar->height() && childAt(local) == m_topBar) {
            *result = HTCAPTION;
            return true;
        }

        *result = HTCLIENT;
        return true;
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

} // namespace tankoban

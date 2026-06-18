// Tankoban 3 — entry point.
//
// A native C++/Qt6 app: the lean, faithful recreation of Harbor's video mode, built
// incrementally and later customized into our own. Step 1 was the empty window;
// Step 2 brings the shell (Harbor sidebar + dark content area).

#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "player/PlayerView.h"
#include "engine/MpvController.h"
#include "core/torrentstream/LtLinkSmoke.h"
#include "core/torrentstream/StreamEngine.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <cstdio>
#include <memory>

namespace {

QString demoTitleFromSource(const QString& src)
{
    const QFileInfo fileInfo(src);
    if (fileInfo.exists()) {
        const QString base = fileInfo.completeBaseName();
        return base.isEmpty() ? fileInfo.fileName() : base;
    }

    const QUrl url(src);
    if (url.isValid()) {
        const QString fileName = QFileInfo(url.path()).completeBaseName();
        if (!fileName.isEmpty()) return fileName;
        if (!url.host().isEmpty()) return url.host();
    }
    return src;
}

QRect demoWindowedRect(const QRect& available)
{
    if (!available.isValid()) return QRect(0, 0, 1280, 720);

    QSize size(1280, 720);
    QRect inset = available.adjusted(48, 32, -48, -48);
    if (!inset.isValid()) inset = available;
    size.scale(inset.size(), Qt::KeepAspectRatio);

    QRect rect(QPoint(0, 0), size);
    rect.moveCenter(available.center());
    return rect;
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tankoban 3"));
    app.setOrganizationName(QStringLiteral("Tankoban"));
    app.setStyleSheet(tankoban::appStyleSheet());

    // Phase 2 (throwaway): prove the vendored libtorrent links + a session
    // constructs in-process. Env-gated so normal startup is untouched.
    if (qEnvironmentVariableIsSet("TANKOBAN3_LT_SMOKE")) {
        tankoban::tstream::ltLinkSmoke();
        return 0;
    }

    // Phase 3 (throwaway): run the in-process StreamEngine ALONGSIDE the GUI so a
    // curl/mpv can hit it while we confirm the UI stays responsive during a piece
    // wait. Env-gated; kept alive for the whole session in this scope.
    std::unique_ptr<tankoban::tstream::StreamEngine> streamEngine;
    if (qEnvironmentVariableIsSet("TANKOBAN3_STREAM_SMOKE")) {
        const QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                              + QStringLiteral("/torrent-stream");
        QDir().mkpath(cache);
        streamEngine = std::make_unique<tankoban::tstream::StreamEngine>(cache.toStdString());
        if (streamEngine->start()) {
            const std::string url = streamEngine->streamUrl(
                "08ada5a7a6183aae1e09d831df6748d566095a10", -1, {});
            std::fprintf(stdout, "STREAM SMOKE URL: %s\n", url.c_str());
            std::fflush(stdout);
        }
    }

    // Plan 1 — standalone player demo: TANKOBAN3_PLAYER_DEMO=<url|path> ("1" = default sample).
    if (qEnvironmentVariableIsSet("TANKOBAN3_PLAYER_DEMO")) {
        QString src = qEnvironmentVariable("TANKOBAN3_PLAYER_DEMO");
        if (src.isEmpty() || src == QStringLiteral("1"))
            src = QStringLiteral("https://download.blender.org/peach/bigbuckbunny_movies/BigBuckBunny_320x180.mp4");
        auto* view = new PlayerView();
        view->setWindowTitle(QStringLiteral("Tankoban 3 — player"));
        view->setWindowFlag(Qt::FramelessWindowHint);   // no OS chrome -> no fullscreen flash
        view->setGeometry(demoWindowedRect(QGuiApplication::primaryScreen()->availableGeometry()));
        view->setTitleInfo(demoTitleFromSource(src));
        // PlayerView no longer self-closes; in the standalone demo, Back/Esc closes
        // this (top-level) window, which quits the app as before.
        QObject::connect(view, &PlayerView::backRequested, view, &QWidget::close);
        view->show();
        view->play(src);
        // Demo/test affordance: TANKOBAN3_PLAYER_DEMO_SUB=<sid> selects a subtitle
        // track shortly after load, so embedded subs can be eyeballed without the menu.
        const QString demoSub = qEnvironmentVariable("TANKOBAN3_PLAYER_DEMO_SUB");
        if (!demoSub.isEmpty()) {
            QTimer::singleShot(1800, view, [view, demoSub] {
                if (view->controller()) view->controller()->setSubtitleTrack(demoSub);
            });
        }
        return app.exec();
    }

    tankoban::MainWindow window;
    window.showMaximized();

    return app.exec();
}

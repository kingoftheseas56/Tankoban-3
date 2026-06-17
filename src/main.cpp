// Tankoban 3 — entry point.
//
// A native C++/Qt6 app: the lean, faithful recreation of Harbor's video mode, built
// incrementally and later customized into our own. Step 1 was the empty window;
// Step 2 brings the shell (Harbor sidebar + dark content area).

#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "player/PlayerView.h"
#include "engine/MpvController.h"

#include <QApplication>
#include <QFileInfo>
#include <QScreen>
#include <QTimer>
#include <QUrl>

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

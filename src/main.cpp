// Tankoban 3 — entry point.
//
// A native C++/Qt6 app: the lean, faithful recreation of Harbor's video mode, built
// incrementally and later customized into our own. Step 1 was the empty window;
// Step 2 brings the shell (Harbor sidebar + dark content area).

#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "player/PlayerView.h"

#include <QApplication>
#include <QScreen>

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
        view->resize(1280, 720);
        view->move(QGuiApplication::primaryScreen()->availableGeometry().center() - QPoint(640, 360));
        view->show();
        view->play(src);
        return app.exec();
    }

    tankoban::MainWindow window;
    window.showMaximized();

    return app.exec();
}

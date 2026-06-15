// Tankoban 3 — entry point.
//
// A native C++/Qt6 app: the lean, faithful recreation of Harbor's video mode, built
// incrementally and later customized into our own. Step 1 was the empty window;
// Step 2 brings the shell (Harbor sidebar + dark content area).

#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "engine/MpvController.h"
#include "engine/MpvGlWidget.h"

#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tankoban 3"));
    app.setOrganizationName(QStringLiteral("Tankoban"));
    app.setStyleSheet(tankoban::appStyleSheet());

    // Plan 1 Task 4 — temporary render smoke: video into a bare QOpenGLWidget.
    if (qEnvironmentVariableIsSet("TANKOBAN3_RENDER_SMOKE")) {
        QString src = qEnvironmentVariable("TANKOBAN3_RENDER_SMOKE");
        if (src.isEmpty() || src == QStringLiteral("1"))
            src = QStringLiteral("https://download.blender.org/peach/bigbuckbunny_movies/BigBuckBunny_320x180.mp4");
        auto* ctrl = new MpvController();
        ctrl->initialize();
        auto* w = new MpvGlWidget(ctrl);
        w->setWindowTitle(QStringLiteral("Tankoban 3 — render smoke"));
        w->resize(960, 540);
        w->show();
        ctrl->load(src);
        return app.exec();
    }

    tankoban::MainWindow window;
    window.show();

    return app.exec();
}

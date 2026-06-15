// Tankoban 3 — entry point.
//
// A native C++/Qt6 app: the lean, faithful recreation of Harbor's video mode, built
// incrementally and later customized into our own. Step 1 was the empty window;
// Step 2 brings the shell (Harbor sidebar + dark content area).

#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tankoban 3"));
    app.setOrganizationName(QStringLiteral("Tankoban"));
    app.setStyleSheet(tankoban::appStyleSheet());

    tankoban::MainWindow window;
    window.show();

    return app.exec();
}

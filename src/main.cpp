// Tankoban 3 — Step 1: the foundation window.
//
// A native C++/Qt6 app: the lean, faithful recreation of Harbor's video mode,
// built incrementally and later customized into our own. This first step is just
// the empty shell that proves the toolchain compiles and a window opens. The real
// shell (Harbor sidebar + chrome), the addon engine, and the player come next, one
// step at a time.

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tankoban 3"));
    app.setOrganizationName(QStringLiteral("Tankoban"));

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("Tankoban 3"));
    window.resize(1280, 800);

    auto* central = new QWidget(&window);
    central->setStyleSheet(QStringLiteral("background-color: #121317;")); // Harbor canvas (OKLCH 0.18 / 260)

    auto* layout = new QVBoxLayout(central);

    auto* wordmark = new QLabel(QStringLiteral("Tankoban 3"));
    wordmark->setAlignment(Qt::AlignCenter);
    wordmark->setStyleSheet(QStringLiteral(
        "color: #e8b923; font-family: 'Georgia', serif; font-size: 52px; font-weight: 600;"));

    auto* subtitle = new QLabel(QStringLiteral("the foundation — step 1"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(QStringLiteral(
        "color: #aab1bd; font-family: sans-serif; font-size: 15px; letter-spacing: 1px;"));

    layout->addStretch();
    layout->addWidget(wordmark);
    layout->addSpacing(8);
    layout->addWidget(subtitle);
    layout->addStretch();

    window.setCentralWidget(central);
    window.show();

    return app.exec();
}

#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OCTOS");
    app.setApplicationVersion("0.3.0");

    // Set application icon
    QIcon appIcon(":/assets/OCTOS.png");
    app.setWindowIcon(appIcon);

    MainWindow window;
    window.setWindowIcon(appIcon);
    window.resize(1600, 900);
    window.show();

    return app.exec();
}

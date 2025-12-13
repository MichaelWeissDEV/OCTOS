# 1 "./src/main.cpp"
#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OCTOS");
    app.setApplicationVersion("1.0");

    // Set application icon
    QIcon appIcon(":/assets/OCTOS.png");
    app.setWindowIcon(appIcon);

    MainWindow window;
    window.setWindowTitle("OCTOS - Offline Compiler Tool / Optimized Assembly Scrutinizer");
    window.setWindowIcon(appIcon);
    window.resize(1600, 900);
    window.show();

    return app.exec();
}

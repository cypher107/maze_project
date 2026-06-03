#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Maze Solver");
    app.setApplicationVersion("2.0");

    MainWindow window;
    window.show();

    return app.exec();
}

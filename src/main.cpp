#include <QApplication>
#include "main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VE");
    app.setOrganizationName("VE");
    app.setApplicationVersion("0.1.0");

    MainWindow window;
    window.show();

    return app.exec();
}

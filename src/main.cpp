#include <QApplication>
#include <QIcon>
#include "petwindow.h"

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    // Tell Qt where platform plugin is (bypasses dir enumeration for Enigma Virtual Box)
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
        (QCoreApplication::applicationDirPath() + "/platforms").toUtf8());
    QApplication app(argc, argv);
    app.addLibraryPath(QApplication::applicationDirPath());
    app.setWindowIcon(QIcon(":/icon.png"));
    app.setQuitOnLastWindowClosed(false);
    DesktopPet pet;
    return app.exec();
}

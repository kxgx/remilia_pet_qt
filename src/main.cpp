#include <QApplication>
#include <QIcon>
#include "petwindow.h"
#ifdef Q_OS_MAC
#include <objc/objc.h>
#include <objc/message.h>
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    QApplication app(argc, argv);
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
        (QCoreApplication::applicationDirPath() + "/platforms").toUtf8());
    app.addLibraryPath(QApplication::applicationDirPath());
    app.setWindowIcon(QIcon(":/icon.png"));
    app.setQuitOnLastWindowClosed(false);

#ifdef Q_OS_MAC
    // Hide Dock icon, keep menu bar tray
    Class nsapp = (Class)objc_getClass("NSApplication");
    if (nsapp) {
        id sharedApp = ((id (*)(Class, SEL))objc_msgSend)(nsapp, sel_registerName("sharedApplication"));
        // NSApplicationActivationPolicyAccessory = 1
        ((void (*)(id, SEL, NSInteger))objc_msgSend)(sharedApp, sel_registerName("setActivationPolicy:"), 1);
    }
#endif

    DesktopPet pet;
    return app.exec();
}

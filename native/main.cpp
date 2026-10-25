#include "mainwindow.h"
#include "platform.h"
#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc,char **argv) {
    QApplication app(argc,argv);
    QCoreApplication::setApplicationName("Localer"); QCoreApplication::setOrganizationName("Localer"); QCoreApplication::setApplicationVersion(LOCALER_VERSION);
    QApplication::setStyle("Fusion");
    const auto data=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(data);
    QLockFile lock(QDir(data).filePath("desktop.lock")); lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) { QMessageBox::information(nullptr,"Localer","Localer zaten açık. Açık pencereyi kullanabilirsin."); return 0; }
    Engine engine(QDir(QCoreApplication::applicationDirPath()).filePath("easytier"));
    MainWindow window(&engine,Platform::isAdministrator());
    QObject::connect(&app,&QCoreApplication::aboutToQuit,&engine,&Engine::stop);
    window.show();
    return app.exec();
}

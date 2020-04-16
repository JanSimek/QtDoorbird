#include "mainwindow.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Simek.xyz");
    QCoreApplication::setOrganizationDomain("github.com/JanSimek/QtDoorbird");
    QCoreApplication::setApplicationName("Doorbird Doorbell");

    MainWindow w;
    w.show();

    return a.exec();
}

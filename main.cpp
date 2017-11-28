#include "mainwindow.h"
#include <QApplication>
#include <QDesktopWidget>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    /* Place the windows in the middle of the screen*/
    w.move(QApplication::desktop()->screen()->rect().center() - w.rect().center());

    return a.exec();
}

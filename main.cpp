#include "DesktopOrganizer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DesktopOrganizer w;
    w.show();
    return a.exec();
}

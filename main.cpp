#include "DesktopOrganizer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    std::cout << UTF8ToANSI("一共有") << argc << UTF8ToANSI("个参数，参数列表：") << std::endl;
    for (int i = 0;i<argc;i++)
    std::cout << argv[i] << "\n\n";
    DesktopOrganizer w;
    w.show();
    return a.exec();
}

#include "DesktopOrganizer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DesktopOrganizer w;
    w.show();
    return a.exec();
}

//通过扩展名获取注册表执行COMMAND
//#include <iostream>
//#include <windows.h>
//#include <shlwapi.h>
//
//#pragma comment(lib, "shlwapi.lib") // 链接 SHLWAPI 库
//
//int main() {
//    std::wcout.imbue(std::locale("chs"));
//
//    // 文件扩展名
//    const wchar_t* extension = L".docx";
//    // 获取默认打开程序的路径
//    wchar_t* programPath = nullptr;
//    DWORD co = 1024;
//    HRESULT hr = AssocQueryString(ASSOCF_NONE, ASSOCSTR_COMMAND, extension, NULL, programPath, &co);
//    programPath = new wchar_t[co];
//    hr = AssocQueryString(ASSOCF_NONE, ASSOCSTR_COMMAND, extension, NULL, programPath, &co);
//    if (SUCCEEDED(hr))
//    {
//        // 输出结果
//        std::wcout << L"文件扩展名: " << extension << std::endl;
//        //std::wcout << L"文件类型: " << fileType << std::endl;
//        std::wcout << L"默认打开程序路径: " << programPath << std::endl;
//    }
//
//    return 0;
//}
// 测试
//#include <QCoreApplication>
//#include <QRegularExpression>
//#include <QDebug>
//#include <QStringList>
//
//int main(int argc, char* argv[])
//{
//    QCoreApplication app(argc, argv);
//
//    QString b = "%v";
//    QString first_char = QRegularExpression::escape(b.front()); // 转义第一个字符
//    QString escaped_b = QRegularExpression::escape(b);         // 转义整个字符串
//
//    // 构建正则表达式
//    QString pattern = QString("(?<!%1)%2|(?<=(?:%1%1)+)%2")
//        .arg(first_char, escaped_b);
//
//    QRegularExpression regex(pattern);
//    if (!regex.isValid()) {
//        qDebug() << "无效正则表达式:" << regex.errorString();
//        return -1;
//    }
//
//    QString text = "aaasd hjs%vdjsaia%%vsas%%%vsod %%%%%%vsa d%%%%v";
//    QRegularExpressionMatchIterator it = regex.globalMatch(text);
//    QStringList matches;
//    while (it.hasNext()) matches << it.next().captured(0);
//
//    qDebug() << "匹配结果:" << matches; // 输出: ("abc", "abc", "abc", "abc")
//    return 0;
//}
#include "DesktopOrganizer.h"
#include <QtWidgets/QApplication>
// DataBase
#include "stringProcess.h"
#include ".\\lib\\SQLite\\sqlite3.h"
#include <iostream>

static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}
/*
数据库结构：
id tableName title cx cy width height
id: 窗口id
tableName: 数据表名称
title: 窗口名字
cx: x坐标
cy: y坐标
width: 窗口宽度
height: 窗口高度

数据表结构：
name path position
name: fileName文件名
path: filePath文件路径
position: itemPosition控件位置
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //SQLite数据库
    sqlite3* pDataBase = nullptr;//数据库
    int code = sqlite3_open("config.db", &pDataBase);//打开数据库
    if (!code)
    {
        /* Create SQL statement */
        const char* sql = "CREATE TABLE IF NOT EXISTS WindowsManager("  \
            "id INT PRIMARY KEY NOT NULL," \
            "tableName TEXT NOT NULL,"
            "title TEXT NOT NULL" \
            "); ";
        /* Execute SQL statement */
        char* zErrMsg = nullptr;
        code = sqlite3_exec(pDataBase, sql, callback, 0, &zErrMsg);
        if (code != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            MessageBox(0, L"Database open error.\nClick to exit.\n打开数据库失败，点击确定退出。", L"error", MB_ICONERROR);
            return code;
        }
        sqlite3_close(pDataBase);
    }
    else
    {
        MessageBox(0, L"Database open error.\nClick to exit.\n打开数据库失败，点击确定退出。", L"error", MB_ICONERROR);
        return code;
    }
    DesktopOrganizer w;
    w.setSQLite3Database(pDataBase);
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
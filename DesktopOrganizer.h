#pragma once

//Qt
#include <QtWidgets/QMainWindow>
#include "ui_DesktopOrganizer.h"
#include <QPainter>
#include <QScreen>
#include <QMessageBox>
//#include <QFileSystemModel>
//#include <QListView>
#include <QListWidget>
#include "MyFileListWidget.h"
#include <qlabel.h>
#include <QPushButton>
#include "MyFileListItem.h"


//Windows
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")

//VSC++
#include <iostream>
#include <thread>
#include <tchar.h>
#include <strsafe.h>
//#include <filesystem>//还是不用这个库吧

//我的函数My functions
std::string UTF8ToANSI(std::string utf8Text);

class DesktopOrganizer : public QMainWindow
{
    Q_OBJECT

public:
    DesktopOrganizer(QWidget *parent = nullptr);
    ~DesktopOrganizer();
    void paintEvent(QPaintEvent* e) override;
public slots:
private:
    Ui::DesktopOrganizerClass ui;
};

#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DesktopOrganizer.h"
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

class DesktopOrganizer : public QMainWindow
{
    Q_OBJECT

public:
    DesktopOrganizer(QWidget *parent = nullptr);
    ~DesktopOrganizer();
protected:
    void paintEvent(QPaintEvent* e) override;

private:
    Ui::DesktopOrganizerClass ui;
};

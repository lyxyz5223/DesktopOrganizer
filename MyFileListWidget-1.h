#pragma once
#include <qlistwidget.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <QScreen>
#include "MyFileListItem.h"

class MyFileListWidget : public QWidget
{
    Q_OBJECT
public:

    ~MyFileListWidget() {};
    MyFileListWidget(QWidget* parent = nullptr);
    
    void paintEvent(QPaintEvent* e) override;
    void addItem(MyFileListItem* item, std::string id);
    void deleteItem(std::string id);
    void setViewMode(MyFileListItem::ViewMode View_Mode) {
        viewMode = View_Mode;
    }
    long long getTotalLinesNumber() {
        return TotalLinesNumber;
    }
    long long getTotalRowsNumber() {
        return TotalRowsNumber;
    }
    void setItemSize(std::string id, QSize size)
    {

    }
    MyFileListItem* getItemById(std::string id);
private:
    //struct itemInformation
    //{
    //    QWidget* item;
    //    long long x;
    //    long long y;
    //};
    long long TotalLinesNumber = 0;
    long long TotalRowsNumber = 0;
    long long VerticalSpacing = 10;
    long long HorizontalSpacing = 10;

    MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::List;
    std::map<std::string/*id*/, MyFileListItem*> itemsList;
    std::map <std::string, intptr_t> XCoordinate;
    std::map <std::string,intptr_t> YCoordinate;
};



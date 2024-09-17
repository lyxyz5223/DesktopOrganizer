#pragma once
#include <qwidget.h>
#include "MyFileListItem.h"
#include <map>
class MyFileListWidget : public QWidget
{
public:
	~MyFileListWidget() {}
	MyFileListWidget(QWidget* parent = nullptr);
	void setViewMode(MyFileListItem::ViewMode vm) {
		viewMode = vm;
	}
	void addItem(MyFileListItem* item, std::string id);
	void setConfigFileName(std::string configFileName){
		configText = configString;
	}
private:
	typedef long long llong;
	std::string configText;
	std::string configFileName;
	std::unordered_map<std::string,std::string> configMap;
	llong horizontalSpacing;
	llong verticalSpacing;
	llong latticeWidth, latticeHeight,latticeVerticalNum,latticeHorizontalNum;
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;
	std::map<std::string, MyFileListItem*> itemsMap;
	std::map<std::string, llong> Xindex;
	std::map<std::string, llong> Yindex;
	std::map<llong/*Xindex*/, llong> XCoords;
	std::map<llong/*Yindex*/, llong> YCoords;//索引与坐标的对应关系
protected:
	void paintEvent(QPaintEvent* e);
};


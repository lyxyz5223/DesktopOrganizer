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
	void setConfigFileName(std::string File_Name){
		configFileName = File_Name;
	}
private:
	typedef long long llong;
	std::string configText;
	std::string configFileName = "config.ini";
	std::unordered_map<std::string,std::string> configMap;//图标名字与id的绑定
	llong horizontalSpacing = 10;//桌面图标水平间距
	llong verticalSpacing = 10;//桌面图标垂直间距
	llong latticeWidth = 64, //桌面图标宽度
		latticeHeight = 80,//桌面图标高度
		latticeVerticalNum = 0,//桌面图标垂直数量
		latticeHorizontalNum = 0;//桌面图标水平数量
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;
	std::map<std::string, MyFileListItem*> itemsMap;//id与物体的绑定
	std::map<std::string, llong> Xindex;//id与X索引
	std::map<std::string, llong> Yindex;//id与Y索引
	std::map<llong/*Xindex*/, llong> XCoords;//索引与x坐标的对应关系
	std::map<llong/*Yindex*/, llong> YCoords;//索引与y坐标的对应关系
protected:
	void paintEvent(QPaintEvent* e);
};
/*
配置文件格式
桌面图标名字 id Xindex Yindex
*/

#pragma once
#include <qwidget.h>
#include "MyFileListItem.h"
#include <map>
class MyFileListWidget : public QWidget
{
public:
	~MyFileListWidget() {}
	//MyFileListWidget(QWidget* parent = nullptr);
	MyFileListWidget(QWidget* parent = nullptr,QString path = "");
	void setViewMode(MyFileListItem::ViewMode vm) {
		viewMode = vm;
	}
	void addItem(MyFileListItem* item, std::string id);
	void setConfigFileName(std::string File_Name){
		configFileName = File_Name;
	}
public slots:
	void desktopItemProc(std::wstring name);

private:
	typedef long long llong;
	struct ItemProp {
		MyFileListItem* item;
		std::string filename;
		llong id;
		llong xIndex;
		llong yIndex;
	};
	struct lPoint {
		llong x;
		llong y;
	};
	std::string strConfig = "";
	std::string configFileName = "config.ini";
	llong horizontalSpacing = 10;//桌面图标水平间距
	llong verticalSpacing = 10;//桌面图标垂直间距
	llong latticeWidth = 0, //桌面图标宽度
		latticeHeight = 0,//桌面图标高度
		latticeVerticalNum = 0,//桌面图标垂直数量
		latticeHorizontalNum = 0;//桌面图标水平数量
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;
	std::map<std::string/*id*/, ItemProp> itemsMap;
	std::map<llong/*x或者y索引*/, lPoint> indexToCoord;//index索引与坐标

	//std::unordered_map<std::string/*name*/, std::string/*id*/> configMap;//图标名字与id的绑定
	//std::map<std::string, MyFileListItem*> itemsMap;//id与物体的绑定
	//std::map<std::string, llong> Xindex;//id与X索引
	//std::map<std::string, llong> Yindex;//id与Y索引Yindex[id] = index
	//std::map<std::string, lPoint> index;//id与索引
	//std::map<llong/*Xindex*/, llong> XCoords;//索引与x坐标的对应关系
	//std::map<llong/*Yindex*/, llong> YCoords;//索引与y坐标的对应关系
	bool writeConfig(std::map<std::string/*id*/, ItemProp> config_map, std::string 分隔符 = "\t");
protected:
	void mousePressEvent(QMouseEvent* e);
	void paintEvent(QPaintEvent* e);
};
/*
配置文件格式
桌面图标名字 id Xindex Yindex
*/

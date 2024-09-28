#pragma once
#include <qwidget.h>
#include "MyFileListItem.h"
#include <map>
class MyFileListWidget : public QWidget
{
	Q_OBJECT
public:
	~MyFileListWidget() {}
	//MyFileListWidget(QWidget* parent = nullptr);
	MyFileListWidget(QWidget* parent = nullptr,QString path = "");
	void initialize();
	void resize(int w, int h) {
		resize(QSize(w,h));
	}
	void resize(const QSize&size) {
		QWidget::resize(size);
		initialize();
	}
	void resizeZero()
	{
		QWidget::resize(0, 0);
	}
	void setViewMode(MyFileListItem::ViewMode vm) {
		viewMode = vm;
	}
	void addItem(MyFileListItem* item/*, std::string id*/,std::string nameWithPath);
	void setConfigFileName(std::string File_Name){
		configFileName = File_Name;
	}
	void threadReadDirectoryChangesProc(std::wstring path);
	void threadCheckFilesChange();

	typedef long long llong;
	struct ItemProp {
		MyFileListItem* item;
		std::string filename;//WithPath!!!
		//llong id;
		llong xIndex;
		llong yIndex;
	};
	struct lPoint {
		llong x;
		llong y;
	};
	llong horizontalSpacing = 20;//桌面图标水平间距
	llong verticalSpacing = 15;//桌面图标垂直间距
	llong firstVerticalSpacing = verticalSpacing;
	llong firstHorizontalSpacing = horizontalSpacing;
	llong latticeWidth = 0, //桌面图标宽度
		latticeHeight = 0,//桌面图标高度
		latticeVerticalNum = 0,//桌面图标垂直数量
		latticeHorizontalNum = 0;//桌面图标水平数量

	//llong deletedCount = 0;//删除计数
	//llong lastXindex = 1;
	//llong lastYindex = 1;
	std::map<std::string/*nameWithPath*/, ItemProp> itemsMap;
	//std::map<std::string/*nameWithPath*/, std::string/*id*/> idMap;
	std::map<std::pair<llong,llong>/**/, bool> latticeJudge;
	void SendCreateItemSignal(std::wstring name, std::wstring path)
	{
		emit createItem(name, path);
	}
	void SendDeleteItemSignal(std::wstring name, std::wstring path)
	{
		emit deleteItem(name,path);
	}
signals://信号
	void createItem(std::wstring, std::wstring);
	void deleteItem(std::wstring, std::wstring);

public slots://槽
	void desktopItemProc(std::wstring nameWithPath);
	void CreateItem(std::wstring name, std::wstring path);
	void DeleteItem(std::wstring name, std::wstring path);

private:
	bool delete_ing = false;
	std::string strConfig = "";
	std::string configFileName = "config.ini";
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;
	std::map<llong/*x或者y索引*/, lPoint> indexToCoord;//index索引与坐标

	//std::unordered_map<std::string/*name*/, std::string/*id*/> configMap;//图标名字与id的绑定
	//std::map<std::string, MyFileListItem*> itemsMap;//id与物体的绑定
	//std::map<std::string, llong> Xindex;//id与X索引
	//std::map<std::string, llong> Yindex;//id与Y索引Yindex[id] = index
	//std::map<std::string, lPoint> index;//id与索引
	//std::map<llong/*Xindex*/, llong> XCoords;//索引与x坐标的对应关系
	//std::map<llong/*Yindex*/, llong> YCoords;//索引与y坐标的对应关系
	bool writeConfig(std::map<std::string/*nameWithPath*/, ItemProp> config_map, std::string 分隔符 = "\t");
protected:
	void mousePressEvent(QMouseEvent* e);
	void paintEvent(QPaintEvent* e);
};
/*
配置文件格式
桌面图标名字 id Xindex Yindex
*/

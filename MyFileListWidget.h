#pragma once
#include <qwidget.h>
#include "MyFileListItem.h"
#include "SelectionArea.h"
#include <map>
#include <unordered_map>
#include <set>

class MyFileListWidget : public QWidget
{
	Q_OBJECT
public:
	~MyFileListWidget() {}
	//MyFileListWidget(QWidget* parent = nullptr);
	MyFileListWidget(QWidget* parent,
		std::vector<std::wstring> pathsList,
		std::wstring config);

	void setViewMode(MyFileListItem::ViewMode mode) {
		viewMode = mode;
	}
	std::vector<std::wstring> splitForConfig(std::wstring text, std::wstring delimiter = L" "/*separator,分隔符*/, std::wstring EscapeString = L"" /*char EscapeCharacter*/);
	bool readConfigFile(std::wstring nameWithPath);
	void addPath(std::wstring path) {
		pathsList.push_back(path);
	}
	bool writeConfigFile(std::wstring nameWithPath);
	void checkFilesChangeProc(std::wstring path);
	void sendCreateItemSignal(std::wstring name, std::wstring path) {
		emit createItemSignal(name, path);
	}
	void sendRemoveItemSignal(std::wstring name, std::wstring path) {
		emit removeItemSignal(name, path);
	}
signals:
	void createItemSignal(std::wstring name, std::wstring path);
	void removeItemSignal(std::wstring name, std::wstring path);
public slots:
	void createItem(std::wstring name, std::wstring path);
	void removeItem(std::wstring name, std::wstring path);

protected:
	void paintEvent(QPaintEvent* e) override;
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	struct ItemProp {
		MyFileListItem* item = 0;//Qt Item
		std::wstring name = L"";//No path
		std::wstring path = L"";
		long long position = -1;// 计算后可得到具体坐标
	};
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;
	std::unordered_map<std::wstring/*name with path*/, ItemProp> itemsMap;// 文件列表
	std::vector<std::wstring> pathsList;// 文件路径列表
	std::wstring indexesState = L"0";// 按列计算的索引状态，1表示已占用，0表示未占用
	std::wstring configFileNameWithPath;
	QFont itemFont;
	struct Spacing {
		int line;//行间隔
		int column;//列间隔
	} itemSpacing = { 15, 15 };

	//临时
	int zoomScreen = 10;//item的高度是屏幕高度/宽度中小的那个的1/zoomScreen倍
	void changeItemSizeAndNumbersPerColumn();
	bool isRemovingItem = false;
	bool isCreatingItem = false;
	QRect selectionRect;// 框选区域
	SelectionArea* selectionArea;// 框选区域图形

	size_t itemsNumPerColumn = 0;
	QSize itemSize;

};

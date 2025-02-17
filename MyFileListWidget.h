#pragma once
#include <qwidget.h>
#include "MyFileListItem.h"
#include "SelectionArea.h"
#include "GrabArea.h"

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
		isCreatingItem = true;
		emit createItemSignal(name, path);
	}
	void sendRemoveItemSignal(std::wstring name, std::wstring path) {
		isRemovingItem = true;
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
	void dragEnterEvent(QDragEnterEvent* e) override;
	void dragMoveEvent(QDragMoveEvent* e) override;
	void dragLeaveEvent(QDragLeaveEvent* e) override;
	void dropEvent(QDropEvent* e) override;

private:
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;
	std::unordered_map<std::wstring/*name with path*/, ItemProp> itemsMap;// 文件列表
	std::vector<std::wstring> pathsList;// 文件路径列表
	std::wstring indexesState = L"0";// 按列计算的索引状态，1表示已占用，0表示未占用
	std::wstring configFileNameWithPath;
	QFont itemFont;
	Spacing itemSpacing = { 15, 15 };

	//临时
	int zoomScreen = 10;//item的高度是屏幕高度/宽度中小的那个的1/zoomScreen倍
	void changeItemSizeAndNumbersPerColumn();
	bool isRemovingItem = false;
	bool isCreatingItem = false;
	QRect selectionRect;// 框选区域
	SelectionArea* selectionArea = new SelectionArea(this);// 框选区域图形
	size_t itemsNumPerColumn = 0;
	QSize itemSize;
	GrabArea* grabArea = new GrabArea(this, itemsNumPerColumn, itemSize, itemSpacing);// 选中文件拖动区域



};

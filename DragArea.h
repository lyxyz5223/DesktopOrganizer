#pragma once
#include <qwidget.h>
#include <map>
#include <queue>
#include <deque>
#include <qdrag.h>
#include "ItemProperty.h"
#include <thread>
#include <Windows.h>
#include <iostream>

QPoint relativePosTransition(QWidget* from, QPoint fromPos, QWidget* to);
QPoint relativePosTransition(QRect from, QPoint fromPos, QRect to);

class DragArea : public QWidget
{
	Q_OBJECT
public:
	struct ItemWithPosition {
		void* item;
		ItemProp originalItemProp;
		QRect originalGeometry;
	};

private:

	std::unordered_map<std::wstring, ItemWithPosition> children_map;
	std::vector<std::wstring> children_keys;

	std::vector<int> leftMost;
	std::vector<int> rightMost;
	std::vector<int> topMost;
	std::vector<int> bottomMost;

	size_t& itemsNumPerColumn;
	QSize& itemSize;
	Spacing& itemSpacing;
	QWidget* parentWidget = nullptr;
	QRect goalGeometry = QRect(0, 0, 0, 0);
	void ChangeGoalGeometry();

	QPoint cursorPosOffsetWhenMousePress;

	//void checkCursorPosChange();

public:
	~DragArea() {}
	DragArea(QWidget* parent, size_t& ItemsNumPerColumn, QSize& ItemSize, Spacing& ItemSpacing);
	void removeItem(std::wstring name, std::wstring path);
	void addItem(ItemProp ip);
	void show() {
		//方法1
		// QRect的right可能小于left，bottom可能小于top，这里排除这种可能
		//QRect g;
		//g.setSize(QSize(
		//	(goalGeometry.width() > 0 ? goalGeometry.width() : -goalGeometry.width()),
		//	(goalGeometry.height() > 0 ? goalGeometry.height() : -goalGeometry.height())
		//));
		//g.moveTo(QPoint(
		//	(goalGeometry.left() < goalGeometry.right() ? goalGeometry.left() : goalGeometry.right()),
		//	(goalGeometry.top() < goalGeometry.bottom() ? goalGeometry.top() : goalGeometry.bottom())
		//	));
		//move(g.topLeft());
		//resize(g.size());

		//方法2
		//move(goalGeometry.topLeft());
		//resize(goalGeometry.size());
		QWidget::show();
	}
	[[deprecated]] void move(QPoint pos) {
		QWidget::move(pos);
	}
	[[deprecated]] void move(int x, int y) {
		// move(QPoint(x, y));
	}
	void moveRelative(QPoint pos, QWidget* from, QWidget* to) {
		pos = relativePosTransition(from, pos, to); // 相对坐标-->绝对坐标
		QWidget::move(pos);
	}
	void moveAbsolute(QPoint pos) {
		QWidget::move(pos);
	}
	void correctPosition() {
		moveRelative(goalGeometry.topLeft(), parentWidget, nullptr);
	}

	void hide() {
		QWidget::hide();
	}

	inline Spacing getItemSpacing() const {
		return itemSpacing;
	}
	inline std::unordered_map<std::wstring, ItemWithPosition> getSelectedItems() const {
		return children_map;
	}
	inline std::vector<std::wstring> getSelectedItemsKeys() const {
		return children_keys;
	}
	inline QPoint getCursorPosOffsetWhenMousePress() const {
		return cursorPosOffsetWhenMousePress;
	}
	inline void setCursorPosOffsetWhenMousePress(QPoint CursorPosOffsetWhenMousePress) {
		cursorPosOffsetWhenMousePress = CursorPosOffsetWhenMousePress;
	}

protected:
	void paintEvent(QPaintEvent* e);

signals:
	void hideSignal();

public slots:
	//void moveSlot(QPoint pos) {
	//	move(pos);
	//}



};


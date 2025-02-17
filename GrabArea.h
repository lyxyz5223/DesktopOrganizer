#pragma once
#include <qwidget.h>
#include <map>
#include <queue>
#include <deque>
#include <qdrag.h>
#include "ItemProperty.h"

class GrabArea : public QWidget
{
public:
	~GrabArea() {}
	GrabArea(QWidget* parent, size_t& ItemsNumPerColumn, QSize& ItemSize, Spacing& ItemSpacing);
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
	void hide() {
		move(goalGeometry.topLeft());
		QWidget::hide();
	}

	inline Spacing getItemSpacing() const {
		return itemSpacing;
	}
	struct ItemWithPosition {
		void* item;
		ItemProp originalItemProp;
		QRect originalGeometry;
	};
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
};


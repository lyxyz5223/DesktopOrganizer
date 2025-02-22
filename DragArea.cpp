#include "DragArea.h"
#include "MyFileListItem.h"
#include <qpainter.h>
#include <algorithm>
#include <iostream>
#include <QUrl>
#include <QScreen>

extern bool PathCompletion(std::wstring& path);
QPoint relativePosTransition(QWidget* from, QPoint fromPos, QWidget* to)
{
	if ((!from) && (!to))
		return fromPos;
	QPoint fp, tp;
	fp = (from) ? from->geometry().topLeft() : to->screen()->availableGeometry().topLeft();
	tp = (to) ? to->geometry().topLeft() : from->screen()->availableGeometry().topLeft();

	//fromPos + fp - tp
	return QPoint(
		fromPos.x() + fp.x() - tp.x(),
		fromPos.y() + fp.y() - tp.y()
	);
}
QPoint relativePosTransition(QRect from, QPoint fromPos, QRect to)
{
	auto fp = from.topLeft();
	auto tp = to.topLeft();
	//fromPos + fp - tp
	return QPoint(
		fromPos.x() + fp.x() - tp.x(),
		fromPos.y() + fp.y() - tp.y()
	);
}

DragArea::DragArea(QWidget* parent, size_t& ItemsNumPerColumn, QSize& ItemSize, Spacing& ItemSpacing)
	: QWidget(nullptr),
	itemsNumPerColumn(ItemsNumPerColumn),
	itemSize(ItemSize),
	itemSpacing(ItemSpacing)
{
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::Tool);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_AlwaysStackOnTop, true);
	setAttribute(Qt::WA_TransparentForMouseEvents, true);
	setAttribute(Qt::WA_InputMethodTransparent, true);
	clearFocus();
	setWindowOpacity(0.6);
	parentWidget = parent;
	QWidget::resize(0, 0);
	QWidget::move(0, 0);
	//connect(this, &DragArea::moveSignal, this, &DragArea::moveSlot);
	connect(this, &DragArea::hideSignal, this, &DragArea::hide);
	//std::thread threadCheckCursorPosChange(&DragArea::checkCursorPosChange, this);
	//threadCheckCursorPosChange.detach();
}


void DragArea::removeItem(std::wstring name, std::wstring path)
{
	PathCompletion(path);
	std::wstring nameWithPath = path + name;
	if (children_map.count(nameWithPath))
	{
		ItemWithPosition& childrenStruct = children_map[nameWithPath];

		auto iter_children_key = std::find(children_keys.begin(), children_keys.end(), nameWithPath);
		if (iter_children_key != children_keys.end())
			children_keys.erase(iter_children_key);

		void* p = childrenStruct.item;
		auto item = static_cast<MyFileListItem*>(p);
		item->hide();

		QRect r = childrenStruct.originalGeometry;
		auto iter = std::find(leftMost.begin(), leftMost.end(), r.left());
		if (iter != leftMost.end())
			leftMost.erase(iter);
		iter = std::find(topMost.begin(), topMost.end(), r.top());
		if (iter != topMost.end())
			topMost.erase(iter);
		iter = std::find(rightMost.begin(), rightMost.end(), r.right());
		if (iter != rightMost.end())
			rightMost.erase(iter);
		iter = std::find(bottomMost.begin(), bottomMost.end(), r.bottom());
		if (iter != bottomMost.end())
			bottomMost.erase(iter);


		item->deleteLater();
		item = nullptr;
		p = nullptr;
		children_map.erase(nameWithPath);
#ifdef _DEBUG
		std::cout << "DragArea: removeItem, current item numbers:" << children_map.size() << std::endl;
#endif // _DEBUG

	}

	ChangeGoalGeometry();
	moveRelative(goalGeometry.topLeft(), parentWidget, nullptr);
	resize(goalGeometry.size());

	for (auto iter = children_map.begin(); iter != children_map.end(); iter++)
	{
		int xIndex = iter->second.originalItemProp.position / itemsNumPerColumn + 1;
		int yIndex = iter->second.originalItemProp.position % itemsNumPerColumn + 1;
		QPoint itemPos = QPoint(
			(xIndex - 1) * (itemSize.width()) + xIndex * itemSpacing.column,
			(yIndex - 1) * (itemSize.height()) + yIndex * itemSpacing.line
		);
		itemPos = relativePosTransition(parentWidget, itemPos, this);
		static_cast<MyFileListItem*>(iter->second.item)->move(itemPos.x(), itemPos.y());
		static_cast<MyFileListItem*>(iter->second.item)->show();
	}

}

void DragArea::addItem(ItemProp ip)
{
	MyFileListItem* bak = static_cast<MyFileListItem*>(ip.item);
	MyFileListItem* item = new MyFileListItem(*bak, true);
	item->setParent(this);
	PathCompletion(ip.path);
	std::wstring nameWithPath = ip.path + ip.name;
	//QPoint tl = relativePosTransition(parentWidget, bak->geometry().topLeft(), nullptr);
	QRect r = bak->geometry();
	//r.moveTo(tl);
	children_map[nameWithPath] = {
		item,
		ip,
		r
	};
	children_keys.push_back(nameWithPath);

#ifdef _DEBUG
	std::cout << "DragArea: addItem, current item numbers:" << children_map.size() << std::endl;
#endif // _DEBUG


	bool (*MaxToMin)(int a, int b) = [](int a, int b) {
		return a > b;
		};
	bool (*MinToMax)(int a, int b) = [](int a, int b) {
		return a < b;
		};

	leftMost.push_back(r.left());
	std::sort(leftMost.begin(), leftMost.end(), MaxToMin);
	topMost.push_back(r.top());
	std::sort(topMost.begin(), topMost.end(), MaxToMin);
	rightMost.push_back(r.right());
	std::sort(rightMost.begin(), rightMost.end(), MinToMax);
	bottomMost.push_back(r.bottom());
	std::sort(bottomMost.begin(), bottomMost.end(), MinToMax);

	ChangeGoalGeometry();
	moveRelative(goalGeometry.topLeft(), parentWidget, nullptr);
	resize(goalGeometry.size());

	//int xIndex = ip.position / itemsNumPerColumn + 1;
	//int yIndex = ip.position % itemsNumPerColumn + 1;
	//QPoint itemPos = QPoint(
	//	(xIndex - 1) * (itemSize.width()) + xIndex * itemSpacing.column,
	//	(yIndex - 1) * (itemSize.height()) + yIndex * itemSpacing.line
	//);
	//itemPos = relativePosTransition(parentWidget, itemPos, this);
	////item->resize(itemSize);
	//item->move(itemPos.x() + 10, itemPos.y() + 10);
	//item->show();
	for (auto iter = children_map.begin(); iter != children_map.end(); iter++)
	{
		int xIndex = iter->second.originalItemProp.position / itemsNumPerColumn + 1;
		int yIndex = iter->second.originalItemProp.position % itemsNumPerColumn + 1;
		QPoint itemPos = QPoint(
			(xIndex - 1) * (itemSize.width()) + xIndex * itemSpacing.column,
			(yIndex - 1) * (itemSize.height()) + yIndex * itemSpacing.line
		);
		itemPos = relativePosTransition(parentWidget, itemPos, this);
		static_cast<MyFileListItem*>(iter->second.item)->move(itemPos.x(), itemPos.y());
		static_cast<MyFileListItem*>(iter->second.item)->show();
	}
}

void DragArea::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	//p.fillRect(rect(), QColor(255, 255, 255, 255));
	QPen pen;
	pen.setColor(QColor(0, 0, 0, 155));
	pen.setWidth(2);
	p.setPen(pen);
	p.drawRect(rect());
	QWidget::paintEvent(e);
}

void DragArea::ChangeGoalGeometry()
{
	if (leftMost.empty() || topMost.empty() || rightMost.empty() || bottomMost.empty())
		leftMost = rightMost = topMost = bottomMost = std::vector<int>();
	else
	{
		goalGeometry.setTopLeft(QPoint(leftMost.back() - itemSpacing.column, topMost.back() - itemSpacing.line));
		goalGeometry.setBottomRight(QPoint(rightMost.back() + itemSpacing.column, bottomMost.back() + itemSpacing.line));
	}
}


//void DragArea::checkCursorPosChange()
//{
//	while (true)
//	{
//		if (this->isHidden())
//			continue;
//
//		QCursor cur;
//		SHORT ks = (GetKeyState(VK_LBUTTON) >> (sizeof(SHORT) * 8 - 1) & 1);
//		if (ks)
//		{
//			QPoint cursorPos(cur.pos());
//			emit moveSignal(QPoint(cursorPos.x() - cursorPosOffsetWhenMousePress.x(), cursorPos.y() - cursorPosOffsetWhenMousePress.y()));
//		}
//		else {
//			emit hideSignal();
//		}
//	}
//}


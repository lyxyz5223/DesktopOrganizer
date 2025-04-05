#include "DragArea.h"
#include <algorithm>
#include <iostream>
#include <QUrl>
#include <QScreen>
#include "stringProcess.h"

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
QPoint relativePosTransition(QPoint from, QPoint fromPos, QPoint to)
{
	//fromPos + fp - tp
	return QPoint(
		fromPos.x() + from.x() - to.x(),
		fromPos.y() + from.y() - to.y()
	);
}



DragArea::DragArea(long long& ItemsNumPerColumn, QSize& ItemSize, Spacing& ItemSpacing)
	: itemsNumPerColumn(ItemsNumPerColumn), itemSize(ItemSize), itemSpacing(ItemSpacing)
{

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
		children_map.erase(nameWithPath);

		//auto iter = std::find(leftMost.begin(), leftMost.end(), childrenStruct.geometryInParent.left());
		//if (iter != leftMost.end())
		//	leftMost.erase(iter);
		//iter = std::find(topMost.begin(), topMost.end(), childrenStruct.geometryInParent.top());
		//if (iter != topMost.end())
		//	topMost.erase(iter);
		//iter = std::find(rightMost.begin(), rightMost.end(), childrenStruct.geometryInParent.right());
		//if (iter != rightMost.end())
		//	rightMost.erase(iter);
		//iter = std::find(bottomMost.begin(), bottomMost.end(), childrenStruct.geometryInParent.bottom());
		//if (iter != bottomMost.end())
		//	bottomMost.erase(iter);

#ifdef _DEBUG
		std::cout << "DragArea: removeItem: " << wstr2str_2ANSI(nameWithPath) << ", current item numbers:" << children_map.size() << std::endl;
#endif // !_DEBUG
	}
}

void DragArea::addItem(ItemWithPosition iwp)
{
	PathCompletion(iwp.path);
	std::wstring nameWithPath = iwp.path + iwp.name;
	children_map[nameWithPath] = {
		iwp.name,
		iwp.path,
		iwp.itemImage,
		iwp.geometryInParent,
		iwp.position
	};
	auto iter = std::find(children_keys.begin(), children_keys.end(), nameWithPath);
	if (iter == children_keys.end())
		children_keys.push_back(nameWithPath);
#ifdef _DEBUG
	std::cout << "DragArea: addItem: " << wstr2str_2ANSI(nameWithPath) << ", current item numbers:" << children_map.size() << std::endl;
#endif // _DEBUG
	//bool (*MaxToMin)(int a, int b) = [](int a, int b) { return a > b; };
	//bool (*MinToMax)(int a, int b) = [](int a, int b) { return a < b; };
	//leftMost.push_back(iwp.geometryInParent.left());
	//std::sort(leftMost.begin(), leftMost.end(), MaxToMin);
	//topMost.push_back(iwp.geometryInParent.top());
	//std::sort(topMost.begin(), topMost.end(), MaxToMin);
	//rightMost.push_back(iwp.geometryInParent.right());
	//std::sort(rightMost.begin(), rightMost.end(), MinToMax);
	//bottomMost.push_back(iwp.geometryInParent.bottom());
	//std::sort(bottomMost.begin(), bottomMost.end(), MinToMax);
}

//void DragArea::refreshGeometry()
//{
//	if (leftMost.empty() || topMost.empty() || rightMost.empty() || bottomMost.empty())
//		leftMost = rightMost = topMost = bottomMost = std::vector<int>();
//	else
//	{
//		geometry.setTopLeft(QPoint(leftMost.back() - itemSpacing.column, topMost.back() - itemSpacing.line));
//		geometry.setBottomRight(QPoint(rightMost.back() + itemSpacing.column, bottomMost.back() + itemSpacing.line));
//	}
//}
//

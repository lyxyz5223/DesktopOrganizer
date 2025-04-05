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
#include <qlist.h>
#include <qdatastream.h>
#include <qiodevice.h>

QPoint relativePosTransition(QWidget* from, QPoint fromPos, QWidget* to);
QPoint relativePosTransition(QRect from, QPoint fromPos, QRect to);
QPoint relativePosTransition(QPoint from, QPoint fromPos, QPoint to);

class DragArea
{
	
public:
	struct ItemWithPosition {
		std::wstring name;
		std::wstring path;
		QImage itemImage;
		QRect geometryInParent;
		long long position;
	};

	static struct CustomData {
		long long position;
		QPoint offset;
		friend QDataStream& operator<< (QDataStream& stream, const CustomData& data) {//序列化
			stream << data.position;
			stream << data.offset;
			return stream;
		}
		friend QDataStream& operator>> (QDataStream& stream, CustomData& data) {//反序列化
			stream >> data.position;
			stream >> data.offset;
			return stream;
		}
	};

	static QByteArray serializeCustomDataList(const QList<CustomData>& data) {
		QByteArray byteArray;
		QDataStream stream(&byteArray, QIODevice::WriteOnly);
		stream << data;
		return byteArray;
	}
	static QList<CustomData> deserializeCustomDataList(const QByteArray& byteArray) {
		QList<CustomData> data;
		QDataStream stream(byteArray);
		stream >> data;
		return data;
	}

private:
	std::unordered_map<std::wstring/*nameWithPath*/, ItemWithPosition> children_map;
	std::vector<std::wstring> children_keys;

	//std::vector<int> leftMost;
	//std::vector<int> rightMost;
	//std::vector<int> topMost;
	//std::vector<int> bottomMost;

	long long& itemsNumPerColumn;
	QSize& itemSize;
	Spacing& itemSpacing;
	QWidget* parentWidget = nullptr;
	//QRect geometry = QRect(0, 0, 0, 0);
	//void refreshGeometry();

public:
	~DragArea() {}
	DragArea(long long& ItemsNumPerColumn, QSize& ItemSize, Spacing& ItemSpacing);
	void removeItem(std::wstring name, std::wstring path);
	void addItem(ItemWithPosition iwp);

	inline std::unordered_map<std::wstring, ItemWithPosition> getSelectedItemsMap() const {
		return children_map;
	}
	inline std::vector<std::wstring> getSelectedItemsKeys() const {
		return children_keys;
	}

protected:

private:

};


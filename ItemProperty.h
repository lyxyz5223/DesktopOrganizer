#pragma once

#include <string>
#include <qdatastream.h>


struct ItemProp {
	long long windowId = 0;//窗口id，决定该item在哪一个window
	void* item = 0;//Qt Item
	std::wstring name = L"";//No path
	std::wstring path = L"";// With '\' at the end
	long long position = -1;// 计算后可得到具体坐标

	friend QDataStream& operator<< (QDataStream& stream, const ItemProp& data) {//序列化
		stream << data.windowId;
		stream << (long long)data.item;
		stream << QString::fromStdWString(data.name);
		stream << QString::fromStdWString(data.path);
		stream << data.position;
		return stream;
	}
	friend QDataStream& operator>> (QDataStream& stream, ItemProp& data) {//序列化
		stream >> data.windowId;
		long long item = 0;
		stream >> item;
		data.item = (void*)item;
		QString name, path;
		stream >> name;
		stream >> path;
		data.name = name.toStdWString();
		data.path = path.toStdWString();
		stream >> data.position;
		return stream;
	}
};
struct Spacing {
	int line;//行间隔
	int column;//列间隔
};

#pragma once

#include <string>
struct ItemProp {
	long long windowId = 0;//窗口id，决定该item在哪一个window
	void* item = 0;//Qt Item
	std::wstring name = L"";//No path
	std::wstring path = L"";// With '\' at the end
	long long position = -1;// 计算后可得到具体坐标
};
struct Spacing {
	int line;//行间隔
	int column;//列间隔
};

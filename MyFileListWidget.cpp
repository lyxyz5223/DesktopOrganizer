#include "MyFileListWidget.h"
#include <QPainter>
#include <fstream>
//#include <sstream>
#include <iostream>
#include <vector>
//#include <array>
#include "stringProcess.h"
#include <cstdlib>

MyFileListWidget::MyFileListWidget(QWidget* parent) : QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground, true);
	using namespace std;
	fstream fConfig(configFileName, ios::app|ios::out);
	if (fConfig.is_open())
	{
		//fConfig << "1.txt 1 1 1" << std::endl;//Test Write
		fConfig.close();
		fConfig.open(configFileName, ios::in);
		if (fConfig.is_open())
		{
			string strTmp;
			//llong linesNum = 0;
			//while (fConfig >> strTmp)
			//	linesNum++;
			//fConfig.clear();
			//fConfig.seekg(0, ios::beg);
			if (fConfig.peek() == ifstream::traits_type::eof())//判断文件是否为空
				goto fileNULL;
			while (getline(fConfig, strTmp))
			{
				strConfig += strTmp + "\n";
				vector<string> configContentVector = split(strTmp);
				if (configContentVector.size() < 4)
				{
					assert(false && "configContentVector.size()<4");
					exit(999);
				}
				configMap[configContentVector.at(0)] = configContentVector.at(1);
				Xindex[configContentVector.at(0)] = std::atoll(configContentVector.at(2).c_str());
				Yindex[configContentVector.at(0)] = std::atoll(configContentVector.at(3).c_str());

			}
			strConfig.erase(strConfig.end()-1);
			fileNULL:
			extern std::string UTF8ToANSI(std::string utf8Text);
			cout << UTF8ToANSI("配置文件内容：\n") << UTF8ToANSI(strConfig) << std::endl;
		}
	}
	
}
//写split的测试代码
//std::vector<std::string> aOK;
//string toDo="123456 789,4444 \\ 55 \\\\ 11111 ";
//vector<string> deli;
//deli.push_back(" ");
//deli.push_back(",");
//aOK = split(toDo,deli);
//for (int i = 0; i < aOK.size(); i++)
//	cout << UTF8ToANSI(aOK[i]+";");

void MyFileListWidget::addItem(MyFileListItem* item, std::string id)
{
	item->setParent(this);
	itemsMap[id] = item;
	item->setViewMode(viewMode);
}

void MyFileListWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(255, 255, 255, 255));
	llong 初始的y坐标 = 0,
		初始的x坐标 = 0;//可以改
	llong yNext = 初始的y坐标,
		xNext = 初始的x坐标,
		rowCount = 1,
		lineCount = 1;
	if (latticeHeight + verticalSpacing != 0)
		latticeVerticalNum = (height() + verticalSpacing) / (latticeHeight + verticalSpacing);
	if (latticeHeight + horizontalSpacing != 0)
		latticeHorizontalNum = (height() + horizontalSpacing) / (latticeHeight + horizontalSpacing);
	latticeHorizontalNum = (latticeHorizontalNum > itemsMap.size() / latticeVerticalNum ? latticeHorizontalNum : itemsMap.size() / latticeVerticalNum);
	for (llong i = 1; i <= latticeVerticalNum; i++,yNext += latticeHeight+verticalSpacing)
		YCoords[i] = yNext;
	for (llong i = 1; i <= latticeHorizontalNum; i++, xNext += latticeWidth+horizontalSpacing)
		XCoords[i] = xNext;
	if (strConfig == "")
	{
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
		{
			Xindex[iter->first] = rowCount;
			Yindex[iter->first] = lineCount;
			lineCount++;
			if (lineCount > latticeVerticalNum)
			{
				lineCount = 1;
				rowCount++;
			}
			int aaa = iter->second->width();
			if (iter->second->width() > latticeWidth)
				latticeWidth = iter->second->width();
			if (iter->second->height() > latticeHeight)
				latticeHeight = iter->second->height();
		}
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
		{
			llong ind = Yindex[iter->first];
			ind = YCoords[ind];
			iter->second->move(XCoords[Xindex[iter->first]], YCoords[Yindex[iter->first]]);
			//iter->second->resize(latticeWidth,latticeHeight);
			//iter->second->show();
		}
	}
	QWidget::paintEvent(e);
}
/*
	for (llong i = 1; i <= latticeVerticalNum; i++)
	{
		if (yNext + latticeHeight > height())
		{
			yNext = 初始的y坐标;
			rowCount++;
			xNext += latticeWidth + horizontalSpacing;
			XCoords[rowCount] = xNext;//第rowCount列的x坐标
		}
		YCoords[i] = yNext;//第i行的y坐标
		yNext += latticeHeight + verticalSpacing;

	}
*/


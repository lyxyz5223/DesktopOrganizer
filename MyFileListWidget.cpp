#include "MyFileListWidget.h"
#include <QPainter>
#include <fstream>
//#include <sstream>
#include <iostream>
#include <vector>
//#include <array>
#include "stringProcess.h"
#include <cstdlib>

MyFileListWidget::MyFileListWidget(QWidget* parent)// : QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground, true);
	using namespace std;
	fstream fConfig(configFileName, ios::out);
	if (fConfig.is_open())
	{
		fConfig << "我叼你妈\n操你妈的，我真的服了\n你tm孙子一个" << std::endl;
		fConfig.close();
		fConfig.open(configFileName, ios::in);
		if (fConfig.is_open())
		{
			string strConfig,strTmp;
			//llong linesNum = 0;
			//while (fConfig >> strTmp)
			//	linesNum++;
			//fConfig.clear();
			//fConfig.seekg(0, ios::beg);
			vector<string> ConfigVector;
			while (fConfig >> strTmp)
			{
				strConfig += strTmp + "\n";
				ConfigVector.push_back(strTmp);
			}
			strConfig.erase(strConfig.end()-1);
			extern std::string UTF8ToANSI(std::string utf8Text);
			cout << UTF8ToANSI("配置文件内容：\n") << UTF8ToANSI(strConfig) << std::endl;
			for (string configContent : ConfigVector)
			{
				vector<string> configContentVector = split(configContent);
				configMap[configContentVector.at(0)] = configContentVector.at(1);
				Xindex[configContentVector.at(0)] = std::atoll(configContentVector.at(2).c_str());
				Yindex[configContentVector.at(0)] = std::atoll(configContentVector.at(3).c_str());

			}
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
	itemsMap[id] = item;
}

void MyFileListWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(255, 255, 255, 100));
	if (latticeHeight + verticalSpacing != 0)
		latticeVerticalNum = (height() + verticalSpacing) / (latticeHeight + verticalSpacing);
	if (latticeHeight + horizontalSpacing != 0)
		latticeHorizontalNum = (height() + horizontalSpacing) / (latticeHeight + horizontalSpacing);
	llong 初始的y坐标 = 0,
		初始的x坐标 = 0;//可以改
	llong yNext = 初始的y坐标,
		xNext = 初始的x坐标,
		rowCount=1;
	std::map<llong, llong>::iterator iter;
	for (iter = YCoords.begin(); iter != YCoords.end(); iter++)
	{

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


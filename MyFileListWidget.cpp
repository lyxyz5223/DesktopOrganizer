//有关id的内容觉得没用，于是全都删掉了

//Qt
#include "MyFileListWidget.h"
#include <QPainter>
#include <qevent.h>
#include <QMenu>

//C++
#include <fstream>
//#include <sstream>
#include <iostream>
#include <vector>
//#include <array>
#include "stringProcess.h"
#include <cstdlib>
#include <thread>

//Windows
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")
//My Windows
#include "fileProc.h"
//声明&定义
std::string deletedMaskStr = "<deleted>";
#define ICONSIZE 64
std::wstring j = L"";//需要整理并且放于桌面的路径
//MyFileListWidget* thisWidget;

MyFileListWidget::MyFileListWidget(QWidget* parent,QString path)// : QWidget(parent)
{
	//thisWidget = this;
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground, true);
	resizeZero();
	using namespace std;
	vector<wstring> filesVector = GetFilesArrayForMultiFilePath(vector<wstring>());
	intptr_t fileNum = filesVector.size();
	vector<intptr_t> DirIndexVec = GetDirectoryFromFilesVector(filesVector);
	//intptr_t idCount = 0;
		intptr_t dirIndex = -1;
	for (intptr_t i = 0; i < fileNum; i++)
	{
		MyFileListItem* pLWItem = new MyFileListItem();

		// Get the icon image associated with the item
		SHFILEINFO sfi;
		bool THIS_IS_NOT_A_FILE_OR_DIRECTORY = false;
		for (intptr_t tmpIndex : DirIndexVec)
		{
			if (i == tmpIndex)
			{
				dirIndex = i;
				THIS_IS_NOT_A_FILE_OR_DIRECTORY = true;
				break;
			}
			else if (i == (tmpIndex - 1))
				THIS_IS_NOT_A_FILE_OR_DIRECTORY = true;
		}
		if ((!THIS_IS_NOT_A_FILE_OR_DIRECTORY) && (dirIndex != -1))
		{
			wstring fileNameWithPath = (filesVector[dirIndex] + L"\\" + filesVector[i]);
			DWORD_PTR dw_ptr = SHGetFileInfo(fileNameWithPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
			QIcon desktopItemIcon;
			if (dw_ptr)
				desktopItemIcon.addPixmap(QPixmap::fromImage(QImage::fromHICON(sfi.hIcon)));
			//pLWItem->setIcon(QIcon(desktopItemIcon));
			pLWItem->setText(wstr2str_2UTF8(filesVector[i]).c_str());
			pLWItem->setPath(filesVector[dirIndex]);
			pLWItem->adjustSize();
			pLWItem->setMyIconSize(ICONSIZE);
			pLWItem->setImage(QImage::fromHICON(sfi.hIcon));
			this->addItem(pLWItem/*, std::to_string(idCount)*/, wstr2str_2UTF8(fileNameWithPath));
			//idCount++;
			connect(pLWItem, &MyFileListItem::doubleClicked, this, [=]() {desktopItemProc(fileNameWithPath); });
			connect(pLWItem, &MyFileListItem::deleteItem, this, [=]() {DeleteItem(pLWItem->text().toStdWString(),pLWItem->getPath()); });
		}

	}

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
				vector<string> configContentVector = split(strTmp,"\t");
				if (configContentVector.size() < 3)
				{
					assert(false && "configContentVector.size()<3");
					exit(999);
				}
				//itemsMap[configContentVector.at(1)].filename = configContentVector.at(0);
				//itemsMap[configContentVector.at(1)].id = std::stoll(configContentVector.at(1));
				itemsMap[configContentVector.at(0)] = {
					itemsMap[configContentVector.at(0)].item,//MyFileListItem* item
					configContentVector.at(0),//filename
					//std::stoll(configContentVector.at(1)),//id
					std::stoll(configContentVector.at(1)),//xIndex
					std::stoll(configContentVector.at(2))//yIndex
				};
				std::string::size_type stOfDeletedCount = configContentVector.at(0).find(deletedMaskStr);
				if (stOfDeletedCount != std::string::npos)
				{
					deletedCount = std::stoll(configContentVector.at(0).substr(stOfDeletedCount + deletedMaskStr.size()));
				}
				//configMap[configContentVector.at(0)] = configContentVector.at(1);
				//index[configContentVector.at(1)] = { std::stoll(configContentVector.at(2)) ,std::stoll(configContentVector.at(3)) };
			}
			strConfig.erase(strConfig.end()-1);
			fileNULL:
			extern std::string UTF8ToANSI(std::string utf8Text);
			cout << UTF8ToANSI("配置文件内容：\n") << UTF8ToANSI(strConfig) << std::endl;
		}
	}
	fConfig.close();
	initialize();
	connect(this, &MyFileListWidget::createItem, this, &MyFileListWidget::CreateItem);
	connect(this, &MyFileListWidget::deleteItem, this, &MyFileListWidget::DeleteItem);
	//connect(this, SIGNAL(createItem(std::wstring, std::wstring)), this, SLOT(CreateItem(std::wstring, std::wstring)));
	for (intptr_t tmpIndex : DirIndexVec)
	{
		std::thread threadReadDirectoryChange(&MyFileListWidget::threadReadDirectoryChangesProc,this,filesVector[tmpIndex]);
		threadReadDirectoryChange.detach();
	}
}
void MyFileListWidget::initialize()
{
	llong 初始的y坐标 = 0,
		初始的x坐标 = 0;//可以改
	llong yNext = 初始的y坐标,
		xNext = 初始的x坐标,
		rowCount = 1,
		lineCount = 1;
	for (auto iter = itemsMap.begin(); iter != itemsMap.end(); )
	{
		if (iter->second.item != 0)
		{
			if (iter->second.item->width() > latticeWidth)//
				latticeWidth = iter->second.item->width();
			if (iter->second.item->height() > latticeHeight)
				latticeHeight = iter->second.item->height();
			iter++;
		}
		else
		{
			if (iter->second.filename.find(deletedMaskStr) == std::string::npos)
				itemsMap.erase(iter++);
			else
				iter++;
		}
	}
	if (latticeHeight + verticalSpacing != 0)
		latticeVerticalNum = (height() + verticalSpacing) / (latticeHeight + verticalSpacing);
	if (latticeHeight + horizontalSpacing != 0)
		latticeHorizontalNum = (width() + horizontalSpacing) / (latticeWidth + horizontalSpacing);
	if (latticeVerticalNum != 0)
	{
		latticeHorizontalNum = ((latticeHorizontalNum > ceil(float(itemsMap.size()) / latticeVerticalNum)) ? latticeHorizontalNum : ceil(float(itemsMap.size()) / latticeVerticalNum));
		lastXindex = ceil(float(itemsMap.size()) / latticeVerticalNum);
		lastYindex = itemsMap.size() % latticeVerticalNum;
	}
	for (llong i = 1; i <= latticeVerticalNum; i++, yNext += latticeHeight + verticalSpacing)
		indexToCoord[i].y = yNext;
	for (llong i = 1; i <= latticeHorizontalNum; i++, xNext += latticeWidth + horizontalSpacing)
		indexToCoord[i].x = xNext;
	if (strConfig == "")
	{
		if (size().width() != 0 && size().height() != 0)
			strConfig = "lyxyz5223";
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
		{
			iter->second.xIndex = rowCount;
			iter->second.yIndex = lineCount;
			lineCount++;
			if (lineCount > latticeVerticalNum)
			{
				lineCount = 1;
				rowCount++;
			}
			iter->second.item->move(indexToCoord[iter->second.xIndex].x, indexToCoord[iter->second.yIndex].y);
		}
	}
	else {
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
		{
			if(iter->second.item != 0)
				iter->second.item->move(indexToCoord[iter->second.xIndex].x, indexToCoord[iter->second.yIndex].y);
		}
	}

}

void MyFileListWidget::threadReadDirectoryChangesProc(std::wstring path)
{
	typedef long long llong;
	//auto &itemsMap = thisWidget->itemsMap;
	HANDLE fHandle = CreateFile((path).c_str(),
		FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
		NULL,
		OPEN_EXISTING, 
		FILE_FLAG_BACKUP_SEMANTICS,
		0);
	while(true)
	{
		if (fHandle != INVALID_HANDLE_VALUE)
		{
			DWORD dwBytesReturn = 0;
			BYTE buf[1024] = {};
			FILE_NOTIFY_INFORMATION* lpBuffer = (FILE_NOTIFY_INFORMATION*)buf;
			BOOL RDCresult = ReadDirectoryChangesW(fHandle,
				&buf,
				sizeof(buf),
				FALSE,
				FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
				&dwBytesReturn,
				0,
				0);
			if (RDCresult)
			{
				switch (lpBuffer->Action)
				{
				case FILE_ACTION_ADDED:
				{
					std::wcout << L"创建：" << lpBuffer->FileName << std::endl;
					std::wstring filewithpath = path + L"\\";
					filewithpath += lpBuffer->FileName;
					SendCreateItemSignal(lpBuffer->FileName, path);
				}
				break;
				case FILE_ACTION_REMOVED:
				{
					std::wcout << L"删除：" << lpBuffer->FileName << std::endl;
					SendDeleteItemSignal(lpBuffer->FileName, path);
				}
				break;
				case FILE_ACTION_RENAMED_OLD_NAME:
				{
					std::wcout << L"重命名：" << lpBuffer->FileName << std::endl;
					// 获取新文件名的条目
					FILE_NOTIFY_INFORMATION* offsetFNImformation = (FILE_NOTIFY_INFORMATION*)((LPBYTE)lpBuffer + lpBuffer->NextEntryOffset);
					if (offsetFNImformation->Action == FILE_ACTION_RENAMED_NEW_NAME)
					{
						// 新文件名
						std::wstring newName(lpBuffer->FileName, lpBuffer->FileNameLength / sizeof(WCHAR));
						std::cout << UTF8ToANSI("        ╰->");
						std::wcout << offsetFNImformation->FileName << std::endl;
					}
				}
				break;
				//case FILE_ACTION_RENAMED_NEW_NAME://貌似这个无效
				//{
				//	std::wcout << L"重命名（新）：" << lpBuffer->FileName << std::endl;
				//}
				//break;

				}
			}
		}
	}

}

void MyFileListWidget::addItem(MyFileListItem* item/*, std::string id*/,std::string nameWithPath)
{
	item->setParent(this);
	//itemsMap[id] = item;
	itemsMap[nameWithPath].item = item;
	itemsMap[nameWithPath].filename = nameWithPath;
	//itemsMap[nameWithPath].id = std::stoll(id);
	item->setViewMode(viewMode);
}

void MyFileListWidget::mousePressEvent(QMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::MouseButton::RightButton:
	{
		QMenu* menu1 = new QMenu(this);
		menu1->addAction(QIcon(), "你好");
		menu1->addAction(QIcon(), "傻逼");
		menu1->addAction(QIcon(), "刷新");
		menu1->exec(QCursor::pos());
		break;
	}
	default: 
		break;

	}
}

void MyFileListWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(255, 255, 255, 0));
	writeConfig(itemsMap);
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

void MyFileListWidget::CreateItem(std::wstring name,std::wstring path)
{
	std::wstring namewithpath = path + L"\\";
	namewithpath += name;
	if (!(itemsMap.count(wstr2str_2UTF8(namewithpath)) > 0))
	{
		if (deletedCount > 0)
		{
			llong tmpDeletedCount = deletedCount--;
			itemsMap[(wstr2str_2UTF8(namewithpath))] = itemsMap[deletedMaskStr + std::to_string(tmpDeletedCount)];
			itemsMap.erase(deletedMaskStr + std::to_string(tmpDeletedCount));
		}
		else
		{
			itemsMap[(wstr2str_2UTF8(namewithpath))] = {
				itemsMap[(wstr2str_2UTF8(namewithpath))].item,
				wstr2str_2UTF8(name),
				(lastYindex < latticeVerticalNum ? lastXindex : lastXindex + 1),
				(lastYindex < latticeVerticalNum ? lastYindex + 1 : 1),
			};
		}
		SHFILEINFO sfi = { 0 };
		DWORD_PTR dw_ptr = SHGetFileInfo(namewithpath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
		QIcon desktopItemIcon;
		if (dw_ptr)
			desktopItemIcon.addPixmap(QPixmap::fromImage(QImage::fromHICON(sfi.hIcon)));
		MyFileListItem* pLWItem = new MyFileListItem();
		pLWItem->setText(wstr2str_2UTF8(name).c_str());
		pLWItem->setPath(path);
		pLWItem->adjustSize();
		pLWItem->setMyIconSize(ICONSIZE);
		pLWItem->setImage(QImage::fromHICON(sfi.hIcon));
		addItem(pLWItem/*, std::to_string(itemsMapElement1.id)*/, wstr2str_2UTF8(namewithpath));
		pLWItem->move(indexToCoord[itemsMap[wstr2str_2UTF8(namewithpath)].xIndex].x, indexToCoord[itemsMap[wstr2str_2UTF8(namewithpath)].yIndex].y);
		connect(pLWItem, &MyFileListItem::doubleClicked, this, [=]() {desktopItemProc(namewithpath); });
		connect(pLWItem, &MyFileListItem::deleteItem, this, [=]() {DeleteItem(pLWItem->text().toStdWString(), pLWItem->getPath()); });
		pLWItem->show();
		initialize();
	}
}

void MyFileListWidget::DeleteItem(std::wstring name, std::wstring path)
{
	std::wstring namewithpath = path + L"\\";
	namewithpath += name;
	if (itemsMap.count(wstr2str_2UTF8(namewithpath)) > 0)
	{
		++deletedCount;
		std::string tmpstr = deletedMaskStr + std::to_string(deletedCount);

		itemsMap[wstr2str_2UTF8(namewithpath)].item->hide();
		disconnect(itemsMap[wstr2str_2UTF8(namewithpath)].item);
		destroyed(itemsMap[wstr2str_2UTF8(namewithpath)].item);

		itemsMap[wstr2str_2UTF8(namewithpath)].item = 0;
		itemsMap[wstr2str_2UTF8(namewithpath)].filename = tmpstr;
		itemsMap[tmpstr] = itemsMap[wstr2str_2UTF8(namewithpath)];//可以是/:\*?|"<>这几个符号
		//itemsMap[tmpstr].item = 0;
		itemsMap.erase(wstr2str_2UTF8(namewithpath));
		//itemsMap[wstr2str_2UTF8(namewithpath)].item->deleteLater();
	}

}

bool MyFileListWidget::writeConfig(std::map<std::string/*id*/, ItemProp> config_map, std::string 分隔符)
{
	using namespace std;
	fstream fConfig(configFileName,ios::out);
	if (!fConfig.is_open())
		return false;
	for (auto iter = config_map.begin(); iter != config_map.end(); iter++)
		fConfig << iter->second.filename << 分隔符
		//<< iter->second.id << 分隔符 
		<< iter->second.xIndex << 分隔符 
		<< iter->second.yIndex<< endl;
	fConfig.flush();
	fConfig.close();
	return true;
}

void MyFileListWidget::desktopItemProc(std::wstring nameWithPath)
{
	std::cout << UTF8ToANSI(wstr2str_2UTF8(nameWithPath)).c_str() << std::endl;
	ShellExecute(0, L"open", nameWithPath.c_str(), L"", 0, SW_NORMAL);
}
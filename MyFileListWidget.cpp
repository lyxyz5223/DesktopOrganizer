//有关id的内容觉得没用，于是全都删掉了
//Windows
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")
#include <thumbcache.h>
#include <atlbase.h>
#include <atlcore.h>
#include <atlcom.h>

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

//My Windows
#include "fileProc.h"
//声明&定义
//std::string deletedMaskStr = "<deleted>";
#define ICONSIZE 64

MyFileListWidget::MyFileListWidget(QWidget* parent,QString path)// : QWidget(parent)
{
	//desktopPathVector.push_back(L"C:\\Users\\lyxyz5223\\Desktop\\md");
	if(!path.isEmpty())
		desktopPathVector.push_back(path.toStdWString());
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground, true);
	SetParent((HWND)winId(), (HWND)parent->winId());
	resizeZero();
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
				vector<string> configContentVector = split(strTmp,"\t");
				if (configContentVector.size() < 3)
				{
					assert(false && "configContentVector.size() < 3");
					exit(999);
				}
				itemsMap[configContentVector.at(0)] = {
					itemsMap[configContentVector.at(0)].item,//MyFileListItem* item
					configContentVector.at(0),//filename
					std::stoll(configContentVector.at(1)),//xIndex
					std::stoll(configContentVector.at(2))//yIndex
				};
			}
			strConfig.erase(strConfig.end()-1);
			fileNULL:
			extern std::string UTF8ToANSI(std::string utf8Text);
			cout << UTF8ToANSI("配置文件内容：\n") << UTF8ToANSI(strConfig) << std::endl;
		}
	}
	fConfig.close();
	//initialize();
	connect(this, &MyFileListWidget::createItem, this, &MyFileListWidget::CreateItem);
	connect(this, &MyFileListWidget::deleteItem, this, &MyFileListWidget::DeleteItem);
	//connect(this, SIGNAL(createItem(std::wstring, std::wstring)), this, SLOT(CreateItem(std::wstring, std::wstring)));
	std::thread threadCheckFilesChange(&MyFileListWidget::checkFilesChangeProc, this, desktopPathVector);
	threadCheckFilesChange.detach();
}
void MyFileListWidget::initialize()
{
	llong yNext = firstVerticalSpacing,
		xNext = firstHorizontalSpacing,
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
			itemsMap.erase(iter++);
	}
	if (latticeHeight + verticalSpacing != 0)
		latticeVerticalNum = (height() + verticalSpacing) / (latticeHeight + verticalSpacing);//计算屏幕能容纳的（垂直）个数
	if (latticeHeight + horizontalSpacing != 0)
		latticeHorizontalNum = (width() + horizontalSpacing) / (latticeWidth + horizontalSpacing);//计算屏幕能容纳的（水平）个数
	if (latticeVerticalNum != 0)
		latticeHorizontalNum = ((latticeHorizontalNum > ceil(float(itemsMap.size()) / latticeVerticalNum)) ? latticeHorizontalNum : ceil(float(itemsMap.size()) / latticeVerticalNum));
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
			latticeJudge[std::pair<llong, llong>(iter->second.xIndex, iter->second.yIndex)] = true;
		}
	}
	else
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
			if (iter->second.item != 0)
			{
				iter->second.item->move(indexToCoord[iter->second.xIndex].x, indexToCoord[iter->second.yIndex].y);
				latticeJudge[std::pair<llong, llong>(iter->second.xIndex, iter->second.yIndex)] = true;
			}
	writeConfig(itemsMap);
}

void MyFileListWidget::addItem(MyFileListItem* item/*, std::string id*/,std::string nameWithPath)
{
	item->setParent(this);
	itemsMap[nameWithPath].item = item;
	itemsMap[nameWithPath].filename = nameWithPath;
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
	QWidget::paintEvent(e);
}

void MyFileListWidget::CreateItem(std::wstring name,std::wstring path)
{
	std::wstring namewithpath = path + L"\\";
	namewithpath += name;
	if (!(itemsMap.count(wstr2str_2UTF8(namewithpath)) > 0))
	{
		llong i_x = 1;
		llong i_y = 1;
		for (; i_x <= latticeHorizontalNum+1;)
		{
			if (latticeJudge.count(std::pair<llong, llong>(i_x, i_y)))
			{
				if (!latticeJudge[std::pair<llong, llong>(i_x, i_y)])
					break;
			}
			else
				//下一个位置
				break;
			++i_y;
			if (i_y > latticeVerticalNum)
			{
				++i_x;
				i_y = 1;
			}
		}
		itemsMap[(wstr2str_2UTF8(namewithpath))] = {
			itemsMap[(wstr2str_2UTF8(namewithpath))].item,
			wstr2str_2UTF8(name),
			i_x,
			i_y
		};
		QImage pLWItemImage;
		//IShellItemImageFactory* ISIIFactory = nullptr;
		//HRESULT hr = SHCreateItemFromParsingName(namewithpath.c_str(), nullptr, __uuidof(IShellItemImageFactory), (void**)&ISIIFactory);
		//if (SUCCEEDED(hr))
		//{
		//	SIZE imageSize = { 256,256 };
		//	HBITMAP bitmap;
		//	hr = ISIIFactory->GetImage(imageSize, SIIGBF_RESIZETOFIT, &bitmap);
		//	if (SUCCEEDED(hr))
		//		pLWItemImage = QImage::fromHBITMAP(bitmap);
		//	DeleteObject(bitmap);
		//	ISIIFactory->Release();
		//}
		if (pLWItemImage.isNull())
		{
			SHFILEINFO sfi;
			DWORD_PTR dw_ptr = SHGetFileInfo(namewithpath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
			if (dw_ptr)
				pLWItemImage = QImage::fromHICON(sfi.hIcon);
		}
		MyFileListItem* pLWItem = new MyFileListItem();
		pLWItem->setText(wstr2str_2UTF8(name).c_str());
		pLWItem->setPath(path);
		pLWItem->adjustSize();
		pLWItem->setMyIconSize(ICONSIZE);
		pLWItem->setImage(pLWItemImage);
		//pLWItem->setImage(QImage::fromHICON(sfi.hIcon));
		addItem(pLWItem, wstr2str_2UTF8(namewithpath));
		//pLWItem->move(indexToCoord[itemsMap[wstr2str_2UTF8(namewithpath)].xIndex].x, indexToCoord[itemsMap[wstr2str_2UTF8(namewithpath)].yIndex].y);
		connect(pLWItem, &MyFileListItem::doubleClicked, this, [=]() {desktopItemProc(namewithpath); });
		connect(pLWItem, &MyFileListItem::deleteItem, this, [=]() {DeleteItem(pLWItem->text().toStdWString(), pLWItem->getPath()); });
		connect(pLWItem, &MyFileListItem::selected, this, 
			[=]() {
				if (selectedItem != 0)
					selectedItem->setSelected(false);
				selectedItem = pLWItem;
				selectedItem->setSelected(true);
			});
		pLWItem->show();
		initialize();
	}
}

void MyFileListWidget::DeleteItem(std::wstring name, std::wstring path)
{
	delete_ing = true;
	std::wstring namewithpath = path + L"\\";
	namewithpath += name;
	if (itemsMap.count(wstr2str_2UTF8(namewithpath)) > 0)
	{
		if (itemsMap[wstr2str_2UTF8(namewithpath)].item != 0)
		{
			itemsMap[wstr2str_2UTF8(namewithpath)].item->hide();
			disconnect(itemsMap[wstr2str_2UTF8(namewithpath)].item);
			destroyed(itemsMap[wstr2str_2UTF8(namewithpath)].item);
		}
		////important!!!!!!!!!!!!
		latticeJudge[std::pair<llong, llong>(itemsMap[wstr2str_2UTF8(namewithpath)].xIndex, itemsMap[wstr2str_2UTF8(namewithpath)].yIndex)] = false;
		itemsMap.erase(wstr2str_2UTF8(namewithpath));
		////
	}
	delete_ing = false;
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

void MyFileListWidget::checkFilesChangeProc(std::vector<std::wstring> pathVector)
{
	while (true)
	{
		using namespace std;
		vector<wstring> filesVector = GetFilesArrayForMultiFilePath(pathVector);
		map<wstring/*nameWithNoPath*/, wstring/*path*/>toBeDelete;
		for (auto iter = itemsMap.begin(); iter != itemsMap.end();iter++)
		{
			if (iter->second.item != 0)
			{
				wstring nameWithNoPath = iter->second.item->text().toStdWString();
				if (std::find(filesVector.begin(), filesVector.end(), nameWithNoPath) == filesVector.end())
				{
					//Can't find it!
					//So delete it!
					//SendDeleteItemSignal(nameWithNoPath, iter->second.item->getPath());
					toBeDelete[nameWithNoPath] = iter->second.item->getPath();
				}
			}
		}
		for (auto iter = toBeDelete.begin(); iter != toBeDelete.end(); iter++)
		{
			SendDeleteItemSignal(iter->first, iter->second);
		}
		if (!delete_ing)
		{

			intptr_t fileNum = filesVector.size();
			vector<intptr_t> DirIndexVec = GetDirectoryFromFilesVector(filesVector);
			//intptr_t idCount = 0;
			intptr_t dirIndex = -1;
			for (intptr_t i = 0; i < fileNum; i++)
			{
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
					if (!itemsMap.count(wstr2str_2UTF8(fileNameWithPath)))
					{
						SendCreateItemSignal(filesVector[i], filesVector[dirIndex]);
					}
				}
			}
		}
		Sleep(100);
	}
}

//void MyFileListWidget::threadReadDirectoryChangesProc(std::wstring path)
//{
//	typedef long long llong;
//	//auto &itemsMap = thisWidget->itemsMap;
//	HANDLE fHandle = CreateFile((path).c_str(),
//		FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
//		NULL,
//		OPEN_EXISTING, 
//		FILE_FLAG_BACKUP_SEMANTICS,
//		0);
//	while(true)
//	{
//		if (fHandle != INVALID_HANDLE_VALUE)
//		{
//			DWORD dwBytesReturn = 0;
//			BYTE buf[1024] = {};
//			FILE_NOTIFY_INFORMATION* lpBuffer = (FILE_NOTIFY_INFORMATION*)buf;
//			BOOL RDCresult = ReadDirectoryChangesW(fHandle,
//				&buf,
//				sizeof(buf),
//				FALSE,
//				FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
//				&dwBytesReturn,
//				0,
//				0);
//			if (RDCresult)
//			{
//				switch (lpBuffer->Action)
//				{
//				case FILE_ACTION_ADDED:
//				{
//					std::wcout << L"创建：" << lpBuffer->FileName << std::endl;
//					std::wstring filewithpath = path + L"\\";
//					filewithpath += lpBuffer->FileName;
//					SendCreateItemSignal(lpBuffer->FileName, path);
//				}
//				break;
//				case FILE_ACTION_REMOVED:
//				{
//					std::wcout << L"删除：" << lpBuffer->FileName << std::endl;
//					SendDeleteItemSignal(lpBuffer->FileName, path);
//				}
//				break;
//				case FILE_ACTION_RENAMED_OLD_NAME:
//				{
//					std::wcout << L"重命名：" << lpBuffer->FileName << std::endl;
//					// 获取新文件名的条目
//					FILE_NOTIFY_INFORMATION* offsetFNImformation = (FILE_NOTIFY_INFORMATION*)((LPBYTE)lpBuffer + lpBuffer->NextEntryOffset);
//					if (offsetFNImformation->Action == FILE_ACTION_RENAMED_NEW_NAME)
//					{
//						// 新文件名
//						std::wstring newName(lpBuffer->FileName, lpBuffer->FileNameLength / sizeof(WCHAR));
//						std::cout << UTF8ToANSI("        ╰->");
//						std::wcout << offsetFNImformation->FileName << std::endl;
//					}
//				}
//				break;
//				//case FILE_ACTION_RENAMED_NEW_NAME://貌似这个无效
//				//{
//				//	std::wcout << L"重命名（新）：" << lpBuffer->FileName << std::endl;
//				//}
//				//break;
//
//				}
//			}
//		}
//	}
//}

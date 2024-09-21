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
std::wstring desktopPath = L"";//需要整理并且放于桌面的路径
void ReadDirectoryChangesProc();
void LpoverlappedCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

MyFileListWidget::MyFileListWidget(QWidget* parent,QString path)// : QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground, true);
	using namespace std;
	WCHAR cFilePath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_DESKTOP, 0, 0, cFilePath);
	//MessageBox(HWND_thisApp, cFilePath, L"", 0);
	if(desktopPath == L"")
		desktopPath = cFilePath;
	//desktopPath += L"\\MyDesktop\\";
	//判断文件夹是否存在，不存在自动创建
	if (PathFileExists(desktopPath.c_str()) == TRUE)
	{
		//exist
	}
	else
		//do not exist
		if (CreateDirectory(desktopPath.c_str(), 0) == FALSE)
		{
			MessageBox(0, L"文件夹不存在且创建失败，请检查你是否有权限，或者尝试以管理员身份运行。程序将退出。", 0, MB_ICONERROR);
			exit(2);
		}
	if (desktopPath.back() != L'\\')
		desktopPath += L"\\";
	std::wstring** filesList;
	intptr_t fileNum = GetFileNum(desktopPath, true);
	GetFilesArray(desktopPath, filesList);
	for (intptr_t i = 0; i < fileNum; i++)
	{
		MyFileListItem* pLWItem = new MyFileListItem();

		// Get the icon image associated with the item
		SHFILEINFO sfi;
		DWORD_PTR dw_ptr = SHGetFileInfo((desktopPath + filesList[i][0]).c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
		QIcon desktopItemIcon;
		if (dw_ptr)
			desktopItemIcon.addPixmap(QPixmap::fromImage(QImage::fromHICON(sfi.hIcon)));
		//pLWItem->setIcon(QIcon(desktopItemIcon));
		pLWItem->setText(wstr2str_2UTF8(filesList[i][0]).c_str());
		pLWItem->adjustSize();
		pLWItem->setMyIconSize(64);
		pLWItem->setImage(QImage::fromHICON(sfi.hIcon));
		this->addItem(pLWItem, std::to_string(i + 1));
		connect(pLWItem, &MyFileListItem::doubleClicked, this, [=]() {desktopItemProc(filesList[i][0]); });
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
				if (configContentVector.size() < 4)
				{
					assert(false && "configContentVector.size()<4");
					exit(999);
				}
				//itemsMap[configContentVector.at(1)].filename = configContentVector.at(0);
				//itemsMap[configContentVector.at(1)].id = std::atoll(configContentVector.at(1).c_str());
				itemsMap[configContentVector.at(1)] = {
					itemsMap[configContentVector.at(1)].item,//MyFileListItem* item
					configContentVector.at(0),//filename
					std::atoll(configContentVector.at(1).c_str()),//id
					std::atoll(configContentVector.at(2).c_str()),//xIndex
					std::atoll(configContentVector.at(3).c_str())//yIndex
				};
				//configMap[configContentVector.at(0)] = configContentVector.at(1);
				//index[configContentVector.at(1)] = { std::atoll(configContentVector.at(2).c_str()) ,std::atoll(configContentVector.at(3).c_str()) };
			}
			strConfig.erase(strConfig.end()-1);
			fileNULL:
			extern std::string UTF8ToANSI(std::string utf8Text);
			cout << UTF8ToANSI("配置文件内容：\n") << UTF8ToANSI(strConfig) << std::endl;
		}
	}
	fConfig.close();
	std::thread threadReadDirectoryChange(ReadDirectoryChangesProc);
	threadReadDirectoryChange.detach();
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

void ReadDirectoryChangesProc()
{
	HANDLE fHandle = CreateFile((desktopPath + L"dd\\").c_str(),
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
				}
				break;
				case FILE_ACTION_REMOVED:
				{
					std::wcout << L"删除：" << lpBuffer->FileName << std::endl;
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

void MyFileListWidget::addItem(MyFileListItem* item, std::string id)
{
	item->setParent(this);
	//itemsMap[id] = item;
	itemsMap[id].item = item;
	itemsMap[id].filename = item->text().toStdString();
	itemsMap[id].id = std::atoll(id.c_str());
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
	for (llong i = 1; i <= latticeVerticalNum; i++, yNext += latticeHeight + verticalSpacing)
		indexToCoord[i].y = yNext;
	for (llong i = 1; i <= latticeHorizontalNum; i++, xNext += latticeWidth+horizontalSpacing)
		indexToCoord[i].x = xNext;
	for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
	{
		if (iter->second.item->width() > latticeWidth)//
			latticeWidth = iter->second.item->width();
		if (iter->second.item->height() > latticeHeight)
			latticeHeight = iter->second.item->height();
	}
	if (strConfig == "")
	{
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
			
			iter->second.item->move(indexToCoord[iter->second.xIndex].x,indexToCoord[iter->second.yIndex].y);
			//iter->second->move(XCoords[Xindex[iter->first]], YCoords[Yindex[iter->first]]);
			//iter->second->resize(latticeWidth,latticeHeight);
			//iter->second->show();
		}
	}
	else {
		for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
		{
			//if (Xindex.count(iter->first) > 0)
			//	iter->second->move(XCoords[Xindex[iter->first]], YCoords[Yindex[iter->first]]);
			iter->second.item->move(indexToCoord[iter->second.xIndex].x, indexToCoord[iter->second.yIndex].y);
			//else
			//{
			//	configMap[iter->second->text().toStdString()] = iter->first;
			//	//找到空出来的位置

			//	//iter->second->move(XCoords[Xindex[iter->first]], YCoords[Yindex[iter->first]]);
			//	iter->second->move(XCoords[index[iter->first].x], YCoords[index[iter->first].y]);
			//}
		}
	}
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

bool MyFileListWidget::writeConfig(std::map<std::string/*id*/, ItemProp> config_map, std::string 分隔符)
{
	using namespace std;
	fstream fConfig(configFileName,ios::out);
	if (!fConfig.is_open())
		return false;
	for (auto iter = config_map.begin(); iter != config_map.end(); iter++)
		fConfig << iter->second.filename << 分隔符
		<< iter->second.id << 分隔符 
		<< iter->second.xIndex << 分隔符 
		<< iter->second.yIndex<< endl;
	fConfig.flush();
	fConfig.close();
	return true;
}

void MyFileListWidget::desktopItemProc(std::wstring name)
{
	if (desktopPath.back() != L'\\')
		desktopPath += L"\\";

	/*
	std::wstring comm = L"start \"\" \"";
	comm += desktopPath;
	comm += name;
	comm += L"\"";
	std::cout << wstr2str_2ANSI(comm).c_str() << std::endl;
	_wsystem(comm.c_str());
	//上述语句=下面的
	//std::cout << UTF8ToANSI(wstr2str_2UTF8(comm)).c_str() << std::endl;
	//system(wstr2str_2ANSI((comm)).c_str());
	*/

	std::cout << UTF8ToANSI(wstr2str_2UTF8(name)).c_str() << std::endl;
	ShellExecute(0, L"open", name.c_str(), L"", desktopPath.c_str(), SW_NORMAL);
}

//有关id的内容觉得没用，于是全都删掉了
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
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
#include <qmessagebox.h>

//C++
#include <fstream>
//#include <sstream>
#include <iostream>
#include <vector>
//#include <array>
#include "stringProcess.h"
#include <cstdlib>
#include <thread>
#include <codecvt>
#include <locale>

// DataBase
#define OTL_ODBC
#include "./lib/otlv4.h"

//My Windows
#include "fileProc.h"
//声明&定义


bool isDigits(std::wstring wstr)
{
	for (wchar_t ch : wstr)
		if (ch < '0' || ch > '9')
			return false;
	return true;
}

bool PathCompletion(std::wstring& path)
{
	if (path == L"")
		return false;
	if (/*path != L"" && */path.back() != L'\\' && path.back() != L'/')
		path += L'\\';
	return true;
}
void MyFileListWidget::changeItemSizeAndNumbersPerColumn()
{
	QScreen* sc = this->screen();
	if (!sc) return;
	QRect re = sc->geometry();
	re = sc->availableGeometry();
	itemSize = QSize(re.height() / zoomScreen * 4 / 5, re.height() / zoomScreen);
	itemsNumPerColumn = height() / (itemSize.height() + itemSpacing.line);
	if (itemsNumPerColumn == 0)
		itemsNumPerColumn = 1;
}
MyFileListWidget::MyFileListWidget(QWidget* parent, std::vector<std::wstring> pathsList, std::wstring config)// : QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint);
	SetParent((HWND)winId(), (HWND)parent->winId());
	setMouseTracking(true);
	installEventFilter(this);
	this->pathsList = pathsList;
	configFileNameWithPath = config;
	if (!readConfigFile(config))
		QMessageBox::critical(this, "错误", "配置文件读取失败！程序即将退出。");
	/*计算item大小和每列item的个数*/
	changeItemSizeAndNumbersPerColumn();

	connect(this, &MyFileListWidget::createItemSignal, this, &MyFileListWidget::createItem);
	connect(this, &MyFileListWidget::removeItemSignal, this, &MyFileListWidget::removeItem);
	for (auto i = pathsList.begin(); i != pathsList.end(); i++)
	{
		std::thread checkFilesChangeThread(&MyFileListWidget::checkFilesChangeProc, this, *i);
		checkFilesChangeThread.detach();
	}
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

void MyFileListWidget::mouseReleaseEvent(QMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::MouseButton::LeftButton:
	{
		selectionArea->hide();
	}
		break;
	default:
		break;
	}
}

void MyFileListWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(255, 255, 255, 1));

	//选中区域的绘制
	//p.fillRect(selectionRect, QColor(10, 123, 212, 100));
	//p.setPen(QColor(10, 123, 212));
	//p.drawRect(selectionRect);
	QWidget::paintEvent(e);
}
void MyFileListWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		QPoint p = mapToParent(e->pos());
		selectionRect.setBottomRight(p);
		selectionArea->move((selectionRect.left() < selectionRect.right() ? selectionRect.left() : selectionRect.right()),
			(selectionRect.top() < selectionRect.bottom() ? selectionRect.top() : selectionRect.bottom()));
		selectionArea->resize(selectionRect.width() >= 0 ? selectionRect.width() : -selectionRect.width(),
			selectionRect.height() >= 0 ? selectionRect.height() : -selectionRect.height());
		selectionArea->update();
		update();
		//std::cout << selectionRect.x() << "," << selectionRect.y()
		//	<< "," << selectionRect.width() << "," << selectionRect.height() << std::endl;
	}

}
bool MyFileListWidget::eventFilter(QObject* watched, QEvent* event)
{
	switch (event->type())
	{
	case QEvent::MouseButtonPress:
	{
		QMouseEvent* e = static_cast<QMouseEvent*>(event);
		switch (e->button())
		{
		case Qt::LeftButton:
		{
#ifdef _DEBUG
			std::cout << "leftButtonPress" << std::endl;
#endif // _DEBUG
			QPoint p = mapToParent(e->pos());
			selectionRect = QRect(p.x(), p.y(), 0, 0);
			if (!selectionArea)
				selectionArea = new SelectionArea(this);
			selectionArea->resize(0, 0);
			selectionArea->show();
			selectionArea->move((selectionRect.left() < selectionRect.right() ? selectionRect.left() : selectionRect.right()),
				(selectionRect.top() < selectionRect.bottom() ? selectionRect.top() : selectionRect.bottom()));
			break;
		}
		default:
			break;
		}
		break;
	}
	case QEvent::MouseButtonRelease:
	{
		QMouseEvent* e = static_cast<QMouseEvent*>(event);
		switch (e->button())
		{
		case Qt::LeftButton:
		{
#ifdef _DEBUG
			std::cout << "leftButtonRelease" << std::endl;
#endif
			break;
		}
		default:
			break;
		}
		break;
	}
	}
	return QWidget::eventFilter(watched, event);
}

std::vector<std::wstring> MyFileListWidget::splitForConfig(std::wstring text, std::wstring delimiter/*separator,分隔符*/, std::wstring EscapeString /*char EscapeCharacter*/)
{
	std::vector<std::wstring> result(3);
	std::vector<std::wstring> res = split(text, delimiter, EscapeString);
	for (int i = 0; i < 2; i++)
	{
		if (!res.size())
			return std::vector<std::wstring>();
		size_t c = std::count(res[0].begin(), res[0].end(), L'"');
		if (c == 2)
		{
			result[i] += res[0];
			std::wstring::size_type st1 = res[0].find(L'"');
			std::wstring::size_type st2 = res[0].find_last_of(L'"');
			if(st2 > st1)
				result[i] = result[i].substr(st1 + 1, st2 - st1 - 1);//除去字符串前面的双引号
			res.erase(res.begin());
		}
		else if (c == 1)
		{
			std::wstring::size_type st = res[0].find(L'"');
			res[0] = res[0].substr(st + 1);//除去字符串前面的双引号
			//std::wstring::size_type st = std::wstring::npos;
			auto iter = res.begin() + 1;
			for (; iter != res.end(); iter++)
			{
				std::wstring::size_type st = iter->find(L'"');
				if (st != std::wstring::npos && st == iter->length() - 1)
					break;
			}
			if (iter == res.end())
				return std::vector<std::wstring>();
			else
			{
				for (auto it = res.begin(); it != iter + 1; it++)
					result[i] += (*it) + delimiter;
				result[i].pop_back();//除去字符串后面的分隔符delimiter
				result[i].pop_back();//除去字符串后面的双引号
				res.erase(res.begin(), iter + 1);
			}
		}
		else
			return std::vector<std::wstring>();
	}
	for (auto iter = res.begin(); iter != res.end(); iter++)
		if (isDigits(*iter))
			result[2] = *iter;
	return result;
}
bool MyFileListWidget::readConfigFile(std::wstring nameWithPath)
{
	using namespace std;
	fstream fConfig(nameWithPath, ios::app | ios::out);
	if (!fConfig.is_open())
		return false;
	else
	{
		//fConfig << "1.txt 1 1 1" << std::endl;//Test Write
		fConfig.close();
		fConfig.open(nameWithPath, ios::in);
		if (fConfig.is_open())
		{
			//llong linesNum = 0;
			//while (fConfig >> strTmp)
			//	linesNum++;
			//fConfig.clear();
			//fConfig.seekg(0, ios::beg);
#ifdef _DEBUG
			wcout << str2wstr_2UTF8("配置文件：") << endl;
#endif // _DEBUG
			if (fConfig.peek() != ifstream::traits_type::eof())//判断文件是否为空，成立为空
			{
				size_t lineCount = 0;
				wstring wstrLine;
				string strLine;
				while (getline(fConfig, strLine))
				{
					std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
					wstrLine = converter.from_bytes(strLine);
					vector<wstring> conf = splitForConfig(wstrLine);
#ifdef _DEBUG
					if (conf.size() < 3)
					{
						cout << "配置文件存在错误:\n	line " << ++lineCount << ":" << wstr2str_2ANSI(wstrLine) << endl;
						continue;
					}
					cout << wstr2str_2ANSI(wstrLine) << endl;
#else
					if (conf.size() < 3)
						continue;
#endif // _DEBUG
					itemsMap[conf[1]+conf[0]] = {
						0, conf[0], conf[1], stoll(conf[2])
					};
				}
			}
		}
	}
	return true;
}
bool MyFileListWidget::writeConfigFile(std::wstring nameWithPath)
{
	using namespace std;
	wstring delimiter = L" ";
	ofstream outConfig;
	outConfig.open(nameWithPath);
	if (!outConfig.is_open())
		return false;
	else
	{
		string(*encodeType)(wstring) = wstr2str_2UTF8;
		for (auto i = itemsMap.begin(); i != itemsMap.end(); i++)
		{
			outConfig << "\"" << encodeType(i->second.name);
			outConfig << "\"" << encodeType(delimiter);
			outConfig << "\"" << encodeType(i->second.path);
			outConfig << "\"" << encodeType(delimiter)
				<< i->second.position << endl;
			outConfig.flush();
		}
		outConfig.close();
	}
	return true;
}

void MyFileListWidget::checkFilesChangeProc(std::wstring path)
{
	//using namespace std;
	//if (!PathCompletion(path))
	//	return;
	//vector<vector<wstring>> filesVector;
	//long long filesNum = GetFilesArray(path, filesVector);
	//if (filesNum <= 0)
	//	MessageBox((HWND)this->winId(), L"检测文件更改失败，软件可能无法正常运行", L"错误", MB_ICONERROR);
	using namespace std;
	WCHAR* cFilePath;
	cFilePath = (WCHAR*)path.c_str();
	while (true)
	{
		intptr_t FileIndex = 0;
		WIN32_FIND_DATA ffd;
		LARGE_INTEGER filesize;
		//TCHAR szDir[MAX_PATH];
		HANDLE hFind = INVALID_HANDLE_VALUE;
		DWORD dwError = 0;
		if (!PathCompletion(path))
			return;
		wstring p = path + L"*";
		// Find the first file in the directory.
		hFind = FindFirstFile(p.c_str(), &ffd);
		if (INVALID_HANDLE_VALUE == hFind)
		{
			//DisplayErrorBox(TEXT("FindFirstFile"));
			MessageBox((HWND)this->winId(), L"检测文件更改失败，软件可能无法正常运行", L"错误", MB_ICONERROR);
		}
		auto tmpItemsMap = itemsMap;// 新建临时map
		// List all the files in the directory with some info about them.
		do
		{
			if (wcscmp(ffd.cFileName, L".") != 0
				&& wcscmp(ffd.cFileName, L"..") != 0)
			{
				vector<wstring> tmpwstringarray(2);
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					//wprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
					//这是一个文件夹，名称为：ffd.cFileName
					tmpwstringarray[0] = ffd.cFileName;
					tmpwstringarray[1] = L"Directory";
				}
				else
				{
					filesize.LowPart = ffd.nFileSizeLow;
					filesize.HighPart = ffd.nFileSizeHigh;
					//wprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
					//这是一个文件，名称为：ffd.cFileName，大小为filesize.QuadPart
					tmpwstringarray[0] = ffd.cFileName;
					tmpwstringarray[1] = L"File";
				}
				wstring nameWithPath = path + tmpwstringarray[0];
				if (itemsMap.count(nameWithPath))
				{
					// Find it,remove it from tmpItemsMap
					tmpItemsMap.erase(nameWithPath);
					if (!itemsMap[nameWithPath].item && !isCreatingItem && !isRemovingItem)
					{
						isCreatingItem = true;
						emit createItemSignal(tmpwstringarray[0], path);
					}
				}
				else
				{
					// Can't find the file in itemsMap
					// Send CreateItem Signal
					if (!isCreatingItem && !isRemovingItem)
					{
						isCreatingItem = true;
						emit createItemSignal(tmpwstringarray[0], path);
					}
				}
				FileIndex++;
			}
		} while (FindNextFile(hFind, &ffd) != 0);
		dwError = GetLastError();
		//if (dwError != ERROR_NO_MORE_FILES)
		//{
		//	//DisplayErrorBox(TEXT("FindFirstFile"));
		//}
		FindClose(hFind);
		// When files list finished, remove the redundant(多余的) items
		// Send RemoveItem Signal
		for (auto i = tmpItemsMap.begin(); i != tmpItemsMap.end(); )
		{
			if (i->second.item != nullptr)
			{
				isRemovingItem = true;
				i->second.item->deleteLater();
				isRemovingItem = false;
			}
			tmpItemsMap.erase(i++);//////////////////////////////////////////
		}
		size_t npcBack = itemsNumPerColumn;
		/*重新计算item大小和每列item的个数*/
		changeItemSizeAndNumbersPerColumn();
		// 变小
		for (auto i = itemsMap.begin(); i != itemsMap.end(); i++)
		{
			int xIndex = i->second.position / itemsNumPerColumn + 1;
			int yIndex = i->second.position % itemsNumPerColumn + 1;
			QPoint itemPos = QPoint(
				(xIndex - 1) * (itemSize.width()) + xIndex * itemSpacing.column,
				(yIndex - 1) * (itemSize.height()) + yIndex * itemSpacing.line);
			if (i->second.item && itemPos != i->second.item->pos())
				emit i->second.item->moveSignal(itemPos);
		}
	}
}

void MyFileListWidget::createItem(std::wstring name, std::wstring path)
{
	isCreatingItem = true;
	while (isRemovingItem) Sleep(10);
	if(!PathCompletion(path))
		return;
	std::wstring nameWithPath = path;
	nameWithPath += name;
	if (!itemsMap[nameWithPath].item)
	{
		/*新添加的item的图标*/
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
			DWORD_PTR dw_ptr = SHGetFileInfo(nameWithPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
			if (dw_ptr)
				pLWItemImage = QImage::fromHICON(sfi.hIcon);
		}

		/*判断是否已经放置图标*/
		std::wstring::size_type st = indexesState.find(L'0');
		if (st == std::wstring::npos)
		{
			st = indexesState.length();
			indexesState += L'0';
		}
		indexesState.replace(st, 1, L"1");
		if (itemsNumPerColumn == 0)
			itemsNumPerColumn = 1;
		/*计算新添加的item的合适位置的index*/
		int xIndex = st / itemsNumPerColumn + 1;
		int yIndex = st % itemsNumPerColumn + 1;
		/*创建新item*/
		MyFileListItem* pLWItem = new MyFileListItem(this, itemSize);
		pLWItem->setText(wstr2str_2UTF8(name).c_str());
		pLWItem->setViewMode(viewMode);
		pLWItem->setPath(path);
		pLWItem->setImage(pLWItemImage);
		//pLWItem->adjustSize();
		QPoint itemPos = QPoint(
			(xIndex - 1) * (itemSize.width()) + xIndex * itemSpacing.column, 
			(yIndex - 1) * (itemSize.height()) + yIndex * itemSpacing.line);
		pLWItem->move(itemPos);
		//pLWItem->setImage(QImage::fromHICON(sfi.hIcon));
		connect(pLWItem, &MyFileListItem::removeSelfSignal, this, [=]() {
			emit removeItemSignal(pLWItem->text().toStdWString(), pLWItem->getPath());
			});
		/*将新添加的item加入map*/
		auto position = itemsMap[nameWithPath].position;
		itemsMap[nameWithPath] = { pLWItem, name, path, (position == -1 ? (long long)st : position)};
		pLWItem->show();
	}
	writeConfigFile(configFileNameWithPath);
	isCreatingItem = false;
}

void MyFileListWidget::removeItem(std::wstring name, std::wstring path)
{
	isRemovingItem = true;
	while (isCreatingItem) Sleep(10);
	writeConfigFile(configFileNameWithPath);
	isRemovingItem = false;
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

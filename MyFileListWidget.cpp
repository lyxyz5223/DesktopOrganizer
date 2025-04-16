//右键菜单，新建的列表项内容：
//注册表：计算机\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Discardable\PostSetup\ShellNew

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
#include <commoncontrols.h>
// GetIcon
#pragma comment(lib, "Comctl32.lib")

//Qt
#include "MyFileListWidget.h"
#include <QPainter>
#include <qevent.h>
#include <QMenu>
#include <qmessagebox.h>
#include <qdrag.h>
#include <qmimedata.h>
#include "MyMenuAction.h"
#include "MyMenu.h"
#include <QSettings>
#include <qregularexpression.h>

//C++
#include <fstream>
//#include <sstream>
#include <iostream>
#include <vector>
//#include <array>
#include "StringProcess.h"
#include <cstdlib>
#include <thread>
#include <codecvt>
#include <locale>
#include <string>

// DataBase

//My Lib
#include "fileProc.h"
#include <QFileInfo>
#include "FileChangesChecker.h"
#include <QGraphicsDropShadowEffect>
//声明&定义
//注册表
//根目录
#define HKEY_CLASSES_ROOT_STR "HKEY_CLASSES_ROOT"
//新建
#define HKEY_CURRENT_USER_ShellNew_STR "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Discardable\\PostSetup\\ShellNew"
//桌面空处右键
#define HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_STR "HKEY_CLASSES_ROOT\\DesktopBackground"
#define HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_SHELL_STR "HKEY_CLASSES_ROOT\\DesktopBackground\\Shell"
#define HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_SHELLEX_STR "HKEY_CLASSES_ROOT\\DesktopBackground\\shellex"
//文件夹右键
#define HKEY_CLASSES_ROOT_DIRECTORY_STR "HKEY_CLASSES_ROOT\\Directory"
#define HKEY_CLASSES_ROOT_DIRECTORY_SHELL_STR "HKEY_CLASSES_ROOT\\Directory\\shell"
#define HKEY_CLASSES_ROOT_DIRECTORY_SHELLEX_STR "HKEY_CLASSES_ROOT\\Directory\\shellex"
//文件夹空处
#define HKEY_CLASSES_ROOT_DIRECTORY_BACKGROUND_STR "HKEY_CLASSES_ROOT\\Directory\\Background"
#define HKEY_CLASSES_ROOT_DIRECTORY_BACKGROUND_SHELL_STR "HKEY_CLASSES_ROOT\\Directory\\Background\\shell"
#define HKEY_CLASSES_ROOT_DIRECTORY_BACKGROUND_SHELLEX_STR "HKEY_CLASSES_ROOT\\Directory\\Background\\shellex"


long long MyFileListWidget::useCount = 0;// 对象计数

std::mutex MyFileListWidget::mtxConfigFile;
std::mutex MyFileListWidget::mtxWindowsConfigFile;

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

MyFileListWidget::~MyFileListWidget()
{
	//计数-1
	useCount -= 1;
	checkFilesChangeThreadExit = true;
	//for (auto iter = checkFilesChangeThreads.begin(); iter != checkFilesChangeThreads.end(); iter++)
	//	iter->join();
	for (auto iter = fileChangesCheckerList.begin(); iter != fileChangesCheckerList.end(); iter++)
	{
		(*iter)->stop();
		delete* iter;
	}
	fileChangesCheckerList.clear();
	if (itemTaskThread.joinable())
		itemTaskThread.join();
	if (sizeCalculateThread.joinable())
		sizeCalculateThread.join();
}

void MyFileListWidget::changeItemSizeAndNumbersPerColumn()
{
	QScreen* sc = this->screen();
	if (!sc) return;
	QRect re = sc->geometry();
	re = sc->availableGeometry();
	itemSize = QSize(re.width() / zoomScreenWidth, re.width() / zoomScreenWidth *5 / 4);
	itemsNumPerColumn = height() / (itemSize.height() + itemSpacing.line);
	if (itemsNumPerColumn == 0)
		itemsNumPerColumn = 1;
}


void MyFileListWidget::refreshInitialize(QWidget* parent, std::vector<std::wstring> pathsList, std::wstring config)
{
	for (auto iter = pathsList.begin(); iter != pathsList.end(); iter++)
	{
		PathCompletion(*iter);
	}
	this->pathsList = pathsList;
	/*计算item大小和每列item的个数*/
	// 先计算，因为读取配置文件的时候要按照大小创建桌面图标Item
	changeItemSizeAndNumbersPerColumn();
	//读取配置文件
	if (/*pathsList.size() && */!readConfig(config, true))
	{
		QMessageBox::critical(this, "错误", "配置文件读取失败！程序即将退出。");
		return;
	}

	//如果路径列表有内容，就创建Item
	//for (auto i = pathsList.begin(); i != pathsList.end(); i++)
	//	checkFilesChangeThreads.push_back(std::thread(&MyFileListWidget::checkFilesChangeProc, this, *i));
	for (auto i = pathsList.begin(); i != pathsList.end(); i++)
	{
		//开局先检测一遍现有文件与配置文件之间的出入，并更新
		std::thread(&MyFileListWidget::checkFilesChange, this, *i, true).detach();

		//文件监测器，启动！
		FileChangesChecker* fcc = new FileChangesChecker(*i);
		fileChangesCheckerList.push_back(fcc);
		std::wstring path = *i;
		fcc->setCallback([this, path](FileChangesChecker* checker, FileChangesChecker::FileChanges fileChanges, void* parameter) {
			ItemTask itemTask;
			itemTask.args = { fileChanges.oldName, path };
			switch (fileChanges.action)
			{
			case FileChangesChecker::Action::Added://有新的文件
			{
				itemTask.task = ItemTask::Create;
				std::lock_guard<std::mutex> lock(mtxItemTaskQueue);
				addItemTaskIfNotInQueue(itemTask);
				break;
			}
			case FileChangesChecker::Action::Removed://有文件被删除
			{
				itemTask.task = ItemTask::Remove;
				std::lock_guard<std::mutex> lock(mtxItemTaskQueue);
				addItemTaskIfNotInQueue(itemTask);
				break;
			}
			case FileChangesChecker::Action::RenamedNewName://有文件被重命名
			{
				itemTask.args.push_back(fileChanges.newName);
				itemTask.task = ItemTask::Rename;
				std::lock_guard<std::mutex> lock(mtxItemTaskQueue);
				addItemTaskIfNotInQueue(itemTask);
				break;
			}
			default:
				break;
			}
			}, &path);
		fcc->start();
	}
}

void MyFileListWidget::publicInitialize(QWidget* parent,
	std::wstring config, std::wstring childrenWindowsConfig,
	long long windowId, bool showTitle, std::wstring title,
	bool isMainWindow)
{
	//计数+1
	useCount += 1;
	std::cout << UTF8ToANSI("当前MyFileListWidget对象计数: ") << useCount << std::endl;
	// TODO: 子窗口以及配置管理
	this->windowsConfigName = childrenWindowsConfig;
	this->configName = config;
	this->windowId = windowId;
	childrenWindowsMap[windowId].title = title;
	childrenWindowsMap[windowId].window = this;
	//读取Windows配置
	if (childrenWindowsConfig.size() && !readWindowsConfig(childrenWindowsConfig))
	{
		QMessageBox::critical(this, "错误", "窗口配置文件读取失败！程序即将退出。");
		return;
	}

	setWindowTitle(title);

	//读取配置文件
	if (childrenWindowsConfig.size() && readWindowsConfig(childrenWindowsConfig))
	{
		for (auto& i : childrenWindowsMap)
		{
			signed long long id = i.first;
			if (id == windowId)//排除配置文件中的当前窗口
				continue;
			i.second.window = new MyFileListWidget(
				this, //父母窗口
				std::vector<std::wstring>(),//路径列表
				configName + L"_" + std::to_wstring(id),//内容配置文件
				this->windowsConfigName + L"_" + std::to_wstring(id),//窗口配置文件
				id,//windowId窗口id
				true,//是否显示标题
				i.second.title // title标题
			);
			connect(i.second.window, &MyFileListWidget::updateWindowsConfig, this, [=]() { childrenWindowsMap[id].rect = childrenWindowsMap[id].window->geometry(); writeWindowsConfig(windowsConfigName); });
			i.second.window->setGeometry(i.second.rect);
			i.second.window->show();
		}
	}


	//setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint);
	this->parent = parent;
	if (parent)
	{
		// TODO: 修改父母设置方法
		if (!isMainWindow)
			setParent(parent);
		else
			SetParent((HWND)winId(), (HWND)parent->winId());
	}
	//setParent(parent);
	ifShowTitle = showTitle;
	if (ifShowTitle)
		setMinimumHeight(titleBarGeometry.y() + titleBarGeometry.height());
	setIfShowTitle(showTitle);

	setMouseTracking(true);
	installEventFilter(this);
	setAcceptDrops(true);

	// 获取cmd图标
	WCHAR* sysPath = new WCHAR[MAX_PATH];//系统路径
	UINT sysPathLen = GetSystemDirectory(sysPath, MAX_PATH);
	if (sysPathLen > MAX_PATH)
	{
		//err: buffer is too small
	}
	else//cmd图标
	{
		// 1
		//std::wstring cmdPath = sysPath;
		//PathCompletion(cmdPath);
		//cmdPath += L"cmd.exe";
		//SHFILEINFO sfi;
		//DWORD_PTR dw_ptr = SHGetFileInfo(cmdPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
		//if (dw_ptr)
		//	iconCMD = QIcon(QPixmap::fromImage(QImage::fromHICON(sfi.hIcon)));

		// 2
		//HMODULE hShell32 = LoadLibrary(L"shell32.dll");
		//typedef BOOL(WINAPI* ShellGetImageLists)(HIMAGELIST* phLarge, HIMAGELIST* phSmall);
		//typedef BOOL(WINAPI* FileIconInit)   (BOOL fFullInit);
		//ShellGetImageLists ShellGetImageListsProc = (ShellGetImageLists)GetProcAddress(hShell32, (LPCSTR)71);
		//FileIconInit FileIconInitProc = (FileIconInit)GetProcAddress(hShell32, (LPCSTR)660);
		//if (FileIconInitProc != 0)
		//	FileIconInitProc(TRUE);
		//// 获得大图标和小图标的系统图像列表句柄
		//HIMAGELIST phLarge;
		//HIMAGELIST phSmall;
		//ShellGetImageListsProc(&phLarge, &phSmall);
		//HICON hicon = ImageList_GetIcon(phSmall, 10, ILD_NORMAL);
		//iconCMD = QIcon(QPixmap::fromImage(QImage::fromHICON(hicon)));

		// 3 与2相同
		//SHSTOCKICONINFO info{};
		//info.cbSize = sizeof(SHSTOCKICONINFO);
		//HRESULT hr = SHGetStockIconInfo(SIID_SHIELD, SHGSI_ICON | SHGSI_LARGEICON, &info);
		//iconCMD = QIcon(QPixmap::fromImage(QImage::fromHICON(info.hIcon)));

		// 4
		//iconCMD.addFile(":/DesktopOrganizer/img/terminal.ico");
	}

	{
		SHSTOCKICONINFO iconInfo{};
		iconInfo.cbSize = sizeof(SHSTOCKICONINFO);
		HRESULT hr = SHGetStockIconInfo(SIID_FOLDER, SHGSI_ICON | SHGSI_LARGEICON, &iconInfo);
		iconFolder = QIcon(QPixmap::fromImage(QImage::fromHICON(iconInfo.hIcon)));
	}
	{
		SHSTOCKICONINFO iconInfo{};
		iconInfo.cbSize = sizeof(SHSTOCKICONINFO);
		HRESULT hr = SHGetStockIconInfo(SIID_LINK, SHGSI_ICON | SHGSI_LARGEICON, &iconInfo);
		QImage imageLink = QImage::fromHICON(iconInfo.hIcon);
		iconLink = QIcon(QPixmap::fromImage(imageLink.copy(0, imageLink.height() * 3 / 5, imageLink.height() * 2 / 5, imageLink.width() * 2 / 5)));
	}
	//disconnect(this, &MyFileListWidget::createItemSignal, this, &MyFileListWidget::createItem);
	//disconnect(this, &MyFileListWidget::removeItemSignal, this, &MyFileListWidget::removeItem);
	//disconnect(this, &MyFileListWidget::renameItemSignal, this, &MyFileListWidget::renameItem);
	if (connect(this, &MyFileListWidget::createItemSignal, this, &MyFileListWidget::createItem))
	{
#ifdef _DEBUG
		std::cout << "Connect succeeded: connect(this, &MyFileListWidget::createItemSignal, this, &MyFileListWidget::createItem)\n";
#endif // _DEBUG
	}
	if (connect(this, SIGNAL(removeItemSignal(std::wstring, std::wstring)), this, SLOT(removeItem(std::wstring, std::wstring))))
	{
#ifdef _DEBUG
		std::cout << "Connect succeeded: connect(this, &MyFileListWidget::removeItemSignal, this, &MyFileListWidget::removeItem)\n";
#endif // _DEBUG
	}
	if (connect(this, &MyFileListWidget::renameItemSignal, this, &MyFileListWidget::renameItem))
	{
#ifdef _DEBUG
		std::cout << "Connect succeeded: connect(this, &MyFileListWidget::renameItemSignal, this, &MyFileListWidget::renameItem)\n";
#endif // _DEBUG
	}
	//连接窗口配置文件更新信号与槽
	if (connect(this, &MyFileListWidget::updateWindowsConfig, this, [&]() { writeWindowsConfig(windowsConfigName); }))
	{
#ifdef _DEBUG
		std::cout << "Connect succeeded: connect(this, &MyFileListWidget::updateWindowsConfig, this, [&]() { writeWindowsConfig(windowsConfigName); })\n";
#endif // _DEBUG
	}

	itemTaskThread = std::thread(&MyFileListWidget::itemTaskExecuteProc, this);

	sizeCalculateThread = std::thread(
		[&]() {
			while (!checkFilesChangeThreadExit)
			{
				Sleep(100);
				//size_t npcBackup = itemsNumPerColumn;
				/*重新计算item大小和每列item的个数*/
				changeItemSizeAndNumbersPerColumn();
				std::lock_guard<std::mutex> lock(mtxItemsMap);
				for (auto i = itemsMap.begin(); i != itemsMap.end(); i++)
				{
					Index index = calculateIndex(i->second.position);
					QPoint itemPos = calculatePosFromIndex(index);
					if (i->second.item && i->second.windowId == this->windowId)
					{
						if (static_cast<MyFileListItem*>(i->second.item)->size() != itemSize)
							emit static_cast<MyFileListItem*>(i->second.item)->resizeSignal(itemSize);
						if (itemPos != static_cast<MyFileListItem*>(i->second.item)->pos())
						{
							emit static_cast<MyFileListItem*>(i->second.item)->moveSignal(itemPos);
							//if (static_cast<MyFileListItem*>(i->second.item)->text() == "酷呆桌面.lnk")
							{
								std::cout << this->windowId << " ";
							}
						}
					}
				}
			}
		});
	dragArea = new DragArea(itemsNumPerColumn, itemSize, itemSpacing);

}



MyFileListWidget::MyFileListWidget(
	QWidget* parent,//父亲控件
	std::vector<std::wstring> pathsList,//文件路径列表
	std::wstring config,//配置文件
	std::wstring childrenWindowsConfig,//窗口配置文件
	long long windowId,//窗口Id
	bool showTitle,//是否显示标题栏
	std::wstring title,//标题内容
	bool isMainWindow
	)
	// : QWidget(parent)
{
	publicInitialize(parent, config, childrenWindowsConfig, windowId, showTitle, title, isMainWindow);
	refreshInitialize(parent, pathsList, config);
}



void MyFileListWidget::refreshSelf()
{
	exit(0);
}

void MyFileListWidget::openCMD(std::wstring path)
{
	PathCompletion(path);
#ifdef _DEBUG
	std::cout << "Open cmd" << (path == L"" ? " " : ": ") << UTF8ToANSI(wstr2str_2UTF8(path)) << std::endl;
#endif // _DEBUG
	ShellExecute(0, L"open", L"cmd.exe", ((path == L"") ? path : (std::wstring(L"/s /k pushd \"") + path + L"\"")).c_str(), 0, SW_NORMAL);

}
void MyFileListWidget::openPowerShell(std::wstring path)
{
	PathCompletion(path);
#ifdef _DEBUG
	std::cout << "Open PowerShell" << (path == L"" ? " " : ": ") << UTF8ToANSI(wstr2str_2UTF8(path)) << std::endl;
#endif // _DEBUG
	ShellExecute(0, L"open", L"powershell.exe", ((path == L"") ? path : (std::wstring(L"-noexit -command Set-Location -literalPath \"") + path + L"\"")).c_str(), 0, SW_NORMAL);

}
std::vector<MyFileListWidget::Index> MyFileListWidget::getItemsInRange(Range range)
{
	std::vector<Index> items;
	for (long long x = range.start.x; x <= range.end.x; x++)
	{
		for (long long y = range.start.y; y <= range.end.y; y++)
		{
			long long position = calculatePositionNumberFromIndex({ x, y });
			if (positionNameWithPathMap.count(position))
			{
				std::wstring nameWithPath = positionNameWithPathMap[position];
				if (itemsMap.count(nameWithPath))
					items.push_back({ x, y });
			}
		}
	}
	return items;
}

std::vector<MyFileListWidget::Index> MyFileListWidget::getSelectedItemsInRange(Range range)
{
	std::vector<Index> items;
	for (long long x = range.start.x; x <= range.end.x; x++)
	{
		for (long long y = range.start.y; y <= range.end.y; y++)
		{
			long long position = calculatePositionNumberFromIndex({ x, y });
			if (positionNameWithPathMap.count(position))
			{
				std::wstring nameWithPath = positionNameWithPathMap[position];
				if (itemsMap.count(nameWithPath) && itemsMap[nameWithPath].item && static_cast<MyFileListItem*>(itemsMap[nameWithPath].item)->isChecked())
					items.push_back({ x, y });
			}
		}
	}
	return items;
}

void MyFileListWidget::sendCreateItemSignalAndWriteConfig(std::wstring name, std::wstring path)
{
	std::unique_lock<std::mutex> ulMtxItemTask(mtxItemTaskExecute); // 互斥锁管理器，允许在不同线程间传递，允许手动加锁解锁
	std::cout << "Send CreateItem Signal\n";
	emit createItemSignal(name, path);
	cvItemTaskFinished.wait(ulMtxItemTask);
	writeConfig(configName);
}

void MyFileListWidget::sendRemoveItemSignalAndWriteConfig(std::wstring name, std::wstring path)
{
	std::unique_lock<std::mutex> ulMtxItemTask(mtxItemTaskExecute); // 互斥锁管理器，允许在不同线程间传递，允许手动加锁解锁
	std::cout << "Send RemoveItem Signal\n";
	emit removeItemSignal(name, path);
	cvItemTaskFinished.wait(ulMtxItemTask);
	writeConfig(configName);
}

void MyFileListWidget::sendRenameItemSignalAndWriteConfig(std::wstring oldName, std::wstring path, std::wstring newName)
{
	std::unique_lock<std::mutex> ulMtxItemTask(mtxItemTaskExecute); // 互斥锁管理器，允许在不同线程间传递，允许手动加锁解锁
	std::cout << "Send RenameItem Signal\n";
	emit renameItemSignal(oldName, path, newName);
	cvItemTaskFinished.wait(ulMtxItemTask);
	writeConfig(configName);
}

void MyFileListWidget::openProgram(std::wstring exeFilePath, std::wstring parameter, int nShowCmd, std::wstring workDirectory, HWND msgOrErrWindow)
{
	//wchar_t* programPath = nullptr;
	//DWORD co = 1024;
	//HRESULT hr = AssocQueryString(ASSOCF_NONE, ASSOCSTR_COMMAND, extension, NULL, programPath, &co);
	//programPath = new wchar_t[co];
	//hr = AssocQueryString(ASSOCF_NONE, ASSOCSTR_COMMAND, extension, NULL, programPath, &co);
	//if (SUCCEEDED(hr))
	//{
	//	// 输出结果
	//	std::wcout << L"文件扩展名: " << extension << std::endl;
	//	//std::wcout << L"文件类型: " << fileType << std::endl;
	//	std::wcout << L"默认打开程序路径: " << programPath << std::endl;
	//}

	HINSTANCE hInstance = ShellExecute(msgOrErrWindow, L"open", (exeFilePath).c_str(), parameter.c_str(), workDirectory.c_str(), SW_NORMAL);
	if ((INT_PTR)hInstance <= 32)
	{
		//error
#ifdef _DEBUG
		std::cout << UTF8ToANSI("[Error]: 打开失败，Err code: ") << (INT_PTR)hInstance << " ";
		if ((INT_PTR)hInstance == 2)
			std::cout << "File not found!";
		std::cout << "\n";
#endif // 
	}
}

HICON MyFileListWidget::ExtractIconFromRegString(std::wstring regString, HINSTANCE hInst)
{
	using std::wstring;
	WCHAR* tmp = new WCHAR[1];
	DWORD len = ExpandEnvironmentStrings(regString.c_str(), tmp, 0);
	delete[] tmp;
	if (!len)
		assert(true && "err: ExpandEnvironmentStrings 1:get len");
	tmp = new WCHAR[len + 1];
	len = ExpandEnvironmentStrings(regString.c_str(), tmp, len + 1);
	if (!len)
		assert(true && "err: ExpandEnvironmentStrings 2:get str");
	regString = tmp;
	delete[] tmp;

	auto st = regString.find_last_of(L',');
	if (st == wstring::npos)
	{
		HICON res = ExtractIcon(hInst, regString.c_str(), 0);
#ifdef _DEBUG
		if (!res)
			std::cout << "Uninclude any icon resource: " << wstr2str_2ANSI(regString) << std::endl;
		else if ((INT)res == 1)
			std::cout << "The file specified was not an executable file, DLL, or icon file: " << wstr2str_2ANSI(regString) << std::endl;
#endif // _DEBUG
		return res;
	}
	wstring filePath = regString.substr(0, st);
	wstring tmpIndexStr = regString.substr(st + 1);
	wstring indexStr;
	for (wchar_t c : tmpIndexStr)
		if (c >= L'0' && c <= L'9')
			indexStr += c;
		else if (c == L'-')
			indexStr += c;
	unsigned int index = 0;
	try {
		index = std::stoul(indexStr);
	}
	catch(std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	HICON res = ExtractIcon(hInst, filePath.c_str(), index);
#ifdef _DEBUG
	if (!res)
		std::cout << "Uninclude any icon resource: " << wstr2str_2ANSI(regString) << std::endl;
	else if ((INT)res == 1)
		std::cout << "The file specified was not an executable file, DLL, or icon file: " << wstr2str_2ANSI(regString) << std::endl;
#endif // _DEBUG
	return res;
}

std::wstring MyFileListWidget::LoadDllStringFromRegString(std::wstring regString)
{
	{
		using std::wstring;
		wstring text;
		auto st = regString.find_last_of(L',');
		if (st == wstring::npos)
			return text;
		wstring filePath = regString.substr(0, st);
		HMODULE hModule = LoadLibrary(filePath.c_str());
		if (!hModule)
		{
#ifdef _DEBUG
			std::cerr << "LoadLibrary: " << wstr2str_2ANSI(filePath)
				<< ", GetLastError() return code:" << GetLastError()
				<< std::endl;
#endif
			return text;
		}
		wstring tmpIndexStr = regString.substr(st + 1);
		wstring indexStr;
		for (wchar_t c : tmpIndexStr)
			if (c >= L'0' && c <= L'9')
				indexStr += c;
			//else if (c == L'-')
			//	indexStr += c;
		unsigned int index = 0;
		try {
			index = std::stoul(indexStr);
		}
		catch (std::exception e) {
			std::cerr << e.what() << std::endl;
		}
		WCHAR* tmp = new WCHAR[8];
		int textLength = LoadString(hModule, index, tmp, 0);
		delete[] tmp;
		if (!textLength)
		{
#ifdef _DEBUG
			std::cerr << "Uninclude any string in: " << wstr2str_2ANSI(filePath) << std::endl;
#endif
			return text;
		}
		WCHAR* buffer = new WCHAR[textLength + 1];
		int gottenCharNum = LoadString(hModule, index, buffer, textLength + 1);
#ifdef _DEBUG
		if (!gottenCharNum)
			std::cerr << "Uninclude any string in: " << wstr2str_2ANSI(filePath) << std::endl;
#endif
		if (gottenCharNum < textLength + 1)
			buffer[gottenCharNum] = 0;
		text += buffer;
		delete[] buffer;
		return text;
	}
}

void MyFileListWidget::addActionsFromRegedit(QString path, MyMenu* menu)
{
	QSettings regDirectoryBackground(path, QSettings::NativeFormat);
	QStringList keys = regDirectoryBackground.childGroups();
	for (QString key : keys)
	{
		//QSettings re = regDirectoryBackground.findChild<QSettings>(key);
		QSettings re(QString(path) + "\\" + key, QSettings::NativeFormat);
		QVariant res = re.value(".", true);
		QString name;
		if (res.typeId() == QMetaType::Bool && res.toBool())
		{
			//continue;
			res = re.value("MUIVerb");
			if (res.typeId() == QMetaType::QString)
				name = res.toString();
			else
				name = key;
		}
		else if (res.typeId() == QMetaType::QString)
		{
			name = res.toString();
			name.remove(QRegularExpression("^ +\\s*"));
			if (name.size() && name[0] != '@')
			{
			}
			else//链接了的字符串
			{
				name.removeFirst();
				//展开注册表路径中的环境变量
				WCHAR* tmp = new WCHAR[1];
				DWORD len = ExpandEnvironmentStrings(name.toStdWString().c_str(), tmp, 0);
				delete[] tmp;
				if (!len)
					assert(true && "err: ExpandEnvironmentStrings");
				tmp = new WCHAR[len + 1];
				len = ExpandEnvironmentStrings(name.toStdWString().c_str(), tmp, len + 1);
				if (!len)
					assert(true && "err: ExpandEnvironmentStrings");
				name = QString::fromWCharArray(tmp);
				delete[] tmp;
				name = QString::fromStdWString(LoadDllStringFromRegString(name.toStdWString()));
			}
			//此处应该处理加速键(&)
			QString tmp = name;
			name.clear();
			bool isEscapeChar = false;
			for (QChar c : tmp)
			{
				if (c == '&' && (!isEscapeChar))
				{
					isEscapeChar = true;
					continue;
				}
				name += c;
				isEscapeChar = false;
			}
		}
		else
			continue;
		//QSettings cmd = re.findChild<QSettings>("command");
		QSettings cmd(QString(path) + "\\" + key + "\\command", QSettings::NativeFormat);
		QVariant cmdContents = cmd.value(".", true);
		QString cmdStr = "";
		if (cmdContents.typeId() == QMetaType::Bool && cmdContents.toBool())
		{
			//continue;
			//此处应该处理SettingsURI命令
			QSettings cmd(QString(path) + "\\" + key, QSettings::NativeFormat);
			QVariant cmdContents = cmd.value("SettingsURI");
			if (cmdContents.typeId() == QMetaType::QString)
				cmdStr = cmdContents.toString();
		}
		else if (cmdContents.typeId() == QMetaType::QString)
			cmdStr = cmdContents.toString();

		std::vector<std::wstring> cmdVec = splitForShellExecuteFromRegedit(cmdStr.toStdWString(), L" ");
		if (!cmdVec.size())
			continue;
		bool shouldCreateSubMenu = false;//检测是否创建子菜单
		std::wstring exeFilePath = cmdVec[0];
		cmdVec.erase(cmdVec.begin());//去除vector中第一项：程序路径


		//%V通常是当前文件(夹)的路径
		//%L为长目录
		//%W为工作目录
		std::vector<std::wstring> rep = { L"%V", L"%L", L"%1", L"%W", L"%v", L"%l", L"%w" };
		std::vector<std::wstring> sign = { L"\"", L"'", L""};
		std::vector<size_t> toReplaceTextIndex;
		size_t cmdVecSize = cmdVec.size();
		for (size_t i = 0; i < cmdVecSize; i++)
		{
			std::wstring upperCmdParameter = cmdVec[i];
			for (std::wstring r : rep)
			{
				//TODO: 此处应该处理转义字符，但仍未实现
				if (upperCmdParameter.find(r) != std::wstring::npos)
				{
					shouldCreateSubMenu = true;
					toReplaceTextIndex.push_back(i);
				}
			}

		}
		QIcon itemIcon;
		QVariant iconSourceVariant = re.value("Icon");
		if (iconSourceVariant.typeId() == QMetaType::QString)
		{
			QString iconSource = iconSourceVariant.toString();
			itemIcon = ExtractQIconFromRegString(iconSource);
		}
		MyMenuAction* item = new MyMenuAction(itemIcon, name);
		if (shouldCreateSubMenu)
		{
			MyMenu* sub = new MyMenu(menu);
			for (auto iter = pathsList.begin(); iter != pathsList.end(); iter++)
			{
				auto tmpVec = cmdVec;
				for (size_t i : toReplaceTextIndex)
				{
					for (auto iterRep = rep.begin(); iterRep != rep.end(); iterRep++)
					{
						auto st = tmpVec[i].find(*iterRep);
						if (st != std::wstring::npos)
							tmpVec[i] = tmpVec[i].replace(st, iterRep->size(), iter->c_str());
					}
				}

				QString cmdQStr = QString::fromStdWString(join(tmpVec, L" "));
				sub->addAction(QString::fromStdWString(*iter), this, [=]() {
					openProgram(exeFilePath, cmdQStr.toStdWString());
					});
			}
			item->setMenu(sub);
		}
		else
		{
			QString cmdQStr = QString::fromStdWString(join(cmdVec, L" "));
			connect(item, &MyMenuAction::triggered, this, [=]() {
				openProgram(exeFilePath, cmdQStr.toStdWString());
				});
		}
		menu->addAction(item);
	}
}


void MyFileListWidget::showDesktopOldPopupMenu(QPoint pos)
{
	MyMenu* menu = new MyMenu(this);
	addActionsFromRegedit(HKEY_CLASSES_ROOT_DIRECTORY_BACKGROUND_SHELL_STR, menu);

	//QSettings regDesktopBackground(HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_SHELL_STR, QSettings::NativeFormat);
	//keys = regDesktopBackground.childGroups();
	//for (QString key : keys)
	//{
	//	QString keyPath = QString(HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_SHELL_STR) + "\\" + key;
	//	QSettings keyReg(keyPath, QSettings::NativeFormat);
	//	QVariant keyVar = keyReg.value(".", true);
	//	if (keyVar.typeId() == QMetaType::Bool)
	//		continue;
	//	else if (keyVar.typeId() == QMetaType::QString)
	//	{

	//	}
	//}
	menu->addSeparator();
	addActionsFromRegedit(HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_SHELL_STR, menu);
	menu->exec(pos);
}

std::wstring MyFileListWidget::getTemplateFileNameWithPathFromReg(std::wstring extension)
{
	std::wstring templateFileNameWithPath;
	QSettings extensionReg(QString::fromStdWString(TEXT(HKEY_CLASSES_ROOT_STR)L"\\" + extension), QSettings::NativeFormat);
	QVariant regValue = extensionReg.value(".", true);
	if (regValue.typeId() == QMetaType::QString)//获取子级目录名字
	{
		QString subName = regValue.toString();
		QSettings extensionRegSubReg(
			QString::fromStdWString(
				TEXT(HKEY_CLASSES_ROOT_STR)L"\\"
				+ extension) + "\\"
			+ subName + "\\ShellNew",
			QSettings::NativeFormat);
		regValue = extensionRegSubReg.value("FileName");
		if (regValue.typeId() == QMetaType::QString)
		{
			QString templateFileNameWithPathQStr = regValue.toString();
			templateFileNameWithPath = templateFileNameWithPathQStr.toStdWString();
		}
	}
	//else if (regValue.typeId() == QMetaType::Bool && regValue.toBool())//如果默认没有内容
	//{

	//}
	return templateFileNameWithPath;
}

bool MyFileListWidget::newFileProc(std::wstring extension, std::wstring path)
{
	std::wstring fileDescription;
	SHFILEINFO info;
	if (SHGetFileInfo(extension.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES | SHGFI_ICON))
	{
		fileDescription = info.szTypeName;
		fileDescription += extension;
		return createNewFile(fileDescription, path, getTemplateFileNameWithPathFromReg(extension));
	}
	return false;
}

bool MyFileListWidget::createNewFile(std::wstring newFileName, std::wstring path, std::wstring templateFileNameWithPath)
{
	//templateFileNameWithPath = L"EXCEL12.xlsx";
	newFileName = L"新建 " + newFileName; // 文件名
	PathCompletion(path);
	std::wstring::size_type extPos = newFileName.find_last_of(L".");
	std::wstring newFileNameNoExtension = newFileName;
	std::wstring newFileExtension;
	if (extPos != std::wstring::npos)
	{
		newFileNameNoExtension = newFileName.substr(0, extPos);
		newFileExtension = newFileName.substr(extPos);
	}


	//先读取模板文件
	DWORD dwDesiredAccess = GENERIC_READ; // 访问模式
	DWORD dwShareMode = FILE_SHARE_READ; // 共享模式
	LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL; // 安全属性
	DWORD dwCreationDisposition = OPEN_EXISTING; // 创建方式
	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL; // 文件属性和标志
	HANDLE hTemplateFile = NULL; // 模板文件句柄
	HANDLE newFileHandle = NULL; // 新文件句柄
#define OPENTEMPLATEFILE CreateFile(templateFileNameWithPath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
#define CREATEFILE CreateFile((path + newFileName).c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
#define CREATEFILEC(count) CreateFile((path + newFileNameNoExtension + L" (" + std::to_wstring(count) + L")" + newFileExtension).c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)

	//if (templateFileNameWithPath != L"")
	//{
	//	hTemplateFile = OPENTEMPLATEFILE;
	//	if (INVALID_HANDLE_VALUE == hTemplateFile)
	//		return false;//打开模板文件失败
	//}
	if (templateFileNameWithPath != L"")
	{
		BOOL bSuccess = CopyFile(templateFileNameWithPath.c_str(), (path + newFileName).c_str(), TRUE);
		unsigned long long count = 1;
		while (!bSuccess)
		{
			if (count == (unsigned long long)0)
				return false;
			bSuccess = CopyFile(templateFileNameWithPath.c_str(), (path + newFileNameNoExtension + L" (" + std::to_wstring(count) + L")" + newFileExtension).c_str(), TRUE);
			count++;
		}
		return true;
	}
	else
	{
		dwDesiredAccess = GENERIC_WRITE;
		dwShareMode = FILE_SHARE_WRITE;
		lpSecurityAttributes = NULL;
		dwCreationDisposition = CREATE_NEW;
		dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
		//hTemplateFile = hTemplateFile;
		//按照模板文件创建新文件
		if (INVALID_HANDLE_VALUE == (newFileHandle = CREATEFILE))
		{
			unsigned long long count = 1;
			while (INVALID_HANDLE_VALUE == (newFileHandle = CREATEFILEC(count)))
			{
				if (count == (unsigned long long)0)
				{
					if (hTemplateFile)
						CloseHandle(hTemplateFile);
					return false;
				}
				count++;
			}
		}
		if (hTemplateFile)
			CloseHandle(hTemplateFile);
		CloseHandle(newFileHandle);

		return true;
	}
}


void MyFileListWidget::MenuClickedProc(QAction* action)
{
	//if (action->text() == "刷新");
}

void MyFileListWidget::CreateDesktopPopupMenu()
{
	QPoint curPos = QCursor::pos();
	//创建菜单
	MyMenu* menu1 = new MyMenu(this);
	MyMenuAction* actionRefresh = new MyMenuAction(iconRefresh, "刷新\tE");
	connect(actionRefresh, &MyMenuAction::triggered, this, &MyFileListWidget::refreshSelf);
	MyMenuAction* pasteAction = new MyMenuAction(iconPaste, "粘贴\tP");
	connect(pasteAction, &MyMenuAction::triggered, this, &MyFileListWidget::pasteProc);
	MyMenuAction* newFileOrFolderAction = new MyMenuAction(iconCirclePlus, "新建\tW");


	//-------------------------------------------------------------------------------------------------
	//二级菜单：Desktop Organizer
	MyMenuAction* desktopOrganizerAction = new MyMenuAction(QIcon(), "Desktop Organizer");
	MyMenu* desktopOrganizerMenu = new MyMenu(menu1);
	desktopOrganizerMenu->addAction(
		"Create new box",
		this,
		[&]() {
			//TODO: 菜单项创建新窗口
			signed long long wid = getAvailableWindowId();
			auto& newWindow = childrenWindowsMap[wid] = {};
			newWindow.title = L"New Box";
			newWindow.window = new MyFileListWidget(
				this, //父母窗口
				std::vector<std::wstring>(),//路径列表
				configName + L"_" + std::to_wstring(wid),//内容配置文件
				this->windowsConfigName + L"_" + std::to_wstring(wid),//窗口配置文件
				wid,//windowId窗口id
				true,//是否显示标题
				newWindow.title // title标题
			);
			connect(newWindow.window, &MyFileListWidget::updateWindowsConfig, this, [=]() { childrenWindowsMap[wid].rect = childrenWindowsMap[wid].window->geometry(); writeWindowsConfig(windowsConfigName); });
			newWindow.rect = QRect(mapFromGlobal(curPos), size() / 3);
			newWindow.window->setGeometry(newWindow.rect);
			newWindow.window->show();
		}
	);
	desktopOrganizerAction->setMenu(desktopOrganizerMenu);
	//二级菜单：Widget控制
	bool bClose = false;
	MyMenuAction* myFileListWidgetAction = new MyMenuAction(QIcon(), "Widget Control");
	MyMenu* widgetControlMenu = new MyMenu(menu1);
	widgetControlMenu->addAction("关闭Widget", this, [&] { /*MyFileListWidget::*/bClose = true; });
	widgetControlMenu->addAction("退出软件", this, []() {exit(0); });
	widgetControlMenu->addAction("(主窗口已嵌入桌面)")->setEnabled(false);
	widgetControlMenu->addAction("脱离主窗口", this, [&]() {
		SetParent((HWND)this->winId(), 0);
		});
	widgetControlMenu->addAction("嵌入主窗口", this, [&]() {
		SetParent((HWND)this->winId(), (HWND)parent->winId());
		});
	widgetControlMenu->addAction("取消无边框窗口属性", this, [&]() {
		QRect rect(geometry());
		this->setWindowFlags(this->windowFlags() & (~Qt::FramelessWindowHint));
		move(rect.topLeft());
		resize(rect.size());
		show();
		});
	widgetControlMenu->addAction("添加无边框窗口属性", this, [&]() {
		QRect rect(geometry());
		this->setWindowFlags(this->windowFlags() | (Qt::FramelessWindowHint));
		move(rect.topLeft());
		resize(rect.size());
		show();
		});
	myFileListWidgetAction->setMenu(widgetControlMenu);


	//二级菜单：新建
	MyMenu* newOptions = new MyMenu(menu1);
	newOptions->addAction(iconFolder, "文件夹");
	newOptions->addAction(iconLink, "快捷方式");
	newFileOrFolderAction->setMenu(newOptions);
	newOptions->addSeparator();
	//newOptions->addAction("txt文本文档");
	//获取右键新建文件子菜单列表项
	QSettings shellNewContents(HKEY_CURRENT_USER_ShellNew_STR, QSettings::NativeFormat);
	QVariant extensionsValue = shellNewContents.value("Classes");
	if (extensionsValue.typeId() == QMetaType::QStringList)
	{
		QStringList extensionList = extensionsValue.toStringList();
		extensionList.insert(0, ".txt");
		std::cout << UTF8ToANSI("新建一栏扩展名：");
		for (qsizetype i = 0; i < extensionList.size(); i++)
		{
			std::cout << extensionList[i].toStdString() << " ";

			std::wstring extension = extensionList[i].toStdWString();
			if (extension == L".library-ms" || extension == L".lnk"/* || extension == L"Folder"*/)
				continue;//排除指定文件类型
			if (extension.length() > 0)
			{
				SHFILEINFO info;
				if (SHGetFileInfo(extension.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES | SHGFI_ICON))
				{
					std::wstring type = info.szTypeName;
					QIcon icon = QPixmap::fromImage(QImage::fromHICON(info.hIcon));

					newOptions->addAction(icon, QString::fromStdWString(type),
						QKeySequence(),//加速键
						this, [=]() {
							if (pathsList.size() == 1)
							{
								newFileProc(extension, pathsList.front());
							}
							else if (pathsList.size() > 1)
							{
								MyMenu* newFilePathSelectionMenu = new MyMenu(this);
								for (auto iter = pathsList.begin(); iter != pathsList.end(); iter++)
								{
									newFilePathSelectionMenu->addAction(QIcon(), QString::fromStdWString(*iter),
										QKeySequence(),
										this, [=]() {
											newFileProc(extension, *iter);
										});
								}

								newFilePathSelectionMenu->exec(QCursor::pos());
								newFilePathSelectionMenu->deleteLater();
							}

						});
				}
			}

			// 注册表法通过文件后缀获取文件类型描述
			//QSettings filePathAtRegister(QString(HKEY_CLASSES_ROOT_STR) + "\\" + vl[i], QSettings::NativeFormat);
			//QVariant fn = filePathAtRegister.value(".", true);
			//if (fn.typeId() == QMetaType::Bool && fn.toBool())
			//	continue;
			//QSettings fileDescription(QString(HKEY_CLASSES_ROOT_STR) + "\\" + fn.toString(), QSettings::NativeFormat);
			//fn = fileDescription.value(".", true);
			//if (fn.typeId() == QMetaType::Bool && fn.toBool())
			//	continue;
			//QString fileDescriptionText = fn.toString();
			//newOptions->addAction(fileDescriptionText);
		}
		std::cout << std::endl;
	}


	MyMenuAction* displayFullActions = new MyMenuAction("显示全部选项");
	connect(displayFullActions, &MyMenuAction::triggered, this, [=]() {
		showDesktopOldPopupMenu(curPos);
		});

	// 菜单添加列表项
	menu1->addAction(actionRefresh);
	menu1->addSeparator();
	//menu1->addAction(iconCMD, "打开cmd\n    - 程序运行的工作路径\tO", this, [=]() { MyFileListWidget::openCMD(L""); });
	//for (std::wstring path : pathsList)
	//{
	//	MyMenuAction* openCMDAction = new MyMenuAction();
	//	openCMDAction->setText(QString("打开cmd\n    - ") + QString::fromStdWString(path));
	//	openCMDAction->setIcon(iconCMD);
	//	connect(openCMDAction, &MyMenuAction::triggered, this, [=]() { MyFileListWidget::openCMD(path); });
	//	menu1->addAction(openCMDAction);
	//}
	MyMenuAction* cmdAction = new MyMenuAction(iconCMD, "打开cmd");
	MyMenu* cmdListMenu = new MyMenu(menu1);
	cmdListMenu->addAction("程序运行的工作路径", this, [=]() { MyFileListWidget::openCMD(L""); });
	for (std::wstring path : pathsList)
		cmdListMenu->addAction(QString::fromStdWString(path), this, [=]() { MyFileListWidget::openCMD(path); });
	cmdAction->setMenu(cmdListMenu);
	menu1->addAction(cmdAction);

	menu1->addSeparator();
	menu1->addAction(pasteAction);
	menu1->addSeparator();
	menu1->addAction(desktopOrganizerAction);
	menu1->addAction(myFileListWidgetAction);
	menu1->addSeparator();
	menu1->addAction(newFileOrFolderAction);
	menu1->addSeparator();
	menu1->addAction(displayFullActions);// 添加二级菜单

	connect(menu1, &QMenu::triggered, this, &MyFileListWidget::MenuClickedProc);
	menu1->setCursorPos(curPos);
	menu1->exec(curPos);
	disconnect(menu1, &QMenu::triggered, this, &MyFileListWidget::MenuClickedProc);
	menu1->deleteLater();
	if (bClose)
		close();
}

signed long long MyFileListWidget::getAvailableWindowId()
{
	signed long long wid = 0;
	{
		std::lock_guard<std::mutex> lock(mtxChildrenWindowsMap);
		while ((wid++) >= 0)//wid为负数时表示id已经被占用完
		{
			if (!childrenWindowsMap.count(wid))
				break;
		}
		if (wid < 0)
		{
			QMessageBox::critical(this, "Error", "No available window id!");
		}
	}
	return wid;
}


void MyFileListWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() == Qt::MouseButton::NoButton)
	{
		QPoint pos = e->pos();
		if (pos.x() < borderWidth)//左边界
		{
			if (pos.y() < borderWidth)//左上角
				setCursor(Qt::SizeFDiagCursor);//左上角或右下角
			else if (pos.y() > height() - borderWidth)//左下角
				setCursor(Qt::SizeBDiagCursor);//左下角或右上角
			else//左边
				setCursor(Qt::SizeHorCursor);
		}
		else if (pos.x() > width() - borderWidth)//右边界
		{
			if (pos.y() < borderWidth)//右上角
				setCursor(Qt::SizeBDiagCursor);
			else if (pos.y() > height() - borderWidth)//右下角
				setCursor(Qt::SizeFDiagCursor);
			else//右边
				setCursor(Qt::SizeHorCursor);
		}
		else if (pos.y() < borderWidth)//上边界
			setCursor(Qt::SizeVerCursor);
		else if (pos.y() > height() - borderWidth)//下边界
			setCursor(Qt::SizeVerCursor);
		else
		{
			windowMoveResize.resizeDirection = windowMoveResize.None;
			setCursor(Qt::CustomCursor);
		}
	}
	if (e->buttons() & Qt::LeftButton)
	{
		//鼠标左键拖动
		{
			QRect geo = QRect(
				windowMoveResize.originGeo.topLeft(),
				windowMoveResize.originGeo.size()
				);
			QCursor cursor;
			QPoint mouseOffset = cursor.pos() - windowMoveResize.mouseDownPos;
			int left = windowMoveResize.originGeo.x() + mouseOffset.x();
			int top = windowMoveResize.originGeo.y() + mouseOffset.y();
			//int right = originGeo.right() + mouseOffset.x();
			//int bottom = originGeo.bottom() + mouseOffset.y();
			int wid = windowMoveResize.originGeo.width() + mouseOffset.x();
			int hei = windowMoveResize.originGeo.height() + mouseOffset.y();
			if (windowMoveResize.resizeDirection & windowMoveResize.Left)
				geo.setX((left < geo.right() - 2 * borderWidth) ? left : (geo.right() - 2 * borderWidth));
			if (windowMoveResize.resizeDirection & windowMoveResize.Top)
				geo.setY((top < geo.bottom() - 2 * borderWidth) ? top : (geo.bottom() - 2 * borderWidth));
			if (windowMoveResize.resizeDirection & windowMoveResize.Right)
				geo.setWidth((wid > 2 * borderWidth) ? wid : (2 * borderWidth));
			if (windowMoveResize.resizeDirection & windowMoveResize.Bottom)
				geo.setHeight((hei > 2 * borderWidth) ? hei : (2 * borderWidth));
			//窗口移动
			if (windowMoveResize.move)
			{
				geo.moveTo(geo.topLeft() + mouseOffset);
				qDebug() << "WindowMove: " << geo;
				setGeometry(geo);
			}
			if (windowMoveResize.resizeDirection != windowMoveResize.None)
				setGeometry(geo);
		}

	}
	//鼠标拖动事件，框选item
	if (!windowMoveResize.move && (e->buttons() & (Qt::LeftButton | Qt::RightButton)))
	if (windowMoveResize.resizeDirection == windowMoveResize.None && !titleBarFrameGeometry.contains(e->pos()))
	{
		//TODO: 拖动框选item
		QPoint mdPos = selectionArea->getMouseDownPos();
		QPoint mPos = QCursor::pos();
		QRect selectionAreaGeometry;
		QPoint topLeft(mdPos.x(), mdPos.y());
		int width = mPos.x() - mdPos.x();
		int height = mPos.y() - mdPos.y();
		if (width < 0)
		{
			width = -width;
			topLeft.setX(mPos.x());
		}
		if (height < 0)
		{
			height = -height;
			topLeft.setY(mPos.y());
		}
		topLeft = mapFromGlobal(topLeft);//坐标变换
		selectionAreaGeometry.setRect(topLeft.x(), topLeft.y(), width, height);
		selectionArea->setGeometry(selectionAreaGeometry);
		//qDebug() << "SelectionArea Geometry: " << selectionAreaGeometry;
		emit selectionAreaResized(selectionAreaGeometry);
	}
}


void MyFileListWidget::mousePressEvent(QMouseEvent* e)
{
	raise();
	setFocus();

	//窗口内部框选item
	{
		selectionArea->raise();
		selectionArea->setMouseDownPos(QCursor::pos());
	}

	switch (e->button())
	{
	case Qt::MouseButton::LeftButton:
	{
		if (windowMoveResize.resizeDirection == windowMoveResize.None && !titleBarFrameGeometry.contains(e->pos()))
		{
			selectionArea->reset();
			emit selectionAreaResized(selectionArea->getGeometry());
		}
		//记录鼠标按下时候的数据
		//窗口移动与大小
		{
			windowMoveResize.originGeo = QRect(geometry().topLeft(), QSize(geometry().size()));
			windowMoveResize.mouseDownPos = QCursor::pos();
			windowMoveResize.move = false;
		}

		//当前窗口的大小修改（拖动边框预处理）
		{
			//GetWindowRect((HWND)this->winId(), &windowMoveResize.originGeo);
			//GetCursorPos(&windowMoveResize.mouseDownPos);
			QPoint pos = e->pos();
			if (pos.x() < borderWidth)//左边界
			{
				if (pos.y() < borderWidth)//左上角
					windowMoveResize.resizeDirection = windowMoveResize.TopLeft;
				else if (pos.y() > height() - borderWidth)//左下角
					windowMoveResize.resizeDirection = windowMoveResize.BottomLeft;
				else//左边
					windowMoveResize.resizeDirection = windowMoveResize.Left;
			}
			else if (pos.x() > width() - borderWidth)//右边界
			{
				if (pos.y() < borderWidth)//右上角
					windowMoveResize.resizeDirection = windowMoveResize.TopRight;
				else if (pos.y() > height() - borderWidth)//右下角
					windowMoveResize.resizeDirection = windowMoveResize.BottomRight;
				else//右边
					windowMoveResize.resizeDirection = windowMoveResize.Right;
			}
			else if (pos.y() < borderWidth)//上边界
				windowMoveResize.resizeDirection = windowMoveResize.Top;
			else if (pos.y() > height() - borderWidth)//下边界
				windowMoveResize.resizeDirection = windowMoveResize.Bottom;
			else
			{
				windowMoveResize.resizeDirection = windowMoveResize.None;
				if (titleBarFrameGeometry.contains(e->pos()))
					windowMoveResize.move = true;
			}
		}
		break;
	}
	case Qt::MouseButton::RightButton:
	{
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
		//窗口状态重置
		windowMoveResize.resizeDirection = windowMoveResize.None;
		windowMoveResize.move = false;

		//将 selectionArea 的位置和大小重置为0
		selectionArea->reset();
	}
		break;
	case Qt::MouseButton::RightButton:
	{
		CreateDesktopPopupMenu();
	}
		break;
	default:
		break;
	}
}

void MyFileListWidget::focusInEvent(QFocusEvent* e)
{
	if (!pathsList.size())
	{
		borderWidth = 6;
		backgroundColor.setAlpha(backgroundColor.alpha() + 50);
		update();
	}
}
void MyFileListWidget::focusOutEvent(QFocusEvent* e)
{
	if (!pathsList.size())
	{
		borderWidth = 3;
		backgroundColor.setAlpha(backgroundColor.alpha() - 50);
		update();
	}
}


void MyFileListWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	//QPen pen(QColor(
	//	255 - backgroundColor.red(),
	//	255 - backgroundColor.green(),
	//	255 - backgroundColor.blue(),
	//	255), borderWidth);
	if (!borderColor.isValid())
	{
		borderColor = QColor(
			255 - backgroundColor.red(),
			255 - backgroundColor.green(),
			255 - backgroundColor.blue(),
			255);
	}
	//背景颜色
	p.fillRect(rect(), backgroundColor);
	//边框颜色
	QPen pen(borderColor, borderWidth);
	p.save();
	p.setPen(pen);
	p.drawRect(rect());
	p.restore();
	//标题栏自绘
	if (ifShowTitle)
	{
		p.save();
		QPen tPen = pen;
		tPen.setWidth(pen.width() / 2);
		p.setPen(tPen);
		QLine line;
		line.setLine(0, 0, 0, 0);
		QRect titleBarGeometry = this->titleBarGeometry;
		if (titleBarGeometry.width() == -1)
			titleBarGeometry.setWidth(width());
		if (titleBarGeometry.height() == -1)
			titleBarGeometry.setHeight(height());
		QRect& textRect = titleBarGeometry;
		switch (titleBarPositionMode)
		{
		default:
		case TitleBarPositionMode::Coord:
			line.setPoints(
				QPoint(titleBarGeometry.x(),
				titleBarGeometry.y() + titleBarGeometry.height()),
				QPoint(titleBarGeometry.x() + titleBarGeometry.width(),
					titleBarGeometry.y() + titleBarGeometry.height())
			);
			break;
		case TitleBarPositionMode::BottomLeft:
			line.setPoints(
				QPoint(0, height() - titleBarGeometry.height()),
				QPoint(titleBarGeometry.width(), height() - titleBarGeometry.height())
			);
			titleBarGeometry.moveTo(0, height() - titleBarGeometry.height());
			break;
		case TitleBarPositionMode::TopLeft:
			line.setPoints(
				QPoint(0, titleBarGeometry.height()),
				QPoint(titleBarGeometry.width(), titleBarGeometry.height())
			);
			titleBarGeometry.moveTo(0, 0);
			break;
		case TitleBarPositionMode::BottomCenter:
			line.setPoints(
				QPoint(
					(width() - titleBarGeometry.width()) / 2,
					height() - titleBarGeometry.height()
				),
				QPoint(
					(width() - titleBarGeometry.width()) / 2 + titleBarGeometry.width(),
					height() - titleBarGeometry.height()
				)
			);
			titleBarGeometry.moveTo(
				(width() - titleBarGeometry.width()) / 2,
				height() - titleBarGeometry.height()
			);
			break;
		case TitleBarPositionMode::TopCenter:
			line.setPoints(
				QPoint(
					(width() - titleBarGeometry.width()) / 2,
					titleBarGeometry.height()
				),
				QPoint(
					(width() - titleBarGeometry.width()) / 2 + titleBarGeometry.width(),
					titleBarGeometry.height()
				)
			);
			titleBarGeometry.moveTo((width() - titleBarGeometry.width()) / 2, 0);
			break;
		case TitleBarPositionMode::BottomRight:
			line.setPoints(
				QPoint(
					width() - titleBarGeometry.width(),
					height() - titleBarGeometry.height()
				),
				QPoint(width(), height() - titleBarGeometry.height())
			);
			titleBarGeometry.moveTo(
				width() - titleBarGeometry.width(),
				height() - titleBarGeometry.height()
			);
			break;
		case TitleBarPositionMode::TopRight:
			line.setPoints(
				QPoint(
					width() - titleBarGeometry.width(),
					titleBarGeometry.height()
				),
				QPoint(width(), titleBarGeometry.height())
			);
			titleBarGeometry.moveTo(width() - titleBarGeometry.width(), 0);
			break;
		}
		titleBarFrameGeometry = titleBarGeometry;
		//p.drawLine(line);
		p.drawRect(titleBarGeometry);
		p.setPen(QPen(textColor));
		p.drawText(textRect, windowTitle(), QTextOption(Qt::AlignCenter));
		p.restore();
	}

	QWidget::paintEvent(e);

	//绘制拖动多选框
	//p.fillRect(selectionArea.getGeometry(), selectionArea.getBackgroundColor());
	//p.save();
	//pen = QPen(selectionArea.getBorderColor(), selectionArea.getBorderWidth());
	//p.setPen(pen);
	//p.drawRect(selectionArea.getGeometry());
	//p.restore();
}

void MyFileListWidget::moveEvent(QMoveEvent* e)
{
	childrenWindowsMap[windowId].rect.moveTo(e->pos());
	writeWindowsConfig(windowsConfigName);
	emit updateWindowsConfig();
}

void MyFileListWidget::resizeEvent(QResizeEvent* e)
{
	childrenWindowsMap[windowId].rect.setSize(e->size());
	writeWindowsConfig(windowsConfigName);
	emit updateWindowsConfig();
}

bool MyFileListWidget::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
	MSG* msg = (MSG*)message;
	switch (msg->message)
	{
	case WM_NCHITTEST:
	{
		//if (canResize)
		//{
		//	QCursor cursor = this->cursor();
		//	QPoint pos = mapFromGlobal(cursor.pos());
		//	int xPos = pos.x();
		//	int yPos = pos.y();
		//	if (xPos < borderWidth && yPos < borderWidth)//左上角
		//		*result = HTTOPLEFT;
		//	else if (xPos > width() - borderWidth && yPos < borderWidth)//右上角
		//		*result = HTTOPRIGHT;
		//	else if (xPos < borderWidth && yPos > height() - borderWidth)//左下角
		//		*result = HTBOTTOMLEFT;
		//	else if (xPos > width() - borderWidth && yPos > height() - borderWidth)//右下角
		//		*result = HTBOTTOMRIGHT;
		//	else if (xPos < borderWidth)//左边
		//		*result = HTLEFT;
		//	else if (xPos > width() - borderWidth)//右边
		//		*result = HTRIGHT;
		//	else if (yPos < borderWidth)//上边
		//		*result = HTTOP;
		//	else if (yPos > height() - borderWidth)//下边
		//		*result = HTBOTTOM;
		//	else
		//		return false;
		//	return true;
		//}
	}
	break;
	case WM_NCCALCSIZE:
	{
	}
	break;
	case WM_SIZE:   //要让窗体能够随着缩放改变，要响应WM_SIZE消息
	{
		
	}
	break;
	}
	return QWidget::nativeEvent(eventType, message, result);
}


bool MyFileListWidget::eventFilter(QObject* watched, QEvent* event)
{
	// 处理子控件的事件
	if (watched->objectName().contains("MyFileListItem") && watched->parent() == this)
		//要求子控件必须是MyFileListItem
	{
		MyFileListItem* item = dynamic_cast<MyFileListItem*>(watched);
		QMouseEvent* e = dynamic_cast<QMouseEvent*>(event);
		if (item)
		{
			//qDebug() << event->type();
			switch (event->type())
			{
			case QEvent::MouseButtonPress:
			{
				if (e && (e->buttons() & (Qt::LeftButton | Qt::RightButton)))
				{
#ifdef _DEBUG
					std::cout << "MouseButtonPress: MyFileListItem: " << UTF8ToANSI(item->objectName().toStdString()) << std::endl;
#endif // _DEBUG
					keyboardModifiers = QGuiApplication::keyboardModifiers();
					if (keyboardModifiers & Qt::ControlModifier)
					{

					}
					else
					{
						if (!item->isChecked())
						{
							selectionArea->reset();
							emit selectionAreaResized(selectionArea->getGeometry());
							item->setChecked(true);
						}
					}
				}
				break;
			}
			case QEvent::MouseButtonRelease:
			{
#ifdef _DEBUG
				std::cout << "MouseButtonRelease: MyFileListItem: " << UTF8ToANSI(item->objectName().toStdString()) << std::endl;
#endif // _DEBUG
				if (e && (e->button() == Qt::LeftButton))//左键松开
				{
					// Ctrl键按下时，选中或取消选中
					if (keyboardModifiers & Qt::ControlModifier)
					{
						if (item->isChecked())
							item->setChecked(false);
						else
							item->setChecked(true);
					}
					else
					{
						if (item->isChecked())
						{
							selectionArea->reset();
							emit selectionAreaResized(selectionArea->getGeometry());
							item->setChecked(true);
						}
					}
				}
				else if (e && (e->buttons() & Qt::RightButton))//右键松开
				{

				}
				break;
			}
			case QEvent::MouseMove:
			{
				//#ifdef _DEBUG
				//std::cout << "MouseMove: MyFileListItem: " << UTF8ToANSI(item->objectName().toStdString()) << std::endl;
				//#endif // _DEBUG
				if (e && e->buttons() & (Qt::LeftButton | Qt::RightButton))
				{
					QDrag* drag = new QDrag(this);
					std::wstring itemNameWithPath = item->getPath() + item->text().toStdWString();
					const auto& ip = itemsMap[itemNameWithPath];
					const auto& currentItemPosition = ip.position;
					const auto& currentItemIndex = calculateIndex(currentItemPosition);
					// 计算当前item周围一定范围内的item的position
					Index startIndex;
					//if (currentItemIndex.x > 2)
					//	startIndex.x = currentItemIndex.x - 2;
					//else if (currentItemIndex.x == 2)
					//	startIndex.x = currentItemIndex.x - 1;
					if (currentItemIndex.x > 1)
						startIndex.x = currentItemIndex.x - 1;
					else
						startIndex.x = currentItemIndex.x;
					if (currentItemIndex.y > 1)
						startIndex.y = currentItemIndex.y - 1;
					else
						startIndex.y = currentItemIndex.y;
					Index endIndex;
					if (currentItemIndex.y < itemsNumPerColumn)
						endIndex.y = currentItemIndex.y + 1;
					else
						endIndex.y = currentItemIndex.y;
					endIndex.x = currentItemIndex.x + 3;
					//鼠标坐标相对于拖动显示的pixmap左上角的偏移量
					QPoint mousePosOffset = mapFromGlobal(e->globalPosition().toPoint()) - calculatePosFromIndex(startIndex);
					drag->setHotSpot(mousePosOffset);//设置拖动的热点位置
					//拖动时，文件以及自定义数据
					QMimeData* mineData = new QMimeData();
					//选中的items map
					const auto& selectedItemsMap = dragArea->getSelectedItemsMap();
					QList<QUrl> urls;//拖动的文件列表
					QList<DragArea::CustomData> customDatas;//自定义数据，用于移动控件放下时候获取位置
					for (auto it = selectedItemsMap.begin(); it != selectedItemsMap.end(); it++)
					{
						urls.push_back(QUrl::fromLocalFile(QString::fromStdWString(it->second.path + it->second.name)));
						DragArea::CustomData customData;
						std::wstring nwp/*name with path*/ = it->second.path + it->second.name;
						if (itemsMap.count(nwp))
						{
							auto item = static_cast<MyFileListItem*>(itemsMap[nwp].item);
							customData.item = { item->text().toStdWString(), item->getPath(), item->getImage() };
							customData.offset = mapFromGlobal(e->globalPosition().toPoint()) - static_cast<MyFileListItem*>(itemsMap[it->second.path + it->second.name].item)->geometry().center();
							customData.source = this;
							customDatas.push_back(customData);
						}
						else
						{
							//从dragArea的selectedItemsMap中删除该元素
							dragArea->removeItem(it->second.name, it->second.path);
						}
					}

					mineData->setUrls(urls);//设置文件路径列表
					//设置自定义数据
					mineData->setData("application/custom-data", DragArea::serializeCustomDataList(customDatas));
					//设置拖动数据
					drag->setMimeData(mineData);
					//拖动显示的图像
					QPixmap dragPixmap(
						QSize(
							itemSize.width() * 6 + itemSpacing.column * 5,
							itemSize.height() * 3 + itemSpacing.line * 2
						)
					);
					dragPixmap.fill(Qt::transparent);//用透明色填充
					// 检测周围的item有哪些是选中的
					std::vector<Index> itemsInRange = getSelectedItemsInRange(Range{ startIndex, endIndex });
					//从左到右，从上到下排序
					std::sort(itemsInRange.begin(), itemsInRange.end(), [](const Index& a, const Index& b) {
						return a.x < b.x || (a.x == b.x && a.y < b.y);
						});
					//遍历添加item到拖动图像
					for (auto i = itemsInRange.begin(); i != itemsInRange.end(); i++)
					{
						const auto& ip = itemsMap[positionNameWithPathMap[calculatePositionNumberFromIndex(*i)]];
						if (ip.item)
						{
							/*
							QPoint itemPosInPixmap = relativePosTransition(this->geometry().topLeft(), calculatePosFromIndex(*i), calculatePosFromIndex(startIndex));
							static_cast<MyFileListItem*>(ip.item)->render(&dragPixmap, itemPosInPixmap, rect());
							*/
							QPoint itemPosInPixmap = 
								relativePosTransition(this->geometry().topLeft(), calculatePosFromIndex(*i), calculatePosFromIndex(startIndex))
								 - this->geometry().topLeft();
							QPainter painter(&dragPixmap);
							auto shadowItem = static_cast<MyFileListItem*>(ip.item)->getShadowItem();
							QRect imageRect = shadowItem->getImageRect();
							QImage image = shadowItem->getImage();
							painter.drawImage(QRect(itemPosInPixmap + imageRect.topLeft(), imageRect.size()), image);
							painter.setFont(shadowItem->getFont());
							painter.setPen(shadowItem->getPen());
							painter.drawText(QRect(itemPosInPixmap + shadowItem->getTextRect().topLeft(), shadowItem->getTextRect().size()), shadowItem->getText(), shadowItem->getTextOption());
						}
					}
					QPainter painter(&dragPixmap);
					// 拖动时半透明扩散效果
					// 创建临时蒙版图像（透明背景）
					QImage maskImage(dragPixmap.size(), QImage::Format_ARGB32);
					maskImage.fill(Qt::transparent);
					// 定义径向渐变（从中心到边缘）
					QRadialGradient gradient(mousePosOffset.toPointF(), qMin(maskImage.width(), maskImage.height()) / 2);
					gradient.setColorAt(0.0, QColor(0, 0, 0, 255)); // 中心不透明
					gradient.setColorAt(1.0, Qt::transparent); // 边缘透明
					// 在蒙版图像上绘制渐变
					QPainter maskPainter(&maskImage);
					maskPainter.fillRect(maskImage.rect(), gradient);
					maskPainter.end();
					// 将蒙版应用到原图
					painter.setCompositionMode(QPainter::CompositionMode_DestinationIn); // 使用Alpha通道合成
					painter.drawImage(dragPixmap.rect(), maskImage);
					drag->setPixmap(dragPixmap);
					drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction);
					drag->deleteLater();
				}

				break;
			}
			break;
			}
		}
	}
	return QWidget::eventFilter(watched, event);
}

void MyFileListWidget::dragEnterEvent(QDragEnterEvent* e)
{
	if (e->mimeData()->hasFormat("text/uri-list"))
		e->acceptProposedAction();
}

void MyFileListWidget::dragMoveEvent(QDragMoveEvent* e)
{

}

// 当QDrag离开当前窗口
void MyFileListWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
	//if (dragArea)
	//	dragArea->hide();

}

// 接受QDrag的drop时调用
void MyFileListWidget::dropEvent(QDropEvent* e)
{
	//if (pathsList.size())
	//{
	//	std::wstring goalPath;
	//	if (pathsList.size() > 1)
	//	{
	//		QMenu menu(this);
	//		for (std::wstring path : pathsList)
	//		{
	//			QAction* action = new QAction(QString::fromStdWString(path), &menu);
	//			connect(action, &QAction::triggered, this, [&goalPath, action]() {
	//				goalPath = action->text().toStdWString();
	//				});
	//			menu.addAction(action);
	//		}
	//		menu.exec(QCursor::pos());
	//	}
	//	else
	//		goalPath = pathsList[0];
	//	if (!goalPath.empty())
	//	{
	//		PathCompletion(goalPath);
	//		std::wstring localFiles;
	//		for (QUrl url : e->mimeData()->urls())
	//		{
	//			std::wstring localFile = url.toLocalFile().replace("/", "\\").toStdWString();
	//			//std::wstring fileName = url.fileName().toStdWString();
	//			localFiles += localFile + L'\0';
	//		}
	//		const wchar_t* pFrom = localFiles.c_str();
	//		const wchar_t* pTo = (goalPath).data();
	//		SHFILEOPSTRUCT fileOp = { 0 };
	//		//ZeroMemory(&fileOp, sizeof(fileOp));
	//		fileOp.fAnyOperationsAborted = 0;
	//		fileOp.hwnd = (HWND)this->winId();
	//		fileOp.pFrom = pFrom;
	//		fileOp.pTo = pTo;
	//		fileOp.wFunc = FO_COPY;
	//		//fileOp.fFlags
	//		SHFileOperation(&fileOp);
	//	}
	//}
	if (e->source())
	{
		const QMimeData* mimeData = e->mimeData();
		if (mimeData->hasFormat("application/custom-data"))
		{
			QList<DragArea::CustomData> data = DragArea::deserializeCustomDataList(mimeData->data("application/custom-data"));
			MyFileListWidget* source = data.size() ? static_cast<MyFileListWidget*>(data.front().source) : nullptr;
			if (!source)
				return;
			std::lock_guard<std::mutex> lock(mtxItemsMap);//互斥锁
			if (source != this)
				source->mtxItemsMap.lock();//互斥锁
			//从左到右，从上到下排序
			std::sort(data.begin(), data.end(), [=](const DragArea::CustomData& a, const DragArea::CustomData& b) {
				std::wstring nwpa = a.item.path + a.item.name;
				std::wstring nwpb = b.item.path + b.item.name;
				auto posa = static_cast<MyFileListWidget*>(a.source)->getItemsMap()[nwpa].position;
				auto posb = static_cast<MyFileListWidget*>(b.source)->getItemsMap()[nwpb].position;
				auto indexa = calculateIndex(posa);
				auto indexb = calculateIndex(posb);
				return indexa.x < indexb.x || (indexa.x == indexb.x && indexa.y < indexb.y);
				});
			std::cout << "Custom data list: \n";
			std::unordered_map<std::wstring, ItemProp> selectedItemsMap;
			std::unordered_map<long long, std::wstring> selectedPositionNameWithPathMap;
			for (auto i : data)
			{
				source = static_cast<MyFileListWidget*>(i.source);
				std::wstring nwp = i.item.path + i.item.name;
				selectedItemsMap[nwp] = source->getItemsMap()[nwp];
				//static_cast<MyFileListItem*>(selectedItemsMap[nwp].item)->deleteLater();
				//selectedItemsMap[nwp].item = nullptr;
				selectedPositionNameWithPathMap[selectedItemsMap[nwp].position] = nwp;
				
				//这里不能是source->itemsMap，
				// 因为来源处的itemsMap仍需保留这一项，
				// 不然配置文件有问题
				if (itemsMap.count(nwp))
					itemsMap.erase(nwp);
				if (source->positionNameWithPathMap.count(selectedItemsMap[nwp].position))
					source->positionNameWithPathMap.erase(selectedItemsMap[nwp].position);
			}
			for (auto i = data.begin(); i != data.end(); i++)
			{
				std::wstring nwp = i->item.path + i->item.name;
				ItemProp& itemProp = selectedItemsMap[nwp];
				std::cout << "   Position: " << itemProp.position << "\t";
				std::cout << "   Offset: (" << i->offset.x() << "," << i->offset.y() << ")" << std::endl;
				QPoint itemPos = e->position().toPoint() - i->offset;
				const long long& oldPosition = itemProp.position;
				long long newPosition = calculatePositionNumberFromPos(itemPos);
				qDebug() << "   calc item pos: " << itemPos;
				std::cout << "   New position: " << newPosition << std::endl;
				//检测是否有item冲突，如有则消除排斥
				long long tmpPosition = newPosition;
				while (positionNameWithPathMap.count(tmpPosition))
					tmpPosition += 1;
				if (tmpPosition < 0)
					continue;
				for (long long iPosition = tmpPosition - 1; iPosition >= newPosition; iPosition--)
				{
					std::wstring nwp = positionNameWithPathMap[iPosition];
					itemsMap[nwp].position = iPosition + 1;
					positionNameWithPathMap.erase(iPosition);
					positionNameWithPathMap[iPosition + 1] = nwp;
				}
				itemProp.windowId = this->windowId;
				itemProp.position = newPosition;
			}
			//更新itemsMap
			for (auto i = selectedItemsMap.begin(); i != selectedItemsMap.end(); i++)
			{
				itemsMap[i->first] = i->second;
				//把item放置到新的位置
				positionNameWithPathMap[i->second.position] = i->first;
				MyFileListItem* item = static_cast<MyFileListItem*>(i->second.item);
				Index index = calculateIndex(i->second.position);
				QPoint itemPos = calculatePosFromIndex(index);
				if (source != this)
				{
					source->getItemsMap()[i->first].windowId = this->windowId;
					source->changeDragAreaItem(false, item);
					disconnect(source, &MyFileListWidget::selectionAreaResized, item, &MyFileListItem::judgeSelection);
					disconnect(item, &MyFileListItem::checkChange, source, 0);
					item->setParent(this);
					item->move(itemPos);
					connect(this, &MyFileListWidget::selectionAreaResized, item, &MyFileListItem::judgeSelection);
					connect(item, &MyFileListItem::checkChange, this, [=](bool checkState) {
						changeDragAreaItem(checkState, item);
						}/*, Qt::QueuedConnection*/);
					item->removeEventFilter(source);
					item->installEventFilter(this);
					item->show();
					changeDragAreaItem(true, item);
				}
			}
			if (source != this)
			{
				source->mtxItemsMap.unlock();//互斥锁
				source->writeConfig(source->configName);
			}
		}
		// 拖动完成后，写入配置文件
		writeConfig(configName);
	}//拖放item，本软件窗口之间的item拖放，不涉及文件复制和移动
}


std::vector<std::wstring> MyFileListWidget::splitForShellExecuteFromRegedit(std::wstring text, std::wstring delimiter, std::wstring escapeString)
{
	std::vector<std::wstring> result;
	std::vector<std::wstring> res = split(text, delimiter, escapeString);
	if (!res.size())
		return result;
	size_t c = std::count(res[0].begin(), res[0].end(), L'"');
	if (c == 2)
	{
		result.push_back(res[0]);
		std::wstring::size_type st1 = res[0].find(L'"');
		std::wstring::size_type st2 = res[0].find_last_of(L'"');
		size_t endIndex = result.size() - 1;
		if (st2 > st1)
			result[endIndex] = result[endIndex].substr(st1 + 1, st2 - st1 - 1);//除去字符串前面的双引号
		res.erase(res.begin());
		for (auto it = res.begin(); it != res.end(); it++)
			result.push_back(*it);
	}
	else if (c == 1)
	{
		std::wstring::size_type st = res[0].find(L'"');
		res[0] = res[0].substr(st);//不除去字符串前面的双引号
		//res[0] = res[0].substr(st + 1);//除去字符串前面的双引号
		//std::wstring::size_type st = std::wstring::npos;
		result.push_back(res[0]);
		auto iter = res.begin() + 1;
		size_t endIndex = result.size() - 1;
		for (; iter != res.end(); iter++)
		{
			result[endIndex] += delimiter + (*iter);
			std::wstring::size_type st = iter->find(L'"');
			if (st != std::wstring::npos && st == iter->length() - 1)
				break;
		}
		//result[endIndex].pop_back();//除去字符串后面的分隔符delimiter
		//result[endIndex].pop_back();//除去字符串后面的双引号
		if (iter == res.end())
			return result;
		else
		{
			res.erase(res.begin(), iter + 1);
			if (res.size())
			{
				for (auto iter = res.begin(); iter != res.end(); iter++)
					result.push_back(*iter);
			}
		}
	}
	else
	{
		std::wstring::size_type st1 = text.find(L'"');
		std::wstring::size_type st2 = text.find(L'\'');
		if (st1 == std::wstring::npos && st2 == std::wstring::npos)
			result.push_back(join(res, delimiter));
		else
		{
			result = res;
		}
	}
	return result;
}
std::vector<std::wstring> MyFileListWidget::splitForConfig(std::wstring text, std::wstring delimiter/*separator,分隔符*/, std::wstring EscapeString /*char EscapeCharacter*/)
{
	std::vector<std::wstring> result(4);
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
	int p = 2;
	for (auto iter = res.begin(); iter != res.end(); iter++)
		if (isDigits(*iter))
		{
			result[p] = *iter;
			if (p == 2)
				p += 1;
		}
	for (auto iter = result.rbegin(); iter != result.rend(); )
	{
		if (iter->length() > 0)
			break;
		iter = std::reverse_iterator(result.erase(std::next(iter).base()));
	}
	return result;
}

bool MyFileListWidget::readWindowsConfig(std::wstring nameWithPath)
{
	std::lock_guard<std::mutex> lock(mtxWindowsConfigFile);//互斥锁加锁
	using namespace std;
	switch (configMode)
	{
	case MyFileListWidget::File:
	{
		/*
		id: 窗口id
		title: 窗口名字
		cx: x坐标
		cy: y坐标
		width: 窗口宽度
		height: 窗口高度
		*/
		fstream fConfig(nameWithPath, ios::app | ios::out);
		if (!fConfig.is_open())
			return false;//打开失败
		fConfig.close();
		fConfig.open(nameWithPath, ios::in);
		if (fConfig.is_open())
		{
	#ifdef _DEBUG
			cout << UTF8ToANSI("Windows配置文件：") << endl;
	#endif // !_DEBUG
			//判断文件是否为空，成立为空
			if (fConfig.peek() != ifstream::traits_type::eof())
			{
				size_t lineCount = 0;
				wstring wstrLine;
				string strLine;
				while (getline(fConfig, strLine))
				{
					++lineCount;
#ifdef _DEBUG
					cout << strLine << endl;
#endif // !_DEBUG
					std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
					wstrLine = converter.from_bytes(strLine);
					vector<wstring> conf = split(wstrLine);
					try {
						if (conf.size() >= 5)
						{
							wstring cx = *(conf.end() - 4),
								cy = *(conf.end() - 3),
								width = *(conf.end() - 2),
								height = *(conf.end() - 1);
							bool bCx = isDigits(cx);
							bool bCy = isDigits(cy);
							bool bWidth = isDigits(width);
							bool bHeight = isDigits(height);
							string errMsg = "error:";
							if (!bCx)
								errMsg += " cx,";
							if (!bCy)
								errMsg += " cy,";
							if (!bWidth)
								errMsg += " width,";
							if (!bHeight)
								errMsg += " height,";
							if (errMsg.back() == ',')
								errMsg.pop_back();
							if (errMsg != "error:")
							{
								errMsg += " should be number!";
								throw errMsg;
							}

							wstring windowTitle;
							if (conf.size() >= 6)
							{
								for (auto iter = conf.begin() + 2; iter != conf.end() - 4;)
								{
									conf[1] += L" " + (*iter);
									iter = conf.erase(iter);
								}
								windowTitle = conf[1];
							}
							if (!childrenWindowsMap.count(stoll(conf[0])))
								childrenWindowsMap[stoll(conf[0])] = {
									nullptr, windowTitle,
									QRect(stoi(cx), stoi(cy), stoi(width), stoi(height))
								};
							else
							{
								auto& windowProp = childrenWindowsMap[stoll(conf[0])];
								windowProp.title = windowTitle;
								windowProp.rect = QRect(stoi(cx), stoi(cy), stoi(width), stoi(height));
							}
						}
					}
					catch(string errMsg) {
						cout << UTF8ToANSI("配置文件存在错误:\n	line ") << lineCount << ":" << wstr2str_2ANSI(wstrLine) << endl;
						cout << UTF8ToANSI(errMsg) << endl;
						continue;
					}
				}
				//关闭文件
				fConfig.close();
			}
		}
		return true;
	}
	break;
	case MyFileListWidget::Database:
	{
		//TODO: 数据库读取
		// 然后再执行 SELECT 查询
		// 预处理
		vector<DatabaseColumn> tableStruct = {
			{ "id", BIGINT },
			{ "title", TEXT },
			{ "x", INTEGER },
			{ "y", INTEGER },
			{ "width", INTEGER },
			{ "height", INTEGER }
		};
		vector<unsigned long long> primaryKeyIndex(1, 0);//这里分配一个unsigned long long，值为0
		sqlite3* db = nullptr;
		sqlite3_stmt* stmt;
		DatabasePreparation dbp = {
			windowsConfigName,
			tableStruct,
			primaryKeyIndex
		};
		std::tie(db, stmt) = prepareDatabase(dbp, Read);
		//开始遍历查询
		int returnCode = SQLITE_OK;
		while ((returnCode = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			long long id = sqlite3_column_int64(stmt, 0);
			const unsigned char* title = sqlite3_column_text(stmt, 1);
			std::string titleStr = title ? reinterpret_cast<const char*>(title) : "";
			std::wstring titleWStr = str2wstr_2UTF8(titleStr);
			int x = sqlite3_column_int(stmt, 2);
			int y = sqlite3_column_int(stmt, 3);
			int width = sqlite3_column_int(stmt, 4);
			int height = sqlite3_column_int(stmt, 5);
			std::cout << id << " " << title << " " << x << " " << y
				<< " " << width << " " << height << std::endl;
			if (!childrenWindowsMap.count(id))
			{
				childrenWindowsMap[id] = {
					nullptr, titleWStr,
					QRect(x, y, width, height)
				};
			}
			else
			{
				auto& windowProp = childrenWindowsMap[id];
				windowProp.title = titleWStr;
				windowProp.rect = QRect(x, y, width, height);
			}
		}
		sqlite3_finalize(stmt);
		sqlite3_close(db);
	}
	break;
	default:
		break;
	}
	return true;
}

bool MyFileListWidget::writeWindowsConfig(std::wstring nameWithPath)
{
	std::lock_guard<std::mutex> lock(mtxWindowsConfigFile);//互斥锁加锁
	using namespace std;
	switch (configMode)
	{
	case MyFileListWidget::File:
	{

		string delimiter = " ";
		ofstream outConfig;
		outConfig.open(nameWithPath, ios::out);
		if (!outConfig.is_open())
			return false;
		else
		{
			string(*encodeType)(wstring) = wstr2str_2UTF8;
			for (auto i = childrenWindowsMap.begin(); i != childrenWindowsMap.end(); i++)
			{
				outConfig << i->first;
				outConfig << delimiter;
				outConfig << encodeType(i->second.title) << delimiter
					<< to_string(i->second.rect.x()) << delimiter
					<< to_string(i->second.rect.y()) << delimiter
					<< to_string(i->second.rect.width()) << delimiter
					<< to_string(i->second.rect.height())
					<< endl;
				outConfig.flush();
			}
			outConfig.close();
		}
		return true;
	}
	break;
	case MyFileListWidget::Database:
	{
		//TODO: 数据库写入
		vector<DatabaseColumn> tableStruct = {
			{ "id", BIGINT },
			{ "title", TEXT },
			{ "x", INTEGER },
			{ "y", INTEGER },
			{ "width", INTEGER },
			{ "height", INTEGER }
		};
		vector<unsigned long long> primaryKeyIndex(1, 0);//这里分配一个unsigned long long，值为0
		DatabasePreparation dbp = {
			windowsConfigName,
			tableStruct,
			primaryKeyIndex
		};
		sqlite3* db = nullptr;
		sqlite3_stmt* stmt;
		std::tie(db, stmt) = prepareDatabase(dbp, Write);
		//保存数据
		string(*encodeType)(wstring) = wstr2str_2UTF8;
		for (auto i = childrenWindowsMap.begin(); i != childrenWindowsMap.end(); i++)
		{
			long long id = i->first;
			wstring titleWStr = i->second.title;
			string title = encodeType(titleWStr);
			QRect rect = i->second.rect;
			sqlite3_bind_int64(stmt, 1, id);
			sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_int(stmt, 3, rect.x());
			sqlite3_bind_int(stmt, 4, rect.y());
			sqlite3_bind_int(stmt, 5, rect.width());
			sqlite3_bind_int(stmt, 6, rect.height());
			int returnCode = sqlite3_step(stmt);
			sqlite3_reset(stmt);
			if (returnCode != SQLITE_DONE) {
				std::cerr << UTF8ToANSI("执行插入失败: ") << sqlite3_errmsg(db) << std::endl;
				MessageBox(0, L"Database save error.\n保存数据库失败，软件运行可能不正常", L"error", MB_ICONERROR);
			}
		}
		sqlite3_finalize(stmt);
		sqlite3_close(db);
	}
	break;
	default:
		break;
	}
	return true;
}

bool MyFileListWidget::readConfig(std::wstring nameWithPath, bool whetherToCreateItem)
{
	switch (configMode)
	{
	case MyFileListWidget::File:
	{
		using namespace std;
		std::lock_guard<std::mutex> lock(mtxConfigFile);//互斥锁加锁

		fstream fConfig(nameWithPath, ios::app | ios::out);
		if (!fConfig.is_open())
			return false;
		else
		{
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
						++lineCount;
#ifdef _DEBUG
						if (conf.size() < 4)
						{
							cout << UTF8ToANSI("配置文件存在错误:\n	line ") << lineCount << ":" << wstr2str_2ANSI(wstrLine) << endl;
							continue;
						}
						cout << wstr2str_2ANSI(wstrLine) << endl;
#else
						if (conf.size() < 4)
							continue;
#endif // _DEBUG
						mtxItemsMap.lock();
						itemsMap[conf[1] + conf[0]] = {
							stoll(conf[3]), 0, conf[0], conf[1], stoll(conf[2])
						};
						if (stoll(conf[3]) == this->windowId)
							positionNameWithPathMap[stoll(conf[2])] = conf[1] + conf[0];
						mtxItemsMap.unlock();
						if (whetherToCreateItem)
						{
							WindowInfo thisWindowInfo{ this, windowTitle().toStdWString(), geometry() };
							auto& windowProp = (stoll(conf[3]) == windowId) ? thisWindowInfo : childrenWindowsMap[stoll(conf[3])];
							if (!windowProp.window)
							{
								QRect newWindowRect(
									QPoint((width() - width() / 3) / 2,
										(height() - height() / 3) / 2),
									size() / 3
								);
								//TODO: 子窗口创建
								windowProp.window = 0;
							}
							if (windowProp.window)
								windowProp.window->createItem(conf[0], conf[1]);
						}
					}
				}
				//关闭文件
				fConfig.close();
			}
		}
		return true;
	}
		break;
	case MyFileListWidget::Database:
	{
		std::lock_guard<std::mutex> lock(mtxConfigFile);//互斥锁加锁

		using namespace std;
		//TODO: 数据库读取
		vector<DatabaseColumn> tableStruct = {
			{ "windowId", BIGINT },
			{ "name", TEXT },
			{ "path", TEXT },
			{ "position", BIGINT }
		};
		vector<unsigned long long> primaryKeyIndex{ 1,2 };//这里表示name和path的复合主键
		sqlite3* db = nullptr;
		sqlite3_stmt* stmt;
		DatabasePreparation dbp = {
			configName,
			tableStruct,
			primaryKeyIndex
		};
		std::tie(db, stmt) = prepareDatabase(dbp, Read);
		//开始遍历查询
		int returnCode = SQLITE_OK;
		wcout << str2wstr_2UTF8("配置文件：") << endl;
		while ((returnCode = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			long long id = sqlite3_column_int64(stmt, 0);
			const unsigned char* name = sqlite3_column_text(stmt, 1);
			const unsigned char* path = sqlite3_column_text(stmt, 2);
			long long position = sqlite3_column_int64(stmt, 3);
			std::string nameStr = name ? reinterpret_cast<const char*>(name) : "";
			std::string pathStr = path ? reinterpret_cast<const char*>(path) : "";
			std::wstring nameWStr = str2wstr_2UTF8(nameStr);
			std::wstring pathWStr = str2wstr_2UTF8(pathStr);
			std::cout << id << " " << nameStr << " " << pathStr << " " << position << std::endl;

			mtxItemsMap.lock();
			itemsMap[pathWStr + nameWStr] = {
				id, 0, nameWStr, pathWStr, position
			};
			if (id == this->windowId)
				positionNameWithPathMap[position] = pathWStr + nameWStr;
			mtxItemsMap.unlock();
			if (whetherToCreateItem)
			{
				WindowInfo thisWindowInfo{ this, windowTitle().toStdWString(), geometry() };
				auto& windowProp = (id == windowId) ? thisWindowInfo : childrenWindowsMap[id];
				if (!windowProp.window)
				{
					QRect newWindowRect(
						QPoint((width() - width() / 3) / 2,
							(height() - height() / 3) / 2),
						size() / 3
					);
					//TODO: 子窗口创建
					windowProp.window = 0;
				}
				if (windowProp.window)
					windowProp.window->createItem(nameWStr, pathWStr);
			}
		}

		sqlite3_finalize(stmt);
		sqlite3_close(db);
	}
		break;
	default:
		break;
	}
	return true;
}

bool MyFileListWidget::writeConfig(std::wstring nameWithPath)
{
	std::lock_guard<std::mutex> lock(mtxConfigFile);//互斥锁加锁
	using namespace std;
	switch (configMode)
	{
	case MyFileListWidget::File:
	{
		string delimiter = " ";
		ofstream outConfig;
		outConfig.open(nameWithPath);
		if (!outConfig.is_open())
			return false;
		else
		{
			string(*encodeType)(wstring) = wstr2str_2UTF8;
			std::lock_guard<std::mutex> lock(mtxItemsMap);
			for (auto i = itemsMap.begin(); i != itemsMap.end(); i++)
			{
				outConfig << "\"" << encodeType(i->second.name);
				outConfig << "\"" << delimiter;
				outConfig << "\"" << encodeType(i->second.path);
				outConfig << "\"" << delimiter
					<< i->second.position << delimiter
					<< i->second.windowId << endl;
				outConfig.flush();
			}
			outConfig.close();
		}
		return true;
	}
		break;
	case MyFileListWidget::Database:
	{
		//TODO: 数据库写入
		vector<DatabaseColumn> tableStruct = {
			{ "windowId", BIGINT },
			{ "name", TEXT },
			{ "path", TEXT },
			{ "position", BIGINT }
		};
		vector<unsigned long long> primaryKeyIndex{ 1,2 };//这里表示name和path的复合主键
		sqlite3* db = nullptr;
		sqlite3_stmt* stmt;
		DatabasePreparation dbp = {
			configName,
			tableStruct,
			primaryKeyIndex
		};
		std::tie(db, stmt) = prepareDatabase(dbp, Write);
		//保存数据
		string(*encodeType)(wstring) = wstr2str_2UTF8;
		for (auto i = itemsMap.begin(); i != itemsMap.end(); i++)
		{
			long long wid = i->second.windowId;
			wstring nameWStr = i->second.name;
			wstring pathWStr = i->second.path;
			string nameStr = encodeType(nameWStr);
			string pathStr = encodeType(pathWStr);
			long long position = i->second.position;
			sqlite3_bind_int64(stmt, 1, wid);
			sqlite3_bind_text(stmt, 2, nameStr.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(stmt, 3, pathStr.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_int64(stmt, 4, position);
			int returnCode = sqlite3_step(stmt);
			sqlite3_reset(stmt);
			if (returnCode != SQLITE_DONE) {
				std::cerr << UTF8ToANSI("执行插入失败: ") << sqlite3_errmsg(db) << std::endl;
				MessageBox(0, L"Database save error.\n保存数据库失败，软件运行可能不正常", L"error", MB_ICONERROR);
			}
		}
		sqlite3_finalize(stmt);
		sqlite3_close(db);
	}
		break;
	default:
		break;
	}
	return true;
}

std::tuple<sqlite3*, sqlite3_stmt*> MyFileListWidget::prepareDatabase(DatabasePreparation dbp, DatabaseOperation operation)
{
	sqlite3* db = nullptr;
	//打开数据库文件，并连接到数据库
	int code = sqlite3_open(databaseFileName.c_str(), &db);
	if (code != SQLITE_OK)
	{//打开失败
		std::cerr << UTF8ToANSI("无法打开数据库: ") << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		MessageBox(0, L"Database open error.\nClick to exit.\n打开数据库失败，点击确定退出。", L"error", MB_ICONERROR);
		this->deleteLater();
	}
	//打开成功
	//没有table就创建新表
	std::string tableStruct;//新表结构
	for (auto i = dbp.columns.begin(); i != dbp.columns.end(); i++)
		tableStruct += i->name + ' ' + SQLite3DataTypeToString(i->type) + ',';
	if (tableStruct.size())
	{
		if (dbp.primaryKeyIndex.size())
		{
			tableStruct += "PRIMARY KEY (";
			for (auto i : dbp.primaryKeyIndex)
				tableStruct += dbp.columns[i].name + ',';
			if (dbp.primaryKeyIndex.size())
				tableStruct.pop_back();
			tableStruct += ")";
		}
		else
			tableStruct.pop_back();
	}
	std::string createTableSQL =
		std::string("CREATE TABLE IF NOT EXISTS ")
		+ wstr2str_2UTF8(dbp.tableName)
		+ " ("
		+ tableStruct
		+ ") WITHOUT ROWID";
	char* errMsg = nullptr;
	int rc = sqlite3_exec(db, createTableSQL.c_str(), nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK)
	{
		std::cerr << UTF8ToANSI("创建表失败: ") << errMsg << std::endl;
		sqlite3_free(errMsg);
		MessageBox(0, L"Database table create error.\nClick to exit.\n创建数据库table失败，点击确定退出。", L"error", MB_ICONERROR);
		this->deleteLater();
	}
	

	std::string prepareText = "";
	switch (operation)
	{
	default:
	case Read:
	{
		//std::string sql = "SELECT id, title, x, y, width, height FROM " + wstr2str_2UTF8(windowsConfigName);
		prepareText = "SELECT ";
		for (auto i = dbp.columns.begin(); i != dbp.columns.end(); i++)
			prepareText += i->name + ',';
		if (dbp.columns.size())
			prepareText.pop_back();
		prepareText += " FROM " + wstr2str_2UTF8(dbp.tableName);
		break;
	}
	case Write:
	{
		//std::string sql = "INSERT INTO " + wstr2str_2UTF8(windowsConfigName) + " (id, title, x, y, width, height) VALUES (?, ?, ?, ?, ?, ?)";
		std::string askSignText;
		prepareText = "INSERT OR REPLACE INTO " + wstr2str_2UTF8(dbp.tableName) + " (";
		for (auto i = dbp.columns.begin(); i != dbp.columns.end(); i++)
		{
			prepareText += i->name + ',';
			askSignText += "?,";
		}
		if (dbp.columns.size())
		{
			prepareText.pop_back();
			askSignText.pop_back();
		}
		prepareText += ") VALUES (";
		prepareText += askSignText;
		prepareText += ")";
		break;
	}
	}
	sqlite3_stmt* stmt;
	code = sqlite3_prepare_v2(db, prepareText.c_str(), -1, &stmt, nullptr);//编译SQL语句为字节码
	if (rc != SQLITE_OK)
	{
		std::cerr << UTF8ToANSI("准备语句失败: ") << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		MessageBox(0, L"Database prepare error.\nClick to exit.\n数据库准备语句失败，点击确定退出。", L"error", MB_ICONERROR);
		this->deleteLater();
	}
	return std::tuple<sqlite3*, sqlite3_stmt*>(db, stmt);
}

void MyFileListWidget::checkFilesChange(std::wstring path, bool isSingleShot)
{
	using namespace std;
	WCHAR* cFilePath;
	cFilePath = (WCHAR*)path.c_str();
	while (!checkFilesChangeThreadExit)
	{
		Sleep(10);

		std::lock_guard<std::mutex> lock1(mtxItemTaskQueue);//互斥锁加锁
		std::lock_guard<std::mutex> lock2(mtxItemsMap);//互斥锁加锁
		intptr_t FileIndex = 0;
		WIN32_FIND_DATA ffd;
		LARGE_INTEGER filesize;
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
					if (!itemsMap[nameWithPath].item && itemsMap[nameWithPath].windowId == this->windowId)
					{
						//ItemTask task = { &MyFileListWidget::sendCreateItemSignalAndWriteConfig, tmpwstringarray[0], path };
						ItemTask task = { ItemTask::Create, {tmpwstringarray[0], path} };
						if (!isItemTaskInQueue(task))
							addItemTaskIfNotInQueue(task);
					}
				}
				else
				{
					// Can't find the file in itemsMap
					// Send CreateItem Signal
					ItemTask task = { ItemTask::Create, {tmpwstringarray[0], path} };
					if (!isItemTaskInQueue(task))
						addItemTaskIfNotInQueue(task);
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
			if ((i->second.path == path) || (std::find(pathsList.begin(), pathsList.end(), i->second.path) == pathsList.end()))
			{
				if (itemsMap.count(i->first))
				{
					ItemTask task = { ItemTask::Remove, {i->second.name, i->second.path} };
					/*MessageBox((HWND)winId(), path.c_str(), i->second.name.c_str(), 0);*/
					if (!isItemTaskInQueue(task))
						addItemTaskIfNotInQueue(task);
				}
			}
			tmpItemsMap.erase(i++);
		}
		//long long npcBack = itemsNumPerColumn;
		/*重新计算item大小和每列item的个数*/
		changeItemSizeAndNumbersPerColumn();
		// 变小
		for (auto i = itemsMap.begin(); i != itemsMap.end(); i++)
		{
			Index index = calculateIndex(i->second.position);
			QPoint itemPos = calculatePosFromIndex(index);
			if (i->second.item && i->second.windowId == this->windowId)
			{
				if (static_cast<MyFileListItem*>(i->second.item)->size() != itemSize)
					emit static_cast<MyFileListItem*>(i->second.item)->adjustSizeSignal();
				if (itemPos != ((MyFileListItem*)i->second.item)->pos())
					emit static_cast<MyFileListItem*>(i->second.item)->moveSignal(itemPos);
			}
		}
		if (isSingleShot)
			break;
	}
}

//请在调用的函数前添加互斥锁！
bool MyFileListWidget::isItemTaskInQueue(ItemTask task)
{
	//请在调用的函数前添加互斥锁！
	//std::lock_guard<std::mutex> lock(mtxItemTaskQueue);
	const std::queue<ItemTask>& queue = itemTaskQueue;
	const std::deque<ItemTask>& deque = queue._Get_container();
	if ((!deque.empty()) && std::find(deque.begin(), deque.end(), task) != deque.end())
	{
		return true;
	}
	return false;
}

void MyFileListWidget::addItemTask(ItemTask task)
{
	std::lock_guard<std::mutex> lock(mtxItemTaskQueue);
	itemTaskQueue.push(task);
}

//请在调用的函数前添加互斥锁！
void MyFileListWidget::addItemTaskIfNotInQueue(ItemTask task)
{
	//请在调用的函数前添加互斥锁！
	//std::lock_guard<std::mutex> lock(mtxItemTaskQueue);
	const std::queue<ItemTask>& queue = itemTaskQueue;
	const std::deque<ItemTask>& deque = queue._Get_container();
	if (deque.empty() || std::find(deque.begin(), deque.end(), task) == deque.end())
	{
		itemTaskQueue.push(task);
#ifdef _DEBUG
		std::cout << "Add item task:" << wstr2str_2ANSI(std::any_cast<std::wstring>(task.args[0])) << std::endl;
#endif
	}
}
void MyFileListWidget::itemTaskExecuteProc()
{
	while (!checkFilesChangeThreadExit)
	{
		mtxItemTaskQueue.lock();
		if (!itemTaskQueue.empty())
		{
			ItemTask it = itemTaskQueue.front();
#ifdef _DEBUG
			std::cout << "Execute item task:" << wstr2str_2ANSI(std::any_cast<std::wstring>(it.args[0])) << std::endl;
#endif
			std::wstring name = std::any_cast<std::wstring>(it.args[0]);
			std::wstring path = std::any_cast<std::wstring>(it.args[1]);
			switch (it.task)
			{
			case ItemTask::Create:
			{
				sendCreateItemSignalAndWriteConfig(name, path);
			}
			break;
			case ItemTask::Remove:
			{
				sendRemoveItemSignalAndWriteConfig(name, path);
			}
			break;
			case ItemTask::Rename:
			{
				std::wstring newName = std::any_cast<std::wstring>(it.args[2]);
				sendRenameItemSignalAndWriteConfig(name, path, newName);
			}
			break;
			default:
				break;
			}
#ifdef _DEBUG
			std::cout << "Finish executing item task:" << wstr2str_2ANSI(std::any_cast<std::wstring>(it.args[0])) << std::endl;
#endif
			itemTaskQueue.pop();
			mtxItemTaskQueue.unlock();
			continue;
		}
		mtxItemTaskQueue.unlock();
		Sleep(10);
	}
}

//unused-已停用
//from csdn
[[deprecated]]HBITMAP GetThumbnail(const std::wstring& filePath)
{
	std::wstring strDir;
	std::wstring strFileName;
	int nPos = filePath.find_last_of(L"\\");
	strDir = filePath.substr(0, nPos);
	strFileName = filePath.substr(nPos + 1);
	CComPtr<IShellFolder> pDesktop = NULL;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);
	if (FAILED(hr))
		return NULL;
	LPITEMIDLIST pidl = NULL;
	hr = pDesktop->ParseDisplayName(NULL, NULL, (LPWSTR)strDir.c_str(), NULL, &pidl, NULL);
	if (FAILED(hr))
		return NULL;
	CComPtr<IShellFolder> pSub = NULL;
	hr = pDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (void**)&pSub);
	if (FAILED(hr))
		return NULL;
	hr = pSub->ParseDisplayName(NULL, NULL, (LPWSTR)strFileName.c_str(), NULL, &pidl, NULL);
	if (FAILED(hr))
		return NULL;
	CComPtr<IExtractImage> pIExtract = NULL;
	hr = pSub->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidl, IID_IExtractImage, NULL, (void**)&pIExtract);
	if (FAILED(hr))
		return NULL;
	SIZE size = { 64, 128 }; // 请求获取的图片大小
	DWORD dwFlags = IEIFLAG_ORIGSIZE | IEIFLAG_QUALITY;
	OLECHAR pathBuffer[MAX_PATH] = { 0 };
	hr = pIExtract->GetLocation(pathBuffer, MAX_PATH, NULL, &size, 24, &dwFlags);
	if (FAILED(hr))
		return NULL;
	HBITMAP hThumbnail = NULL;
	hr = pIExtract->Extract(&hThumbnail);
	if (FAILED(hr))
		return NULL;
	return hThumbnail;
}

// By AI(Kimi)
QImage HBITMAPToQImage(HBITMAP hBitmap, bool& flipped)
{
	// 获取位图信息
	BITMAP bitmapInfo;
	GetObject(hBitmap, sizeof(BITMAP), &bitmapInfo);

	// 创建一个 QImage
	QImage image(bitmapInfo.bmWidth, bitmapInfo.bmHeight, QImage::Format_ARGB32);
	image.fill(Qt::transparent); // 填充透明背景

	// 创建一个内存设备上下文
	HDC hdc = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

	// 将位图数据复制到 QImage
	GetBitmapBits(hBitmap, bitmapInfo.bmWidthBytes * bitmapInfo.bmHeight, image.bits());

	// 恢复旧的位图
	SelectObject(hdcMem, hOldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdc);

	// 检查是否需要翻转
	flipped = (bitmapInfo.bmHeight < 0);

	// 如果高度为负值，表示位图是从底部开始存储的，需要翻转
	if (bitmapInfo.bmHeight < 0)
	{
		image = image.mirrored(false, true);
	}

	return image;
}
void MyFileListWidget::changeDragAreaItem(bool checkState, MyFileListItem* item)
{
	std::wstring nwp = item->getPath() + item->text().toStdWString();
	ItemProp& ip = itemsMap[nwp];
	if (checkState)
	{
		DragArea::ShadowItemInformation sii = { ip.name, ip.path, item->getImage() };
		dragArea->addItem(sii);
	}
	else
	{
		dragArea->removeItem(ip.name, ip.path);
	}
}

void MyFileListWidget::createItem(std::wstring name, std::wstring path)
{
	std::lock_guard<std::mutex> lock(mtxItemsMap);//互斥锁加锁
	if (!PathCompletion(path))
	{
		cvItemTaskFinished.notify_one();
		return;
	}
	std::wstring nameWithPath = path;
	nameWithPath += name;
	if (!itemsMap.count(nameWithPath))
		itemsMap[nameWithPath] = { windowId, nullptr, name, path, -1 };
	if (!itemsMap[nameWithPath].item)
	{
		/*创建新item*/
		MyFileListItem* pLWItem = new MyFileListItem(this, itemSize);
		// std::wstring::size_type st = 0; ///TOREMOVE
		long long newPosition = 0;
		if (itemsMap[nameWithPath].position == -1)
		{
			/*判断是否已经放置图标*/
			/*
			st = indexesState.find(L'0');
			if (st == std::wstring::npos)
			{
				st = indexesState.length();
				indexesState += L'0';
			}
			indexesState.replace(st, 1, L"1");
			*/ ///TOREMOVE
			while (newPosition >= 0)
			{
				if (!positionNameWithPathMap.count(newPosition))
					break;
				newPosition++;
			}
		}
		else
		{
			/*
			st = itemsMap[nameWithPath].position;
			for (size_t i = indexesState.length(); i < st + 2; i++)
				indexesState += L"0";
			indexesState[st] = '1';
			*/ ///TOREMOVE
		}
		if (itemsNumPerColumn == 0)
			itemsNumPerColumn = 1;
		/*计算新添加的item的合适位置的index*/
		//Index index = calculateIndex(st); ///TOREMOVE
		Index index = calculateIndex(newPosition);
		
		/*将新添加的item加入map*/
		auto& itemProp = itemsMap[nameWithPath];
		auto position = itemProp.position;
		auto windowId = itemProp.windowId;
		std::cout << "[New Item]\n";
		std::cout << "New item name: " << wstr2str_2ANSI(name) << "\n";
		std::cout << "New item path: " << wstr2str_2ANSI(path) << "\n";
		//std::cout << "New item position: " << (position == -1 ? (long long)st : position) << "\n"; ///TOREMOVE
		//auto& ip = itemsMap[nameWithPath] = { windowId, pLWItem, name, path, (position == -1 ? (long long)st : position)}; ///TOREMOVE
		std::cout << "New item position: " << (position == -1 ? (long long)newPosition : position) << "\n";
		auto& ip = itemsMap[nameWithPath] = { windowId, pLWItem, name, path, (position == -1 ? (long long)newPosition : position)};
		positionNameWithPathMap[ip.position] = nameWithPath;

		/*新添加的item的图标*/
		QImage pLWItemImage;
		IShellItemImageFactory* ISIIFactory = nullptr;
		HRESULT hr = SHCreateItemFromParsingName(nameWithPath.c_str(), nullptr, __uuidof(IShellItemImageFactory), (void**)&ISIIFactory);
		//HRESULT hr = SHCreateItemFromParsingName(L"C:\\Users\\lyxyz5223\\Desktop\\QQ插件", nullptr, __uuidof(IShellItemImageFactory), (void**)&ISIIFactory);
		if (SUCCEEDED(hr))
		{
			SIZE imageSize = { 256,256 };
			HBITMAP hBitmap;
			hr = ISIIFactory->GetImage(imageSize, SIIGBF_THUMBNAILONLY | SIIGBF_BIGGERSIZEOK, &hBitmap);
			if (SUCCEEDED(hr))
			{
				//pLWItemImage = QImage::fromHBITMAP(hBitmap);
				bool flipped = false;
				pLWItemImage = HBITMAPToQImage(hBitmap, flipped);
				int a = pLWItemImage.height();
				int b = pLWItemImage.heightMM();
				std::cout << "height: " << a << std::endl;
				std::cout << "heightMM: " << b << std::endl;
				BITMAP bmp;
				GetObject(hBitmap, sizeof(BITMAP), &bmp);
				std::cout << "HBITMAP hBitmap->BITMAP bmp->bmp.bmHeight: " << bmp.bmHeight << "\n";
				DeleteObject(&bmp);
			}
			else
			{
				std::cout << "No thumb image" << std::endl;
			}
			DeleteObject(hBitmap);
			ISIIFactory->Release();
		}
		
		// 法二：
		//IShellItem* item = nullptr;
		//HRESULT hr = SHCreateItemFromParsingName(nameWithPath.c_str(), nullptr, IID_PPV_ARGS(&item));
		//IThumbnailCache* cache = nullptr;
		//hr = CoCreateInstance(
		//	CLSID_LocalThumbnailCache,
		//	nullptr,
		//	CLSCTX_INPROC,
		//	IID_PPV_ARGS(&cache));
		//ISharedBitmap* sharedBitmap;
		//hr = cache->GetThumbnail(
		//	item,
		//	0,
		//	WTS_EXTRACT,
		//	&sharedBitmap,
		//	nullptr,
		//	nullptr);
		//HBITMAP hBitmap = NULL;
		//if (sharedBitmap)
		//{
		//	hr = sharedBitmap->GetSharedBitmap(&hBitmap);
		//	if (SUCCEEDED(hr))
		//	{
		//		pLWItemImage = QImage::fromHBITMAP(hBitmap);
		//		DeleteObject(hBitmap);
		//	}
		//	sharedBitmap->Release();
		//}
		//cache->Release();
		//item->Release();

		if (pLWItemImage.isNull())
		{
			SHFILEINFO sfi;
			DWORD_PTR dw_ptr = SHGetFileInfo(nameWithPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
			if (dw_ptr)
			{
				//pLWItemImage = QImage::fromHICON(sfi.hIcon);

				// 获取大号图像列表
				IImageList* piml; 
				HRESULT hr = SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&piml));
				if (SUCCEEDED(hr))
				{
					HICON hico;
					piml->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hico);
					pLWItemImage = QImage::fromHICON(hico);
					piml->Release();
				}
			}
		}
		pLWItem->setText(wstr2str_2UTF8(name).c_str());
		pLWItem->setViewMode(viewMode);
		pLWItem->setPath(path);
		pLWItem->setImage(pLWItemImage);
		pLWItem->setObjectName(QString("MyFileListItem_") + QString::number(position));
		pLWItem->installEventFilter(this);// 安装事件过滤器
		//pLWItem->adjustSize();
		QPoint itemPos = calculatePosFromIndex(index);
		pLWItem->move(itemPos);
		//pLWItem->setImage(QImage::fromHICON(sfi.hIcon));
		//connect(pLWItem, &MyFileListItem::removeSelfSignal, this, [=]() {
		//	sendRemoveItemSignalAndWriteConfig(pLWItem->text().toStdWString(), pLWItem->getPath());
		//	});
		connect(pLWItem, &MyFileListItem::checkChange, this, [=](bool checkState) {
			changeDragAreaItem(checkState, pLWItem);
		}/*, Qt::QueuedConnection*/);
		connect(this, &MyFileListWidget::selectionAreaResized, pLWItem, &MyFileListItem::judgeSelection);
		pLWItem->show();
	}
	cvItemTaskFinished.notify_one();//唤醒一个等待中的线程
}

void MyFileListWidget::removeItem(std::wstring name, std::wstring path)
{
	std::lock_guard<std::mutex> lock(mtxItemsMap);//互斥锁加锁
	std::wstring nameWithPath = path + name;
	if (itemsMap.count(nameWithPath))
	{
		ItemProp& ip = itemsMap[nameWithPath];
		void* item = ip.item;
		//if (ip.position < indexesState.size()) ///TOREMOVE
		//	indexesState[ip.position] = L'0'; ///TOREMOVE
		if (positionNameWithPathMap.count(ip.position))
			positionNameWithPathMap.erase(ip.position);
		itemsMap.erase(nameWithPath);
		if (item)
		{
			static_cast<MyFileListItem*>(item)->deleteLater();
			item = nullptr;
		}
	}
	cvItemTaskFinished.notify_one();//唤醒一个等待中的线程
}

void MyFileListWidget::renameItem(std::wstring oldName, std::wstring path, std::wstring newName)
{
	std::lock_guard<std::mutex> lock(mtxItemsMap);//互斥锁加锁
	std::wstring oldNameWithPath = path + oldName;
	std::wstring newNameWithPath = path + newName;
	if (itemsMap.count(oldNameWithPath))
	{
		ItemProp ip = itemsMap[oldNameWithPath];
		ip.name = newName;
		void* item = ip.item;
		itemsMap[newNameWithPath] = ip;
		itemsMap.erase(oldNameWithPath);
		static_cast<MyFileListItem*>(item)->setText(QString::fromStdWString(newName));
		positionNameWithPathMap[ip.position] = newNameWithPath;
	}
	cvItemTaskFinished.notify_one();//唤醒一个等待中的线程
}

//获取文件改动，同步
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

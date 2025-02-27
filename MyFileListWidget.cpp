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
#include "stringProcess.h"
#include <cstdlib>
#include <thread>
#include <codecvt>
#include <locale>
#include <string>

// DataBase
#define OTL_ODBC
#include "./lib/otlv4.h"

//My Lib
#include "fileProc.h"
//声明&定义
//注册表
#define HKEY_CLASSES_ROOT_STR "HKEY_CLASSES_ROOT"
#define HKEY_CURRENT_USER_ShellNew_STR "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Discardable\\PostSetup\\ShellNew"
#define HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_STR "HKEY_CLASSES_ROOT\\DesktopBackground"
#define HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_SHELL_STR "HKEY_CLASSES_ROOT\\DesktopBackground\\Shell"
#define HKEY_CLASSES_ROOT_DESKTOPBACKGROUND_SHELLEX_STR "HKEY_CLASSES_ROOT\\DesktopBackground\\shellex"
#define HKEY_CLASSES_ROOT_DIRECTORY_STR "HKEY_CLASSES_ROOT\\Directory"
#define HKEY_CLASSES_ROOT_DIRECTORY_SHELL_STR "HKEY_CLASSES_ROOT\\Directory\\shell"
#define HKEY_CLASSES_ROOT_DIRECTORY_SHELLEX_STR "HKEY_CLASSES_ROOT\\Directory\\shellex"
#define HKEY_CLASSES_ROOT_DIRECTORY_BACKGROUND_STR "HKEY_CLASSES_ROOT\\Directory\\Background"
#define HKEY_CLASSES_ROOT_DIRECTORY_BACKGROUND_SHELL_STR "HKEY_CLASSES_ROOT\\Directory\\Background\\shell"
#define HKEY_CLASSES_ROOT_DIRECTORY_BACKGROUND_SHELLEX_STR "HKEY_CLASSES_ROOT\\Directory\\Background\\shellex"



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
void MyFileListWidget::initialize(QWidget* parent, std::vector<std::wstring> pathsList, std::wstring config)
{
	for (auto iter = pathsList.begin(); iter != pathsList.end(); iter++)
	{
		PathCompletion(*iter);
		isCreatingItem[*iter] = (isRemovingItem[*iter] = false);
	}
	this->pathsList = pathsList;
		
	configFileNameWithPath = config;

	selectionArea = new SelectionArea(this);
	dragArea = new DragArea(this, itemsNumPerColumn, itemSize, itemSpacing);
	dragArea->hide();
	// 先计算，因为读取配置文件的时候要按照大小创建桌面图标Item
	changeItemSizeAndNumbersPerColumn();
	if (!readConfigFile(config, false))
		QMessageBox::critical(this, "错误", "配置文件读取失败！程序即将退出。");
	/*计算item大小和每列item的个数*/


	for (auto i = pathsList.begin(); i != pathsList.end(); i++)
		checkFilesChangeThreads.push_back(std::thread(&MyFileListWidget::checkFilesChangeProc, this, *i));
}

MyFileListWidget::MyFileListWidget(QWidget* parent, std::vector<std::wstring> pathsList, std::wstring config)// : QWidget(parent)
{
	//setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint);
	this->parent = parent;
	SetParent((HWND)winId(), (HWND)parent->winId());
	//setParent(parent);
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
	disconnect(this, &MyFileListWidget::createItemSignal, this, &MyFileListWidget::createItem);
	disconnect(this, &MyFileListWidget::removeItemSignal, this, &MyFileListWidget::removeItem);
	if (connect(this, &MyFileListWidget::createItemSignal, this, &MyFileListWidget::createItem))
	{
#ifdef _DEBUG
		std::cout << "connect succeeded: connect(this, &MyFileListWidget::createItemSignal, this, &MyFileListWidget::createItem)\n";
#endif // _DEBUG
	}
	if (connect(this, SIGNAL(removeItemSignal(std::wstring, std::wstring)), this, SLOT(removeItem(std::wstring, std::wstring))))
	{
#ifdef _DEBUG
		std::cout << "connect succeeded: connect(this, &MyFileListWidget::removeItemSignal, this, &MyFileListWidget::removeItem)\n";
#endif // _DEBUG
	}

	initialize(parent, pathsList, config);
}


void MyFileListWidget::refreshSelf()
{
	checkFilesChangeThreadExit = true;
	for (auto iter = checkFilesChangeThreads.begin(); iter != checkFilesChangeThreads.end(); iter++)
	{
		std::thread th;
		th.swap(*iter);
		if (th.joinable())
			th.join();
	}
	checkFilesChangeThreadExit = false;
	if (dragArea)
	{
		dragArea->deleteLater();
		dragArea = nullptr;
	}
	if (selectionArea)
	{
		selectionArea->reset();
		selectionArea->deleteLater();
		selectionArea = nullptr;
	}

	for (auto iter = itemsMap.begin(); iter != itemsMap.end(); iter++)
		if (iter->second.item)
			static_cast<MyFileListItem*>(iter->second.item)->QPushButton::deleteLater();
	itemSize = QSize();
	itemsNumPerColumn = 0;
	selectionRect = QRect();

	indexesState = L"0";
	itemsMap.clear();
	initialize(parent, pathsList, configFileNameWithPath);
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
void MyFileListWidget::openProgram(std::wstring exeFilePath, std::wstring parameter, int nShowCmd, std::wstring workDirectory, HWND msgOrErrWindow)
{
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
			//for (auto iter = upperCmdParameter.begin(); iter != upperCmdParameter.end(); iter++)
			//{
			//	if ((*iter) >= L'a' && (*iter) <= L'z')
			//		(*iter) = (*iter) - L'a' + L'A';
			//}
			for (std::wstring r : rep)
			{
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

void MyFileListWidget::MenuClickedProc(QAction* action)
{
	if (action->text() == "刷新");
}

void MyFileListWidget::mousePressEvent(QMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::MouseButton::LeftButton:
	{
		if (selectionArea)
		{
			QPoint p = e->pos();// mapToParent(e->pos());
			selectionRect = QRect(p.x(), p.y(), 0, 0);
			selectionArea->reset();
			selectionArea->raise();
			selectionArea->show();
			selectionArea->move((selectionRect.left() < selectionRect.right() ? selectionRect.left() : selectionRect.right()),
				(selectionRect.top() < selectionRect.bottom() ? selectionRect.top() : selectionRect.bottom()));
			selectionArea->reset();
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
		if(selectionArea)
			selectionArea->hide();
	}
		break;
	case Qt::MouseButton::RightButton:
	{
		QPoint curPos = QCursor::pos();
		//创建菜单
		MyMenu* menu1 = new MyMenu(this);
		MyMenuAction* actionRefresh = new MyMenuAction(iconRefresh, "刷新\tE");
		connect(actionRefresh, &MyMenuAction::triggered, this, &MyFileListWidget::refreshSelf);
		MyMenuAction* pasteAction = new MyMenuAction(iconPaste, "粘贴\tP");
		connect(pasteAction, &MyMenuAction::triggered, this, &MyFileListWidget::pasteProc);
		MyMenuAction* newFileOrFolderAction = new MyMenuAction(iconCirclePlus, "新建\tW");
		MyMenuAction* myFileListWidgetAction = new MyMenuAction(QIcon(), "Widget Control");
		//二级菜单：Widget控制
		MyMenu* widgetControlMenu = new MyMenu(menu1);
		widgetControlMenu->addAction("关闭Widget", this, &MyFileListWidget::close);
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
		QVariant v = shellNewContents.value("Classes");
		if (v.typeId() == QMetaType::QStringList)
		{
			QStringList vl = v.toStringList();
			vl.insert(0, ".txt");
			std::cout << UTF8ToANSI("新建一栏扩展名：");
			for (qsizetype i = 0; i < vl.size(); i++)
			{
				std::cout << vl[i].toStdString() << " ";

				std::wstring extension = vl[i].toStdWString();
				if (extension == L".library-ms" || extension == L".lnk"/* || extension == L"Folder"*/)
					continue;
				if (extension.length() > 0)
				{
					SHFILEINFO info;
					if (SHGetFileInfo(extension.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES | SHGFI_ICON))
					{
						std::wstring type = info.szTypeName;
						QIcon icon = QPixmap::fromImage(QImage::fromHICON(info.hIcon));
						newOptions->addAction(icon, QString::fromStdWString(type));
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
		if (selectionArea)
		{
			QPoint p = e->pos();// mapToParent(e->pos());
			selectionRect.setBottomRight(p);
			selectionArea->move((selectionRect.left() < selectionRect.right() ? selectionRect.left() : selectionRect.right()),
				(selectionRect.top() < selectionRect.bottom() ? selectionRect.top() : selectionRect.bottom()));
			selectionArea->resize(selectionRect.width() >= 0 ? selectionRect.width() : -selectionRect.width(),
				selectionRect.height() >= 0 ? selectionRect.height() : -selectionRect.height());
			selectionArea->update();
			//selectionArea->mouseMoveProc(selectionRect);
			//std::cout << selectionRect.x() << "," << selectionRect.y()
			//	<< "," << selectionRect.width() << "," << selectionRect.height() << std::endl;
		}
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

void MyFileListWidget::dragEnterEvent(QDragEnterEvent* e)
{
	if (e->mimeData()->hasFormat("text/uri-list"))
		e->acceptProposedAction();
	if (dragArea)
	{
		//QPoint pos(e->globalPosition().x(), e->globalPosition().y());
		QPoint pos(e->position().toPoint().x(), e->position().toPoint().y());
		// std::cout << pos.x() << "," << pos.y() << std::endl;

		dragArea->moveRelative(
			QPoint(
				pos.x() - dragArea->getCursorPosOffsetWhenMousePress().x(),
				pos.y() - dragArea->getCursorPosOffsetWhenMousePress().y()
				),
			nullptr, this
			);
		dragArea->show();
	}

}

void MyFileListWidget::dragMoveEvent(QDragMoveEvent* e)
{
	if (dragArea)
	{
		dragArea->show();
		//QPoint pos(e->globalPosition().x(), e->globalPosition().y());
		//QPoint pos(e->position().toPoint().x(), e->position().toPoint().y());
		QCursor cur;
		QPoint pos(cur.pos());
		// std::cout << pos.x() << "," << pos.y() << std::endl;

		dragArea->moveRelative(
			QPoint(
				pos.x() - dragArea->getCursorPosOffsetWhenMousePress().x(),
				pos.y() - dragArea->getCursorPosOffsetWhenMousePress().y()
			),
			this, nullptr
		);
	}

}

// 当不接受QDrag的drop事件时调用
void MyFileListWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
	if (dragArea)
		dragArea->hide();

}

// 接受QDrag的drop时调用
void MyFileListWidget::dropEvent(QDropEvent* e)
{
	if(dragArea)
		dragArea->hide();
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

bool MyFileListWidget::readConfigFile(std::wstring nameWithPath, bool whetherToCreateItem)
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
					++lineCount;
#ifdef _DEBUG
					if (conf.size() < 3)
					{
						cout << UTF8ToANSI("配置文件存在错误:\n	line ") << lineCount << ":" << wstr2str_2ANSI(wstrLine) << endl;
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
					if (whetherToCreateItem)
						createItem(conf[0], conf[1]);
				}
			}
		}
	}
	return true;
}
bool MyFileListWidget::readConfigFileAndCreateItems(std::wstring nameWithPath)
{
	return readConfigFile(nameWithPath, true);
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
	while (!checkFilesChangeThreadExit)
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
					if (!itemsMap[nameWithPath].item && !isCreatingItem[path] && !isRemovingItem[path])
					{
						sendCreateItemSignal(tmpwstringarray[0], path);
						while (isCreatingItem[path]) Sleep(10);
					}
				}
				else
				{
					// Can't find the file in itemsMap
					// Send CreateItem Signal
					if (!isCreatingItem[path] && !isRemovingItem[path])
					{
						sendCreateItemSignal(tmpwstringarray[0], path);//isCreatingItem = true;已经写在函数内
						while (isCreatingItem[path]) Sleep(10);
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
			// 下面的注释因为：可能item并不存在而配置文件存在多余项需删除，
			// 并且函数内有判断，所以此处不需要判断
			//if (i->second.item != nullptr && i->second.path == path)
			if (i->second.path == path)
			{
				sendRemoveItemSignal(i->second.name, i->second.path);
				while (isRemovingItem[i->second.path]) Sleep(10);
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
				(yIndex - 1) * (itemSize.height()) + yIndex * itemSpacing.line
			);
			if (i->second.item)
			{
				if (static_cast<MyFileListItem*>(i->second.item)->size() != itemSize)
					emit static_cast<MyFileListItem*>(i->second.item)->adjustSizeSignal();
				if (itemPos != ((MyFileListItem*)i->second.item)->pos())
					emit static_cast<MyFileListItem*>(i->second.item)->moveSignal(itemPos);
			}
		}
		Sleep(10);
	}
}
//from csdn
HBITMAP GetThumbnail(const std::wstring& filePath)
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

void MyFileListWidget::createItem(std::wstring name, std::wstring path)
{
	isCreatingItem[path] = true;
	while (isRemovingItem[path]) Sleep(10);
	if (!PathCompletion(path))
	{
		isCreatingItem[path] = false;
		return;
	}
	std::wstring nameWithPath = path;
	nameWithPath += name;
	if (!itemsMap.count(nameWithPath))
		itemsMap[nameWithPath] = ItemProp();
	if (!itemsMap[nameWithPath].item)
	{
		/*创建新item*/
		MyFileListItem* pLWItem = new MyFileListItem(this, itemSize);

		/*新添加的item的图标*/
		QImage pLWItemImage;
		IShellItemImageFactory* ISIIFactory = nullptr;
		HRESULT hr = SHCreateItemFromParsingName(nameWithPath.c_str(), nullptr, __uuidof(IShellItemImageFactory), (void**)&ISIIFactory);
		if (SUCCEEDED(hr))
		{
			SIZE imageSize = { 256,256 };
			HBITMAP hBitmap;
			hr = ISIIFactory->GetImage(imageSize, SIIGBF_RESIZETOFIT, &hBitmap);
			if (SUCCEEDED(hr))
				pLWItemImage = QImage::fromHBITMAP(hBitmap);
			BITMAP bmp;
			GetObject(hBitmap, sizeof(BITMAP), (LPBYTE)&bmp);
			std::cout << "\t\t" << bmp.bmHeight << "\n";
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
				pLWItemImage = QImage::fromHICON(sfi.hIcon);
		}
		std::wstring::size_type st;
		if (itemsMap[nameWithPath].position == -1)
		{
			/*判断是否已经放置图标*/
			st = indexesState.find(L'0');
			if (st == std::wstring::npos)
			{
				st = indexesState.length();
				indexesState += L'0';
			}
			indexesState.replace(st, 1, L"1");
		}
		else
		{
			st = itemsMap[nameWithPath].position;
			for (size_t i = indexesState.length(); i < st + 2; i++)
				indexesState += L"0";
			indexesState[st] = '1';
		}
		if (itemsNumPerColumn == 0)
			itemsNumPerColumn = 1;
		/*计算新添加的item的合适位置的index*/
		int xIndex = st / itemsNumPerColumn + 1;
		int yIndex = st % itemsNumPerColumn + 1;
		pLWItem->setText(wstr2str_2UTF8(name).c_str());
		pLWItem->setViewMode(viewMode);
		pLWItem->setPath(path);
		pLWItem->setImage(pLWItemImage);
		pLWItem->setSelectionArea(selectionArea);
		pLWItem->setGrabArea(dragArea);
		//pLWItem->adjustSize();
		QPoint itemPos = QPoint(
			(xIndex - 1) * (itemSize.width()) + xIndex * itemSpacing.column, 
			(yIndex - 1) * (itemSize.height()) + yIndex * itemSpacing.line);
		pLWItem->move(itemPos);
		//pLWItem->setImage(QImage::fromHICON(sfi.hIcon));
		connect(pLWItem, &MyFileListItem::removeSelfSignal, this, [=]() {
			sendRemoveItemSignal(pLWItem->text().toStdWString(), pLWItem->getPath());
			});
		connect(pLWItem, &MyFileListItem::checkChange, this, [=](bool checkState) {
			if (checkState)
			{
				ItemProp ip = itemsMap[path + name];
				if (dragArea)
					dragArea->addItem(ip);
			}
			else
			{
				if (dragArea)
					dragArea->removeItem(name, path);
			}
			});
		
		/*将新添加的item加入map*/
		auto position = itemsMap[nameWithPath].position;
		itemsMap[nameWithPath] = { pLWItem, name, path, (position == -1 ? (long long)st : position)};
		pLWItem->show();
	}
	writeConfigFile(configFileNameWithPath);
	isCreatingItem[path] = false;
}

void MyFileListWidget::removeItem(std::wstring name, std::wstring path)
{
	isRemovingItem[path] = true;
	while (isCreatingItem[path]) Sleep(10);
	std::wstring nameWithPath = path + name;
	if (itemsMap.count(nameWithPath))
	{
		void*& item = itemsMap[nameWithPath].item;
		if (item)
		{
			static_cast<MyFileListItem*>(item)->deleteLater();
			item = nullptr;
		}
		itemsMap.erase(nameWithPath);
	}
	writeConfigFile(configFileNameWithPath);
	isRemovingItem[path] = false;
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

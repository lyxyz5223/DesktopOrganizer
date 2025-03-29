#pragma once
#include <qwidget.h>
#include "MyFileListItem.h"
#include "SelectionArea.h"
#include "DragArea.h"
#include "MyMenu.h"
#include <map>
#include <unordered_map>
#include <set>
#include <queue>
#include <functional>
#include <mutex>
#include "lib/SQLite/sqlite3.h"
#include "FileChangesChecker.h"

class MyFileListWidget : public QWidget
{
	Q_OBJECT
private:
	//计数，当对象创建(调用构造函数)时，计数+1
	//析构时，计数-1
	static long long useCount;
public:
	static long long getCount() {
		return useCount;
	}
	enum ConfigMode {
		File,
		Database
	};
	struct WindowInfo {
		MyFileListWidget* window = nullptr;
		std::wstring title = L"";
		QRect rect = QRect();
	};

private:// 属性定义区
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;

	std::unordered_map<std::wstring/*name with path*/, ItemProp> itemsMap;// 文件列表
	std::mutex mtxItemsMap;// 读取/写入文件列表的互斥锁

	std::vector<std::wstring> pathsList;// 文件路径列表
	std::wstring indexesState = L"0";// 按列计算的索引状态，1表示已占用，0表示未占用

	//窗口id
	long long windowId = 0;
	//窗口id列表
	static std::vector<long long> windowIdList;
	static std::mutex mtxWindowIdList;// 读取/写入窗口id列表的互斥锁
	//子窗口列表(包括本窗口)
	std::unordered_map<long long/*windowId*/, WindowInfo>& windowsMap;

	//配置相关
	ConfigMode configMode = ConfigMode::File;
	//ConfigMode == File
	std::wstring configName;//如果是File模式，则为配置文件路径；如果是数据库，则为数据表名称
	std::wstring windowsConfigName;//如果是File模式，则为配置文件路径；如果是数据库，则为数据表名称
	static std::mutex mtxWindowsConfigFile;// 读取/写入窗口配置文件的互斥锁
	static std::mutex mtxConfigFile;// 读取/写入配置文件的互斥锁
	//ConfigMode == Database
	sqlite3* database = nullptr;//数据库


	//QFont itemFont;
	Spacing itemSpacing = { 10, 10 };
	//Spacing itemSpacing = { 15, 15 };
	QWidget* parent = nullptr;// 父控件
	QColor backgroundColor = QColor(255, 255, 255, 1);
	QColor borderColor = QColor(0, 0, 0, 150);
	QColor textColor = QColor(borderColor.red(), borderColor.green(), borderColor.blue(), 255);
	bool ifShowTitle = false;
	QRect titleBarGeometry = QRect(0, 0, 200, 20);
	QRect titleBarFrameGeometry;// 用于鼠标移动窗口,在paintEvent函数中被修改
	enum TitleBarPositionMode {
		Coord,
		TopLeft,
		TopCenter,
		TopRight,
		BottomLeft,
		BottomCenter,
		BottomRight,
	} titleBarPositionMode = TitleBarPositionMode::TopCenter;

	bool canResize = false;
	struct WindowMoveResize {
		enum ResizeDirection {
			None = 0x0000,
			Left = 0x0001,
			Right = 0x0010,
			Top = 0x0100,
			Bottom = 0x1000,
			TopLeft = Top | Left,
			TopRight = Top | Right,
			BottomLeft = Bottom | Left,
			BottomRight = Bottom | Right
		} resizeDirection = None;
		bool move = false;
		QPoint mouseDownPos = QPoint();
		QRect originGeo = QRect();
		//POINT mouseDownPos = { 0 };
		//RECT originGeo = { 0 };
	} windowMoveResize;
	double borderWidth = 3.0;

	const QIcon iconRefresh = QIcon(":/DesktopOrganizer/img/iconoir--refresh.svg");
	const QIcon iconCMD = QIcon(":/DesktopOrganizer/img/terminal.ico");
	const QIcon iconPaste = QIcon(":/DesktopOrganizer/img/tabler--clipboard.svg");
	const QIcon iconCopy = QIcon(":/DesktopOrganizer/img/tabler--copy.svg");
	const QIcon iconCut = QIcon(":/DesktopOrganizer/img/tabler--cut.svg");
	const QIcon iconMore = QIcon(":/DesktopOrganizer/img/tabler--dots-vertical.svg");
	const QIcon iconPlus = QIcon(":/DesktopOrganizer/img/tabler--plus.svg");
	const QIcon iconSquarePlus = QIcon(":/DesktopOrganizer/img/tabler--square-plus.svg");
	const QIcon iconSquareRoundedPlus = QIcon(":/DesktopOrganizer/img/tabler--square-rounded-plus.svg");
	const QIcon iconCirclePlus = QIcon(":/DesktopOrganizer/img/tabler--circle-plus.svg");
	const QIcon& iconAdd = iconPlus;
	const QIcon& iconSquareAdd = iconSquarePlus;
	const QIcon& iconSquareRoundedAdd = iconSquareRoundedPlus;
	const QIcon& iconCircleAdd = iconCirclePlus;
	QIcon iconFolder;
	QIcon iconLink;
	//临时
	const int zoomScreen = 10;//item的高度是屏幕高度/宽度中小的那个的1/zoomScreen倍
	const int zoomScreenWidth = 20; ///item的宽度屏幕宽度1 / zoomScreen倍
	void changeItemSizeAndNumbersPerColumn();
	//std::unordered_map<std::wstring, bool> isRemovingItem;
	//std::unordered_map<std::wstring, bool> isCreatingItem;
	QRect selectionRect;// 框选区域
	SelectionArea* selectionArea = nullptr;// 框选区域图形
	size_t itemsNumPerColumn = 0;
	QSize itemSize;
	DragArea* dragArea = nullptr;// 选中文件拖动区域
	//std::vector<std::thread> checkFilesChangeThreads;
	std::vector<FileChangesChecker*> fileChangesCheckerList;
	bool checkFilesChangeThreadExit = false;
	//typedef void (MyFileListWidget::* ItemTask) (std::wstring name, std::wstring path);
	//任务队列，当有创建和删除item的信号时，都会添加任务到队列，随后将按照顺序依次执行队列任务
	struct ItemTask {
		void (MyFileListWidget::*task) (std::wstring name, std::wstring path) = nullptr;
		std::wstring name;
		std::wstring path;
		bool operator==(const ItemTask& itemTask) const {
			if (this != nullptr)
				return (task == itemTask.task && name == itemTask.name && path == itemTask.path);
			return false;
		}
		bool operator!=(const ItemTask& itemTask) const {
			return !operator==(itemTask);
		}
	};
	std::queue<ItemTask> itemTaskQueue;
	std::mutex mtxItemTaskQueue; // 互斥锁
	std::thread itemTaskThread;
	//bool itemTaskFinished = false;//GUI线程是否已经完成了创建/删除item的信号
	std::condition_variable cvItemTaskFinished;
	std::mutex mtxItemTaskExecute;// 创建/删除item过程中的互斥锁

public:
	~MyFileListWidget() {
		//计数-1
		useCount -= 1;
		checkFilesChangeThreadExit = true;
		//for (auto iter = checkFilesChangeThreads.begin(); iter != checkFilesChangeThreads.end(); iter++)
		//	iter->join();
		for (auto iter = fileChangesCheckerList.begin(); iter != fileChangesCheckerList.end(); iter++)
			delete* iter;
		fileChangesCheckerList.clear();
	}
	//MyFileListWidget(QWidget* parent = nullptr);
	MyFileListWidget(QWidget* parent,//父亲控件
		std::vector<std::wstring> pathsList,//文件路径列表
		std::wstring config,//配置文件
		std::wstring windowsConfig,//窗口配置文件
		long long windowId,//窗口Id
		bool isToolbox,//是否是窗口中的工具箱
		bool showTitle,//是否显示标题栏
		std::wstring title);

	MyFileListWidget(QWidget* parent,//父亲控件
		std::wstring config,//配置文件
		std::wstring windowsConfig,//窗口配置文件
		long long windowId,//窗口Id
		std::unordered_map<long long, WindowInfo>& windowsMap,//窗口映射表
		bool showTitle,//是否显示标题栏
		std::wstring title = L"");
	//初始化函数
	void refreshInitialize(QWidget* parent,
		std::vector<std::wstring> pathsList,
		std::wstring config,
		std::wstring windowsConfig);
	void publicInitialize(QWidget* parent,//父亲控件
		std::wstring config,//配置文件
		std::wstring windowsConfig,//窗口配置文件
		long long windowId,//窗口Id
		bool isToolbox,//是否是窗口中的工具箱
		bool showTitle,//是否显示标题栏
		std::wstring title = L"");

	//背景颜色设置
	void setBackgroundColor(QColor color) {
		backgroundColor = color;
	}
	QColor getBackgroundColor() const {
		return backgroundColor;
	}
	void setWindowTitle(const QString& title) {
		windowsMap[windowId].title = title.toStdWString();
		writeWindowsConfig(windowsConfigName);
		QWidget::setWindowTitle(title);
	}
	//创建子窗口
	MyFileListWidget* createChildWindow(QWidget* parent = nullptr,
		QString windowTitle = "DesktopOrganizer SubWindow",
		std::wstring config = L"",
		std::wstring windowsConfig = L"",
		long long windowId = 0,
		bool bShowTitle = true,
		QRect defaultGeometry = QRect(),
		bool bShow = true) {
		if (!parent)
			parent = this;
		MyFileListWidget* newWidget = new MyFileListWidget(parent, config, windowsConfig, windowId, windowsMap, bShowTitle, windowTitle.toStdWString());
		newWidget->setWindowTitle(windowTitle);
		newWidget->setIfShowTitle(true);
		newWidget->setTitleBarPositionMode(TitleBarPositionMode::TopCenter);
		//if (defaultPosition == QPoint())
		//	defaultPosition = mapFromGlobal(QCursor::pos());
		newWidget->setGeometry(defaultGeometry);
#ifdef _DEBUG
		newWidget->setBackgroundColor(backgroundColor);
#endif
		newWidget->setCanResize(true);
		if(bShow)
			newWidget->show();
		return newWidget;
	}

	//标题栏函数
	bool getIfShowTitle() const {
		return ifShowTitle;
	}
	void setIfShowTitle(bool ifShowTitle) {
		this->ifShowTitle = ifShowTitle;
	}
	QSize getTitleBarSize() const {
		return titleBarGeometry.size();
	}
	void setTitleBarSize(QSize size) {
		titleBarGeometry.setSize(size);
		setMinimumHeight(titleBarGeometry.y() + titleBarGeometry.height());
	}
	QPoint getTitleBarPosition() const {
		return titleBarGeometry.topLeft();
	}
	void setTitleBarPosition(QPoint pos) {
		titleBarGeometry.moveTo(pos);
		setMinimumHeight(titleBarGeometry.y() + titleBarGeometry.height());
	}
	QRect getTitleBarGeometry() const {
		return titleBarGeometry;
	}
	void setTitleBarGeometry(QRect rect) {
		titleBarGeometry = rect;
		setMinimumHeight(titleBarGeometry.y() + titleBarGeometry.height());
	}
	void setTitleBarPositionMode(TitleBarPositionMode mode) {
		titleBarPositionMode = mode;
	}
	TitleBarPositionMode getTitleBarPositionMode() const {
		return titleBarPositionMode;
	}
	//边框
	void setCanResize(bool b) {
		canResize = b;
	}
	bool getCanResize() const {
		return canResize;
	}

	void setViewMode(MyFileListItem::ViewMode mode) {
		viewMode = mode;
	}

	//配置文件的分割使用
	std::vector<std::wstring> splitForConfig(std::wstring text, std::wstring delimiter = L" "/*separator,分隔符*/, std::wstring EscapeString = L"" /*char EscapeCharacter*/);
	//注册表字符串分割使用
	std::vector<std::wstring> splitForShellExecuteFromRegedit(std::wstring text, std::wstring delimiter = L" ", std::wstring escapeString = L"");
	//添加文件夹监测路径
	void addPath(std::wstring path) {
		pathsList.push_back(path);
	}

	//配置文件
	bool readWindowsConfig(std::wstring nameWithPath);
	bool writeWindowsConfig(std::wstring nameWithPath);
	bool readConfig(std::wstring nameWithPath, bool whetherToCreateItem = false);
	bool writeConfig(std::wstring nameWithPath);
	//数据库设置
	void setSQLite3Database(sqlite3* pDatabase) {
		this->database = pDatabase;
	}
	//获取数据库
	sqlite3* getSQLite3Database() const {
		return this->database;
	}

	//监视文件夹变动
	void checkFilesChangeProc(std::wstring path);

public:
	//item添加删除
	bool isItemTaskInQueue(ItemTask task);
	[[deprecated]] void addItemTask(ItemTask task);
	void addItemTaskIfNotInQueue(ItemTask task);
	void itemTaskExecuteProc();
	//发送创建item信号，并且写入配置
	void sendCreateItemSignalAndWriteConfig(std::wstring name, std::wstring path) {
		std::unique_lock<std::mutex> ulMtxItemTask(mtxItemTaskExecute); // 互斥锁管理器，允许在不同线程间传递，允许手动加锁解锁
		std::cout << "Send CreateItem Signal\n";
		emit createItemSignal(name, path);
		cvItemTaskFinished.wait(ulMtxItemTask);
		writeConfig(configName);
	}
	//发送删除item信号，并且写入配置
	void sendRemoveItemSignalAndWriteConfig(std::wstring name, std::wstring path) {
		std::unique_lock<std::mutex> ulMtxItemTask(mtxItemTaskExecute); // 互斥锁管理器，允许在不同线程间传递，允许手动加锁解锁
		std::cout << "Send RemoveItem Signal\n";
		emit removeItemSignal(name, path);
		cvItemTaskFinished.wait(ulMtxItemTask);
		writeConfig(configName);
	}
	//打开软件
	static void openProgram(std::wstring exeFilePath, std::wstring parameter, int nShowCmd = SW_NORMAL, std::wstring workDirectory = L"", HWND msgOrErrWindow = NULL);
	//从注册表中获取文件图标Icon和QIcon
	static HICON ExtractIconFromRegString(std::wstring regString, HINSTANCE hInst = 0);
	static QIcon ExtractQIconFromRegString(QString regString, HINSTANCE hInst = 0) {
		return QIcon(QPixmap::fromImage(QImage::fromHICON(ExtractIconFromRegString(regString.toStdWString(), hInst))));
	}
	//从注册表中加载DLL内的字符串
	static std::wstring LoadDllStringFromRegString(std::wstring regString);
	//从注册表中加载菜单栏项目
	void addActionsFromRegedit(QString path, MyMenu* menu);

signals:
	void createItemSignal(std::wstring name, std::wstring path);
	void removeItemSignal(std::wstring name, std::wstring path);
	
public slots:
	void createItem(std::wstring name, std::wstring path);
	void removeItem(std::wstring name, std::wstring path);
	void MenuClickedProc(QAction* action);
	void refreshSelf();
	void openCMD(std::wstring path);
	void openPowerShell(std::wstring path);
	void pasteProc() {}
	void cutProc() {}
	void copyProc() {}
	void showDesktopOldPopupMenu(QPoint cursorPos = QPoint());
	std::wstring getTemplateFileNameWithPathFromReg(std::wstring extension/*扩展名*/);
	bool newFileProc(std::wstring extension, std::wstring path);
	bool createNewFile(std::wstring newFileName, std::wstring path, std::wstring templateFileNameWithPath);

protected:
	void focusInEvent(QFocusEvent* e) override;
	void focusOutEvent(QFocusEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	//窗口移动事件，执行时保存窗口配置文件
	void moveEvent(QMoveEvent* e) override;
	//窗口大小改变事件，执行时保存窗口配置文件
	void resizeEvent(QResizeEvent* e) override;
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	bool eventFilter(QObject* watched, QEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* e) override;
	void dragMoveEvent(QDragMoveEvent* e) override;
	void dragLeaveEvent(QDragLeaveEvent* e) override;
	void dropEvent(QDropEvent* e) override;
	bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result);
};

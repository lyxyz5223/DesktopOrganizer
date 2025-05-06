#pragma once
#include <qwidget.h>
#include "MyFileListItem.h"
#include "SelectionArea.h"
//#include "DragArea.h"
#include "MyMenu.h"
#include <map>
#include <unordered_map>
#include <set>
#include <queue>
#include <functional>
#include <any>
#include <mutex>
#include "lib/SQLite/sqlite3.h"
#include "FileChangesChecker.h"
#include "FunctionWrapper.h"


//数据库配置管理器
#include "ConfigManager.h"


#define LOGLOCK(name) std::cout << "will lock "#name"\n";\
try {\
	std::lock_guard<std::mutex> lock(name);/*互斥锁加锁*/\
}\
catch (std::system_error& e) {\
	std::cout << "lock "#name" failed: " << e.what() << std::endl;\
}\
catch (...) {\
	std::cout << "lock "#name" failed: unknown error" << std::endl;\
}\
std::cout << "locked "#name"\n";

class MyAbstractFileListWidget : public QWidget{
public:

};

class MyFileListWidget : public MyAbstractFileListWidget
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
	enum class ConfigMode {
		File,
		Database,
		DatabaseConfigManager
	};
	struct WindowInfo {
		MyFileListWidget* window = nullptr;
		std::wstring title = L"";
		QRect rect = QRect();
	};
	struct Index {
		long long x = 0,
			y = 0;
		bool isValid() const {
			return x > 0 && y > 0;
		}
		QPoint toPos(QSize itemSize, Spacing itemSpacing) const {
			return QPoint(
				(x - 1) * (itemSize.width()) + x * itemSpacing.column,
				(y - 1) * (itemSize.height()) + y * itemSpacing.line
			);
		}
	};
	struct Range {
		Index start;
		Index end;
	};
	enum TitleBarPositionMode {
		Coord,
		TopLeft,
		TopCenter,
		TopRight,
		BottomLeft,
		BottomCenter,
		BottomRight,
	};

private:// 属性定义区
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

	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;

	//文件路径名字之间与item属性的映射表
	std::unordered_map<std::wstring/*name with path*/, ItemProp> itemsMap;// 文件列表
	std::unordered_map<long long, std::wstring> positionNameWithPathMap;// position到name with path的映射表
	std::mutex mtxItemsMap;// 读取/写入文件列表的互斥锁

	std::vector<std::wstring> pathsList;// 文件路径列表
	//std::wstring indexesState = L"0";// 按列计算的索引状态，1表示已占用，0表示未占用

	//多选item
	Qt::KeyboardModifiers keyboardModifiers = Qt::KeyboardModifier::NoModifier;

	//窗口id
	long long windowId = 0;
	//子窗口列表(包括本窗口)
	std::unordered_map<long long/*windowId*/, WindowInfo> childrenWindowsMap;
	std::mutex mtxChildrenWindowsMap;//id-子窗口映射表的互斥锁

	//配置相关
	ConfigMode configMode = ConfigMode::DatabaseConfigManager;
	//ConfigMode == File
	std::wstring configName;//如果是File模式，则为配置文件路径；如果是数据库，则为数据表名称
	std::wstring windowsConfigName;//如果是File模式，则为配置文件路径；如果是数据库，则为数据表名称
	//static std::mutex mtxWindowsConfigFile;// 读取/写入窗口配置文件的互斥锁
	//static std::mutex mtxConfigFile;// 读取/写入配置文件的互斥锁
	//ConfigMode == Database
	//sqlite3* database = nullptr;//数据库，注释掉：不要多线程同时使用一个db
	DatabaseConfigManager databaseManager;
	std::string databaseFileName = "config.db";
	DatabaseConfigManager::TableStruct windowsConfigTableStruct = {
		{//columns
			{ "id", DatabaseConfigManager::SQLite3DataType::BIGINT },
			{ "title", DatabaseConfigManager::SQLite3DataType::TEXT },
			{ "x", DatabaseConfigManager::SQLite3DataType::INTEGER },
			{ "y", DatabaseConfigManager::SQLite3DataType::INTEGER },
			{ "width", DatabaseConfigManager::SQLite3DataType::INTEGER },
			{ "height", DatabaseConfigManager::SQLite3DataType::INTEGER }
		},
		//primary keys index
		{ 0 },
		//dropRowId
		true
	};
	DatabaseConfigManager::TableStruct configTableStruct = {
		{//columns
			{ "windowId", DatabaseConfigManager::SQLite3DataType::BIGINT },
			{ "name", DatabaseConfigManager::SQLite3DataType::TEXT },
			{ "path", DatabaseConfigManager::SQLite3DataType::TEXT },
			{ "position", DatabaseConfigManager::SQLite3DataType::BIGINT }
		},
		//primary keys index
		{ 1,2 },
		//dropRowId
		true
	};



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
	//标题栏位置
	TitleBarPositionMode titleBarPositionMode = TitleBarPositionMode::TopCenter;

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

	//临时
	bool initialized = false;//窗口是否已经初始化完成
	const int zoomScreen = 10;//item的高度是屏幕高度/宽度中小的那个的1/zoomScreen倍
	const int zoomScreenWidth = 20; ///item的宽度屏幕宽度1 / zoomScreen倍
	void changeItemSizeAndNumbersPerColumn();
	std::thread sizeCalculateThread;
	//std::unordered_map<std::wstring, bool> isRemovingItem;
	//std::unordered_map<std::wstring, bool> isCreatingItem;
	SelectionArea* selectionArea = new SelectionArea(this);// 框选区域图形结构
	long long itemsNumPerColumn = 0;
	QSize itemSize;
	DragArea* dragArea = nullptr;// 选中文件拖动区域


	std::vector<FileChangesChecker*> fileChangesCheckerList;
	bool checkFilesChangeThreadExit = false;
	//typedef void (MyFileListWidget::* ItemTask) (std::wstring name, std::wstring path);
	//任务队列，当有创建和删除item的信号时，都会添加任务到队列，随后将按照顺序依次执行队列任务

	struct ItemTask {
		class Any : public std::any {
		public:
			bool operator==(const Any& a) const {
				//return type() == a.type() && std::any_cast<std::any>((std::any)*this) == std::any_cast<std::any>((std::any)a);
				return (*this) == a;
			}
		};
		enum Task {
			Create,
			Remove,
			Rename
		} task;
		std::vector<std::any> args;

		//std::wstring name;
		//std::wstring path;
		bool operator==(const ItemTask& itemTask) const {
			if (this != nullptr)
			{
				if (task != itemTask.task || args.size() != itemTask.args.size())
					return false;
				for (auto i = 0; i < args.size(); i++)
				{
					if (args[i].type() != itemTask.args[i].type()
						|| std::any_cast<std::wstring>(args[i]) != std::any_cast<std::wstring>(itemTask.args[i]))
						return false;
				}
			}
				//return (task == itemTask.task && args == itemTask.args);
				//return (task == itemTask.task && name == itemTask.name && path == itemTask.path);
			return true;
		}
		bool operator!=(const ItemTask& itemTask) const {
			return !operator==(itemTask);
		}
	};

	//struct ItemTask {
	//	FunctionWrapper task;
	//	bool operator==(const ItemTask& itemTask) const {
	//		return task == itemTask.task;
	//	}
	//};

	//item task任务相关
	std::queue<ItemTask> itemTaskQueue;
	std::mutex mtxItemTaskQueue; // 互斥锁
	std::thread itemTaskThread;
	//bool itemTaskFinished = false;//GUI线程是否已经完成了创建/删除item的信号
	std::condition_variable cvItemTaskFinished;
	std::mutex mtxItemTaskExecute;// 创建/删除item过程中的互斥锁

public:
	~MyFileListWidget();

	MyFileListWidget(QWidget* parent,//父亲控件
		std::vector<std::wstring> pathsList,//文件路径列表
		std::wstring config,//配置文件
		std::wstring childrenWindowsConfig,//窗口配置文件
		long long windowId,//窗口Id，不同窗口具有唯一不同的id
		bool showTitle = false,//是否显示标题栏
		std::wstring title = L"",
		bool isMainWindow = false);


	//初始化函数
	void refreshInitialize(QWidget* parent,
		std::vector<std::wstring> pathsList,
		std::wstring config);
	void publicInitialize(QWidget* parent,//父亲控件
		std::wstring config,//配置文件
		std::wstring childrenWindowsConfig,//窗口配置文件
		long long windowId,//窗口Id
		bool showTitle,//是否显示标题栏
		std::wstring title = L"",
		bool isMainWindow = false);

	//背景颜色设置
	void setBackgroundColor(QColor color) {
		backgroundColor = color;
	}
	QColor getBackgroundColor() const {
		return backgroundColor;
	}

	void setWindowTitle(const QString& title) {
		QWidget::setWindowTitle(title);
		writeWindowsConfig(windowsConfigName);
		emit updateWindowsConfig();
	}
	void setWindowTitle(const std::wstring& title) {
		setWindowTitle(QString::fromStdWString(title));
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
	//确保仅在dropEvent中调用
	bool writeConfig(std::wstring nameWithPath);
	enum DatabaseOperation {
		Read,
		Write
	};
	enum SQLite3DataType {
		INTEGER,
		BIGINT,
		TEXT
	};
	std::string SQLite3DataTypeToString(SQLite3DataType type) {
#define TRANSITIONCASE(type) case SQLite3DataType::type: return #type;
		switch (type) {
			TRANSITIONCASE(INTEGER)
			TRANSITIONCASE(BIGINT)
			case SQLite3DataType::TEXT: return "TEXT";
		default: return "";
		}
	}
	struct DatabaseColumn {
		std::string name;
		SQLite3DataType type;
	};
	struct DatabasePreparation {
		//std::wstring databaseName;
		std::wstring tableName;
		std::vector<DatabaseColumn> columns;
		std::vector<unsigned long long> primaryKeyIndex;
	};

	std::tuple<sqlite3*, sqlite3_stmt*> prepareDatabase(DatabasePreparation dbp, DatabaseOperation operation);//返回值禁止在多线程中使用
	//监视文件夹变动
	void checkFilesChange(std::wstring path, bool isSingleShot = true);
	//void checkFilesChangeSingleShotProc(std::wstring path);
	void close() {
		hide();//先隐藏窗口，防止继续操作
		stopAllThread();//关闭所有子线程
		emit willDeleteWindow(windowId);//处理关闭前窗口内存在的item
		QWidget::close();//最后再关闭窗口
	}

public:
	//item添加删除
	bool isItemTaskInQueue(ItemTask task);
	[[deprecated]] void addItemTask(ItemTask task);
	void addItemTaskIfNotInQueue(ItemTask task);
	void itemTaskExecuteProc();

	// 计算index索引
	Index calculateIndex(long long position) const {
		int xIndex = position / itemsNumPerColumn + 1;
		int yIndex = position % itemsNumPerColumn + 1;
		return {xIndex, yIndex};
	}
	QPoint calculatePosFromIndex(Index index) const {
		return index.toPos(itemSize, itemSpacing);
	}
	long long calculatePositionNumberFromIndex(Index index) const {
		long long result = (index.x - 1) * itemsNumPerColumn + (index.y - 1);
		return result;
	}
	long long calculatePositionNumberFromPos(QPoint pos) const {
		Index index {
			pos.x() / (itemSize.width() + itemSpacing.column),//计算得出当前item所在的位置
			pos.y() / (itemSize.height() + itemSpacing.line)
		};
		index = calculateRelativeIndex(index, 1, 1);
		auto position = calculatePositionNumberFromIndex(index);
		if (position >= 0)
			return position;
		else
			return 0;
	}

	//索引相对位置的计算
	// 向右xOffset>0,向下yOffset>0
	long long calculateRelativePosition(long long position,
		int xOffset, int yOffset) const {
		Index index = calculateIndex(position);
		index = calculateRelativeIndex(index, xOffset, yOffset);
		return calculatePositionNumberFromIndex(index);
	}
	Index calculateRelativeIndex(Index index, long long xOffset, long long yOffset) const {
		index.x += xOffset;
		index.y += yOffset;
		index.x += index.y / itemsNumPerColumn;
		index.y %= itemsNumPerColumn;
		return index;
	}
	std::vector<Index> getItemsInRange(Range range);
	std::vector<Index> getSelectedItemsInRange(Range range);

	//发送创建item信号，并且写入配置
	void sendCreateItemSignalAndWriteConfig(std::wstring name, std::wstring path);
	//发送删除item信号，并且写入配置
	void processRemoveItem(std::wstring name, std::wstring path);
	void sendRemoveItemSignalAndWriteConfig(std::wstring name, std::wstring path);
	//处理重命名文件（因为子窗口也需要判断该item是不是自己的，所以需要递归执行）
	void processRenameItem(std::wstring oldName, std::wstring path, std::wstring newName);
	//发送重命名item信号，并且写入配置
	void sendRenameItemSignalAndWriteConfig(std::wstring oldName, std::wstring path, std::wstring newName);
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
	void CreateDesktopPopupMenu();

private:
	signed long long getAvailableWindowId();
	signed long long getAvailablePosition();
	void changeDragAreaItem(bool checkState, MyFileListItem* item);
	auto& getItemsMap() {
		return itemsMap;
	}
	auto& getPositionNameWithPathMap() {
		return positionNameWithPathMap;
	}
	void fileRename(std::wstring oldName, std::wstring path, std::wstring newName);

	auto changeItemParentToThis(MyFileListItem* item, MyFileListWidget* source, long long position) {
		if (source != this)
		{
			std::wstring nwp = item->getPath() + item->text().toStdWString();
			source->getItemsMap()[nwp].windowId = this->windowId;
			source->changeDragAreaItem(false, item);
			disconnect(source, &MyFileListWidget::selectionAreaResized, item, &MyFileListItem::judgeSelection);
			disconnect(item, &MyFileListItem::checkChange, source, 0);
			disconnect(item, &MyFileListItem::renamed, this, &MyFileListWidget::fileRename);
			item->setParent(this);
			auto pos = calculatePosFromIndex(calculateIndex(position));
			item->move(pos);
			//itemsMap[nwp] = source->getItemsMap()[nwp];
			//positionNameWithPathMap[position] = nwp;
			connect(this, &MyFileListWidget::selectionAreaResized, item, &MyFileListItem::judgeSelection);
			connect(item, &MyFileListItem::checkChange, this, [=](bool checkState) {
				changeDragAreaItem(checkState, item);
				}/*, Qt::QueuedConnection*/);
			connect(item, &MyFileListItem::renamed, this, &MyFileListWidget::fileRename);
			item->removeEventFilter(source);
			item->installEventFilter(this);
			item->show();
			changeDragAreaItem(true, item);
		}
	}

	void stopAllThread() {
		//关闭所有由该窗口创建的线程
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

signals:
	void createItemSignal(std::wstring name, std::wstring path);
	void removeItemSignal(std::wstring name, std::wstring path);
	void renameItemSignal(std::wstring oldName, std::wstring path, std::wstring newName);
	void selectionAreaResized(QRect newGeometry);
	//提醒父母窗口是时候更新窗口配置文件了
	void updateWindowsConfig();
	void willDeleteWindow(long long wid);

public slots:
	void createItem(std::wstring name, std::wstring path);
	void removeItem(std::wstring name, std::wstring path);
	void renameItem(std::wstring oldName, std::wstring path, std::wstring newName);
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
	void closeEvent(QCloseEvent* e) override;
};

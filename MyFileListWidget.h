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

class MyFileListWidget : public QWidget
{
	Q_OBJECT
private:// 属性定义区
	MyFileListItem::ViewMode viewMode = MyFileListItem::ViewMode::Icon;
	std::unordered_map<std::wstring/*name with path*/, ItemProp> itemsMap;// 文件列表
	std::vector<std::wstring> pathsList;// 文件路径列表
	std::wstring indexesState = L"0";// 按列计算的索引状态，1表示已占用，0表示未占用
	std::wstring configFileNameWithPath;
	//QFont itemFont;
	Spacing itemSpacing = { 10, 10 };
	//Spacing itemSpacing = { 15, 15 };
	QWidget* parent = nullptr;// 父控件

	QIcon iconRefresh = QIcon(":/DesktopOrganizer/img/iconoir--refresh.svg");
	QIcon iconCMD = QIcon(":/DesktopOrganizer/img/terminal.ico");
	QIcon iconPaste = QIcon(":/DesktopOrganizer/img/tabler--clipboard.svg");
	QIcon iconCopy = QIcon(":/DesktopOrganizer/img/tabler--copy.svg");
	QIcon iconCut = QIcon(":/DesktopOrganizer/img/tabler--cut.svg");
	QIcon iconMore = QIcon(":/DesktopOrganizer/img/tabler--dots-vertical.svg");
	QIcon iconPlus = QIcon(":/DesktopOrganizer/img/tabler--plus.svg");
	QIcon iconSquarePlus = QIcon(":/DesktopOrganizer/img/tabler--square-plus.svg");
	QIcon iconSquareRoundedPlus = QIcon(":/DesktopOrganizer/img/tabler--square-rounded-plus.svg");
	QIcon iconCirclePlus = QIcon(":/DesktopOrganizer/img/tabler--circle-plus.svg");
	QIcon& iconAdd = iconPlus;
	QIcon& iconSquareAdd = iconSquarePlus;
	QIcon& iconSquareRoundedAdd = iconSquareRoundedPlus;
	QIcon& iconCircleAdd = iconCirclePlus;
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
	std::vector<std::thread> checkFilesChangeThreads;
	bool checkFilesChangeThreadExit = false;
	//typedef void (MyFileListWidget::* ItemTask) (std::wstring name, std::wstring path);
	//任务队列，当有创建和删除item的信号时，都会添加任务到队列，随后将按照顺序依次执行队列任务
	struct ItemTask {
		void (MyFileListWidget::*task) (std::wstring name, std::wstring path) = nullptr;
		std::wstring name;
		std::wstring path;
		bool operator==(const ItemTask& itemTask) const {
			if ((this != nullptr) && (&itemTask) != nullptr)
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
	std::mutex mtxItemTask;// 创建/删除item过程中的互斥锁

public:
	~MyFileListWidget() {
		checkFilesChangeThreadExit = true;
		for (auto iter = checkFilesChangeThreads.begin(); iter != checkFilesChangeThreads.end(); iter++)
			iter->join();
	}
	//MyFileListWidget(QWidget* parent = nullptr);
	MyFileListWidget(QWidget* parent,
		std::vector<std::wstring> pathsList,
		std::wstring config);
	void initialize(QWidget* parent,
		std::vector<std::wstring> pathsList,
		std::wstring config);
	void setViewMode(MyFileListItem::ViewMode mode) {
		viewMode = mode;
	}
	std::vector<std::wstring> splitForConfig(std::wstring text, std::wstring delimiter = L" "/*separator,分隔符*/, std::wstring EscapeString = L"" /*char EscapeCharacter*/);
	std::vector<std::wstring> splitForShellExecuteFromRegedit(std::wstring text, std::wstring delimiter = L" ", std::wstring escapeString = L"");

	void addPath(std::wstring path) {
		pathsList.push_back(path);
	}
	bool readConfigFile(std::wstring nameWithPath, bool whetherToCreateItem = false);
	bool writeConfigFile(std::wstring nameWithPath);
	void checkFilesChangeProc(std::wstring path);

	bool isItemTaskInQueue(ItemTask task);
	void addItemTask(ItemTask task);
	void itemTaskExecuteProc();

	void sendCreateItemSignalAndWriteConfig(std::wstring name, std::wstring path) {
		std::unique_lock<std::mutex> ulMtxItemTask(mtxItemTask); // 互斥锁管理器，允许在不同线程间传递，允许手动加锁解锁
		std::cout << "Send CearteItem Signal\n";
		//isCreatingItem[path] = true;
		emit createItemSignal(name, path);
		cvItemTaskFinished.wait(ulMtxItemTask);
		writeConfigFile(configFileNameWithPath);
	}
	void sendRemoveItemSignalAndWriteConfig(std::wstring name, std::wstring path) {
		std::unique_lock<std::mutex> ulMtxItemTask(mtxItemTask); // 互斥锁管理器，允许在不同线程间传递，允许手动加锁解锁
		std::cout << "Send RemoveItem Signal\n";
		//isRemovingItem[path] = true;
		emit removeItemSignal(name, path);
		cvItemTaskFinished.wait(ulMtxItemTask);
		writeConfigFile(configFileNameWithPath);
	}
	static void openProgram(std::wstring exeFilePath, std::wstring parameter, int nShowCmd = SW_NORMAL, std::wstring workDirectory = L"", HWND msgOrErrWindow = NULL);
	static HICON ExtractIconFromRegString(std::wstring regString, _Reserved_ HINSTANCE hInst = 0);
	static QIcon ExtractQIconFromRegString(QString regString, _Reserved_ HINSTANCE hInst = 0) {
		return QIcon(QPixmap::fromImage(QImage::fromHICON(ExtractIconFromRegString(regString.toStdWString(), hInst))));
	}
	static std::wstring LoadDllStringFromRegString(std::wstring regString);
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
	void paintEvent(QPaintEvent* e) override;
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	bool eventFilter(QObject* watched, QEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* e) override;
	void dragMoveEvent(QDragMoveEvent* e) override;
	void dragLeaveEvent(QDragLeaveEvent* e) override;
	void dropEvent(QDropEvent* e) override;

};

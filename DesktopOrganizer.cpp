#include "DesktopOrganizer.h"
DesktopOrganizer::~DesktopOrganizer(){}//析构函数
//声明
BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam);//枚举窗口过程函数
HWND HWND_thisApp;//这个软件的HWND句柄
HWND HWND_WorkerW;//桌面内容（WorkerW）的HWND句柄
HWND HWND_SHELLDLL_DefView;
int xx, yy;//屏幕宽和高
int xxNo, yyNo;//除去任务栏

DesktopOrganizer::DesktopOrganizer(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    xx = GetSystemMetrics(SM_CXSCREEN);
    yy = GetSystemMetrics(SM_CYSCREEN);
    xxNo = GetSystemMetrics(SM_CXFULLSCREEN);
    yyNo = GetSystemMetrics(SM_CYFULLSCREEN);

    HWND_thisApp = (HWND)winId();
    //HWND HWND_SHELLDLL_DefView = FindWindow(L"Progman", L"Program Manager");
    //HWND_WorkerW = FindWindow(L"", L"WorkerW");
    EnumWindows(EnumWindowsProc, 0);
    HWND_SHELLDLL_DefView = ::FindWindowEx(HWND_WorkerW, NULL, L"SHELLDLL_DefView", NULL);
    if (HWND_SHELLDLL_DefView == NULL)
        exit(999);
    //ShowWindow(HWND_SHELLDLL_DefView, SW_HIDE);
    //Sleep(3000);
    //ShowWindow(HWND_SHELLDLL_DefView, SW_NORMAL);
    //PostMessage(HWND_SHELLDLL_DefView, WM_CLOSE, 0, 0);
    
    SetParent(HWND_thisApp, HWND_WorkerW);
    setAttribute(Qt::WA_TranslucentBackground, true);
    SetWindowLong(HWND_thisApp, GWL_EXSTYLE, GetWindowLong(HWND_thisApp, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
    SetWindowLong(HWND_thisApp, GWL_STYLE, GetWindowLong(HWND_thisApp, GWL_STYLE) | WS_CLIPSIBLINGS);
    SetWindowLong(HWND_thisApp, GWL_STYLE, GetWindowLong(HWND_thisApp, GWL_STYLE) & ~(WS_CAPTION | WS_BORDER | WS_THICKFRAME));
    SetClassLong(HWND_thisApp, GCL_STYLE, GetClassLong(HWND_thisApp, GCL_STYLE) | CS_HREDRAW | CS_VREDRAW);
    ::SetWindowPos(HWND_thisApp, NULL, 0, 0, xx, yy, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    //setWindowFlags(Qt::FramelessWindowHint);
    show();
    MoveWindow(HWND_thisApp, 0, 0, xxNo, yyNo,1);
    std::wcout.imbue(std::locale("chs"));
    //// 创建文件系统模型
    //QFileSystemModel* model = new QFileSystemModel(this);
    //// 设置模型的根目录wstr2str_2UTF8(desktopPath).c_str()
    //model->setRootPath("C:\\Users\\lyxyz5223\\Desktop\\MyDesktop");
    //std::cout << wstr2str_2UTF8(desktopPath).c_str();
    //// 创建树形视图并设置模型
    //QListView* view = new QListView(this);
    //view->setModel(model);
    //view->setRootIndex(model->index(wstr2str_2UTF8(desktopPath).c_str()));
    //re.setSize(QSize(re.width() / 2, re.height() / 2));
    //view->resize(re.size());
    //view->move(0, 0);
    //// 显示视图
    //view->show();
    QScreen* sc = this->screen();
    QRect re = sc->geometry();
    re = sc->availableGeometry();
    MyFileListWidget* pLWidget = new MyFileListWidget(this,"");
    pLWidget->setViewMode(MyFileListItem::ViewMode::Icon);
    pLWidget->setConfigFileName("config.ini");

    pLWidget->resize(re.size());
    this->resize(re.size());
    pLWidget->move(0, 0);
    pLWidget->show();
}

BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
    wchar_t WClass[256];
    GetClassName(hwnd, WClass, 256);
    if (wcscmp(WClass, L"WorkerW") == 0)
    {
        //if (FindWindowEx(hwnd, NULL, L"SysListView32", L"FolderView") != NULL)
        if (FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", L"") != NULL)
        {
            HWND_WorkerW = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}
void DesktopOrganizer::paintEvent(QPaintEvent* e)
{
    QPainter p(this);
    //p.fillRect(rect(), QColor(100, 55, 255, 0));
    //p.fillRect(rect(), QColor(255, 255, 255, 0));
    QPen pen;
    pen.setColor(QColor(110, 110, 119, 255));
    pen.setWidth(5);
    p.setPen(pen);
    p.drawRect(rect());
    QMainWindow::paintEvent(e);
}




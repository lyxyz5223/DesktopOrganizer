#include "DesktopOrganizer.h"

BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam);
struct ScreenSize{
    int width, height;
    int widthNoTaskBar, heightNoTaskBar;
} screenSize;
HWND HWND_thisApp;//这个软件的HWND句柄
HWND HWND_WorkerW;//桌面内容（WorkerW）的HWND句柄
HWND HWND_SHELLDLL_DefView;

DesktopOrganizer::DesktopOrganizer(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    std::wcout.imbue(std::locale("chs"));
    screenSize.width = GetSystemMetrics(SM_CXSCREEN);
    screenSize.height = GetSystemMetrics(SM_CYSCREEN);
    screenSize.widthNoTaskBar = GetSystemMetrics(SM_CXFULLSCREEN);
    screenSize.heightNoTaskBar = GetSystemMetrics(SM_CYFULLSCREEN);
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
    ::SetWindowPos(HWND_thisApp, NULL, 0, 0, screenSize.width, screenSize.height, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    //setWindowFlags(Qt::FramelessWindowHint);
    show();
    MoveWindow(HWND_thisApp, 0, 0, screenSize.widthNoTaskBar, screenSize.heightNoTaskBar, 1);

    QScreen* sc = this->screen();
    QRect re = sc->geometry();
    re = sc->availableGeometry();
    this->resize(re.size());
    MyFileListWidget* pLWidget = new MyFileListWidget(this,
        std::vector<std::wstring>({ L"C:\\Users\\lyxyz5223\\Desktop\\桌面软件测试" }/*文件夹路径C:\\Users\\lyxyz5223\\Desktop\\桌面软件测试*/),
        L"config.ini");
    pLWidget->setViewMode(MyFileListItem::ViewMode::Icon);
    //pLWidget->readConfigFile(L"config.ini");

    pLWidget->resize(re.size());
    pLWidget->move(0, 0);
    pLWidget->show();

}

DesktopOrganizer::~DesktopOrganizer()
{}

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
    //QPen pen;
    //pen.setColor(QColor(110, 110, 119, 255));
    //pen.setWidth(5);
    //p.setPen(pen);
    //p.drawRect(rect());
    QMainWindow::paintEvent(e);
}

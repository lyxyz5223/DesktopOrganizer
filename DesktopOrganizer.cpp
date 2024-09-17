#include "DesktopOrganizer.h"
DesktopOrganizer::~DesktopOrganizer(){}//析构函数
//声明
std::wstring desktopPath;//需要整理并且放于桌面的路径
BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam);//枚举窗口过程函数
HWND HWND_thisApp;//这个软件的HWND句柄
HWND HWND_WorkerW;//桌面内容（WorkerW）的HWND句柄
HWND HWND_SHELLDLL_DefView;
int xx, yy;//屏幕宽和高
int xxNo, yyNo;//除去任务栏
intptr_t GetFileNum(std::wstring path,bool isCountDirectory);
DWORD GetFilesArray(_In_ std::wstring path, _Out_ std::wstring** &filearray, _In_ intptr_t fileNum = 0);
void DisplayErrorBox(const wchar_t* lpszFunction);
std::string wstr2str_2UTF8(std::wstring text);
std::string wstr2str_2ANSI(std::wstring text);

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
    WCHAR cFilePath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_DESKTOP, 0, 0, cFilePath);
    //MessageBox(HWND_thisApp, cFilePath, L"", 0);
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
            MessageBox(HWND_thisApp, L"文件夹不存在且创建失败，请检查你是否有权限，或者尝试以管理员身份运行。程序将退出。", 0, MB_ICONERROR);
            exit(2);
        }
    if (desktopPath.back() != L'\\')
        desktopPath += L"\\";
    std::wstring** filesList;
    intptr_t fileNum = GetFileNum(desktopPath, true);
    GetFilesArray(desktopPath, filesList);
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
    MyFileListWidget* pLWidget = new MyFileListWidget(this);
    pLWidget->setViewMode(MyFileListItem::ViewMode::Icon);
    pLWidget->setConfigString()
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
        pLWidget->addItem(pLWItem,std::to_string(i+1));
        connect(pLWItem, &MyFileListItem::doubleClicked, this, [=]() {desktopItemProc(filesList[i][0]); });
    }

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

//获取文件数目
intptr_t GetFileNum(std::wstring path,bool isCountDirectory)
{
    
    using namespace std;

    intptr_t fileNum = 0;
    WIN32_FIND_DATA ffd;
    LARGE_INTEGER filesize;
    //WCHAR szDir[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;
    if (path.back() != L'\\')
        path += L"\\";
    path += L"*";
    hFind = FindFirstFile(path.c_str(), &ffd);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        DisplayErrorBox(TEXT("FindFirstFile"));
        //return dwError;
    }
    // List all the files in the directory with some info about them.
    do
    {
        if (wcscmp(ffd.cFileName, L".") != 0
            && wcscmp(ffd.cFileName, L"..") != 0)
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //_tprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
                //这是一个文件夹，名称为：ffd.cFileName
                if (isCountDirectory)
                    fileNum++;
            }
            else
            {
                filesize.LowPart = ffd.nFileSizeLow;
                filesize.HighPart = ffd.nFileSizeHigh;
                //_tprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
                //这是一个文件，名称为：ffd.cFileName，大小为filesize.QuadPart
                fileNum++;
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);
    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
    {
        DisplayErrorBox(TEXT("FindFirstFile"));
    }
    FindClose(hFind);
    //return dwError;
    return fileNum;
}
//微软官方示例，展示错误
void DisplayErrorBox(const wchar_t* lpszFunction1)
{
    // Retrieve the system error message for the last-error code
    LPTSTR lpszFunction = (LPTSTR)lpszFunction1;
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    // Display the error message and clean up

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
DWORD GetFilesArray(_In_ std::wstring path, _Out_ std::wstring** &filearray,_In_ intptr_t fileNum)//返回错误代码
{
    using namespace std;
    if(fileNum == 0)
        fileNum = GetFileNum(path, true);
    if (fileNum <= 0)
    {
        MessageBox(HWND_thisApp, L"错误，文件数目统计失败！程序即将退出", 0, MB_ICONERROR);
        exit(1);
    }
    filearray = new wstring*[fileNum+1];//用的时候指的是前面的中括号filearray[1][2];前面那个中括号里面的索引！
    for (int i = 0; i < fileNum + 1; i++)
    {
        filearray[i] = new wstring[2];
    }
    WCHAR* cFilePath;
    cFilePath = (WCHAR*)path.c_str();

    intptr_t FileIndex = 0;
    WIN32_FIND_DATA ffd;
    LARGE_INTEGER filesize;
    //TCHAR szDir[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;
    if (path.back() != L'\\')
        path += L"\\";
    path += L"*";
    // Find the first file in the directory.
    hFind = FindFirstFile(path.c_str(), &ffd);

    if (INVALID_HANDLE_VALUE == hFind)
    {
        DisplayErrorBox(TEXT("FindFirstFile"));
        return dwError;
    }
    // List all the files in the directory with some info about them.
    do
    {
        if (wcscmp(ffd.cFileName, L".") != 0
            && wcscmp(ffd.cFileName, L"..") != 0)
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                wprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
                //这是一个文件夹，名称为：ffd.cFileName
                filearray[FileIndex][0] = ffd.cFileName;
                filearray[FileIndex][1] = L"Directory";
            }
            else
            {
                filesize.LowPart = ffd.nFileSizeLow;
                filesize.HighPart = ffd.nFileSizeHigh;
                wprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
                //这是一个文件，名称为：ffd.cFileName，大小为filesize.QuadPart
                filearray[FileIndex][0] = ffd.cFileName;
                filearray[FileIndex][1] = L"File";

            }
            FileIndex++;
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
    {
        DisplayErrorBox(TEXT("FindFirstFile"));
    }
    FindClose(hFind);
    return dwError;
}
std::string wstr2str_2UTF8(std::wstring text)
{
    CHAR* str;
    int Tsize = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, 0, 0, 0, 0);
    str = new CHAR[Tsize];
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, str, Tsize, 0, 0);
    std::string str1 = str;
    delete[]str;
    return str1;
}
std::string wstr2str_2ANSI(std::wstring text)
{
    CHAR* str;
    int Tsize = WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, 0, 0, 0, 0);
    str = new CHAR[Tsize];
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, str, Tsize, 0, 0);
    std::string str1 = str;
    delete[]str;
    return str1;
}

void DesktopOrganizer::desktopItemProc(std::wstring name)
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
    ShellExecute(HWND_thisApp, L"open", name.c_str(), L"", desktopPath.c_str(), SW_NORMAL);
}

std::string UTF8ToANSI(std::string utf8Text)
{
    WCHAR* wstr;//中间量
    CHAR* str;//转换后的
    int Tsize = MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, 0, 0);
    wstr = new WCHAR[Tsize];
    MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, wstr, Tsize);
    Tsize = WideCharToMultiByte(CP_ACP, 0, wstr, -1, 0, 0, 0, 0);
    str = new CHAR[Tsize];
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, Tsize, 0, 0);
    std::string wstr1 = str;
    delete[]str;
    delete[]wstr;
    return wstr1;
}

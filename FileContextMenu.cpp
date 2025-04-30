#include "FileContextMenu.h"

FileContextMenu::FileContextMenu()
{

}

FileContextMenu::~FileContextMenu()
{

}

bool FileContextMenu::exec(POINT pos)
{
    //HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    //if (FAILED(hr)) {
    //    std::cerr << "CoInitializeEx failed" << std::endl;
    //    return;
    //}

    IShellFolder* pDesktopFolder = NULL;
    IShellFolder* pParentFolder = NULL;
    IContextMenu* pContextMenu = NULL;

    // 获取桌面文件夹接口
    HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
    if (FAILED(hr)) {
        std::cerr << "Failed to get desktop folder" << std::endl;
        CoUninitialize();
        return false;
    }

    // 获取文件父文件夹
    wchar_t parentPath[MAX_PATH];
    wcscpy_s(parentPath, MAX_PATH, filePath.c_str());
    PathRemoveFileSpecW(parentPath);

    LPITEMIDLIST pidlParent = NULL;
    hr = pDesktopFolder->ParseDisplayName(NULL, NULL, parentPath, NULL, &pidlParent, NULL);
    if (FAILED(hr)) {
        pDesktopFolder->Release();
        std::cerr << "Failed to parse parent folder" << std::endl;
        CoUninitialize();
        return false;
    }

    hr = pDesktopFolder->BindToObject(pidlParent, NULL, IID_IShellFolder, (void**)&pParentFolder);
    CoTaskMemFree(pidlParent);
    pDesktopFolder->Release();

    if (FAILED(hr)) {
        std::cerr << "Failed to bind to parent folder" << std::endl;
        CoUninitialize();
        return false;
    }

    // 获取文件PIDL
    LPITEMIDLIST pidlFile = NULL;
    hr = pParentFolder->ParseDisplayName(NULL, NULL, PathFindFileNameW(filePath.c_str()), NULL, &pidlFile, NULL);
    if (FAILED(hr)) {
        pParentFolder->Release();
        std::cerr << "Failed to parse file" << std::endl;
        CoUninitialize();
        return false;
    }

    // 获取上下文菜单
    hr = pParentFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlFile, IID_IContextMenu, NULL, (void**)&pContextMenu);
    CoTaskMemFree(pidlFile);
    pParentFolder->Release();

    if (FAILED(hr)) {
        std::cerr << "Failed to get context menu" << std::endl;
        CoUninitialize();
        return false;
    }

    // 创建弹出菜单
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        pContextMenu->Release();
        std::cerr << "Failed to create menu" << std::endl;
        CoUninitialize();
        return false;
    }

    // 填充菜单项
    hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXPLORE);
    if (FAILED(hr)) {
        DestroyMenu(hMenu);
        pContextMenu->Release();
        std::cerr << "Failed to query context menu" << std::endl;
        CoUninitialize();
        return false;
    }

    // 显示菜单并处理选择
    int cmd = TrackPopupMenuEx(hMenu,
        TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
        pos.x, pos.y, hwnd, NULL);

    if (cmd > 0) {
        CMINVOKECOMMANDINFO cmi = { sizeof(cmi) };
        cmi.lpVerb = MAKEINTRESOURCEA(cmd - 1);
        cmi.nShow = SW_SHOWNORMAL;
        cmi.hwnd = hwnd;

        hr = pContextMenu->InvokeCommand(&cmi);
        if (FAILED(hr)) {
            std::cerr << "Failed to invoke command" << std::endl;
        }
    }

    // 清理资源
    DestroyMenu(hMenu);
    pContextMenu->Release();
    //CoUninitialize();
}

void FileContextMenu::show()
{

}

// 使用示例
//int main() {
//    FileContextMenu menu;
//    if (menu.Show(nullptr, L"C:\\test.txt")) {
//        std::cout << "菜单操作成功\n";
//    }
//    else {
//        std::cerr << "菜单操作失败! 错误代码: " << GetLastError() << "\n";
//    }
//    return 0;
//}
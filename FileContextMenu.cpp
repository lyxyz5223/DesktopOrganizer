#include "FileContextMenu.h"
#include <thread>

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
	//	std::wcerr << L"CoInitializeEx failed: " << std::hex << hr << std::endl;
	//	return;
	//}
	if (files.empty())
		return false;
	IShellFolder* pDesktopFolder = NULL; // 虽然我们直接解析路径，但这是传统方式
	LPITEMIDLIST pidlFull = NULL;
	LPCITEMIDLIST pidlChild = NULL; // 注意这里是LPCITEMIDLIST，因为SHBindToParent返回const
	IShellFolder* pParentFolder = NULL;
	IContextMenu* pContextMenu = NULL;
	IContextMenu2* pContextMenu2 = NULL;
	IContextMenu3* pContextMenu3 = NULL;
	HMENU hMenu = NULL;
	//HRESULT hr = S_OK;
	//1. 将路径解析为完整的 PIDL
	HRESULT hr = SHParseDisplayName(files[0].c_str(), NULL, &pidlFull, 0, NULL);
	if (FAILED(hr))
	{
		std::wcerr << L"SHParseDisplayName failed for path: " << files[0 ] << L" Error: " << std::hex << hr << std::endl;
		return false;
	}
	//2. 绑定到父文件夹并获取子 PIDL 和父 IShellFolder
	hr = SHBindToParent(pidlFull, IID_IShellFolder, (void**)&pParentFolder, &pidlChild);
	if (FAILED(hr))
	{
		std::wcerr << L"SHBindToParent failed: " << std::hex << hr << std::endl;
		if (pidlFull) CoTaskMemFree(pidlFull);
		return false;
	}
	// 3. 获取 IContextMenu 接口
	//hwnd: 父窗口，用于消息处理
	//1: 要获取接口的对象数量 (pidlChild)
	//&pidlChild: 对象的PIDL数组
	//IID_IContextMenu: 请求的接口ID
	//NULL: 保留
	//(void**)&pContextMenu: 返回的接口指针
	hr = pParentFolder->GetUIObjectOf(hwnd, 1, &pidlChild, IID_IContextMenu, NULL, (void**)&pContextMenu);
	if (FAILED(hr))
	{
		std::wcerr << L"GetUIObjectOf (IContextMenu) failed: " << std::hex << hr << std::endl;
		if (pParentFolder) pParentFolder->Release();
		if (pidlFull) CoTaskMemFree(pidlFull);
		return false;
	}


	// 4. 尝试获取更高级的 IContextMenu3 或 IContextMenu2 接口
	// 这些接口的 HandleMenuMsg/HandleMenuMsg2 方法对于动态填充子菜单（如“打开方式”）至关重要
	if (SUCCEEDED(pContextMenu->QueryInterface(IID_IContextMenu3, (void**)&pContextMenu3)))
	{
		g_pCurrentContextMenu3 = pContextMenu3; // pContextMenu3 已 AddRef by QueryInterface
		g_pCurrentContextMenu2 = NULL; // 确保另一个是 NULL
	}
	else if (SUCCEEDED(pContextMenu->QueryInterface(IID_IContextMenu2, (void**)&pContextMenu2)))
	{
		g_pCurrentContextMenu2 = pContextMenu2; // pContextMenu2 已 AddRef
		g_pCurrentContextMenu3 = NULL;
	}
	else
	{
		// 如果都失败，WindowProc 中的消息处理将不会调用 HandleMenuMsg(2)
		// 但基本的菜单项仍然应该由 pContextMenu->QueryContextMenu填充
		g_pCurrentContextMenu2 = NULL;
		g_pCurrentContextMenu3 = NULL;
	}

	// 5. 创建弹出菜单
	hMenu = CreatePopupMenu();
	if (!hMenu)
	{
		std::wcerr << L"CreatePopupMenu failed." << std::endl;
		// goto cleanup; (使用goto不是最佳实践，但可以简化多处错误处理的资源释放)
		if (pContextMenu3) pContextMenu3->Release();
		if (pContextMenu2) pContextMenu2->Release();
		if (pContextMenu) pContextMenu->Release();
		if (pParentFolder) pParentFolder->Release();
		if (pidlFull) CoTaskMemFree(pidlFull);
		return false;
	}

	// 6. 填充菜单项
	// CMF_NORMAL, CMF_EXPLORE, CMF_CANRENAME 等标志控制菜单内容
	// CMF_EXTENDEDVERBS 会显示更详细的动词（如果可用）
	// CMF_RESERVED - 不要使用，除非你知道你在做什么
	hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXPLORE | CMF_CANRENAME);
	if (FAILED(hr))
	{
		std::wcerr << L"QueryContextMenu failed: " << std::hex << hr << std::endl;
		DestroyMenu(hMenu);
		if (pContextMenu3) pContextMenu3->Release();
		if (pContextMenu2) pContextMenu2->Release();
		if (pContextMenu) pContextMenu->Release();
		if (pParentFolder) pParentFolder->Release();
		if (pidlFull) CoTaskMemFree(pidlFull);
		return false;
	}

	// 7. 显示菜单
	// SetForegroundWindow(hwnd); // 确保我们的窗口是前景窗口
	POINT pt = pos;
	int cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, msgWindow, NULL);
	// PostMessage(hwnd, WM_NULL, 0, 0); // 有时用于解决菜单消失后的焦点问题

	// 8. 处理菜单命令
	if (cmd > 0)
	{
		CMINVOKECOMMANDINFOEX ici = { sizeof(CMINVOKECOMMANDINFOEX) };
		ici.fMask = CMIC_MASK_UNICODE;
		if (GetKeyState(VK_CONTROL) < 0) ici.fMask |= CMIC_MASK_CONTROL_DOWN; // 示例：如果Ctrl键按下
		if (GetKeyState(VK_SHIFT) < 0) ici.fMask |= CMIC_MASK_SHIFT_DOWN;   // 示例：如果Shift键按下

		ici.hwnd = hwnd;
		ici.lpVerb = MAKEINTRESOURCEA(cmd - 1); // 命令ID从1开始，lpVerb是0基的
		ici.lpVerbW = MAKEINTRESOURCEW(cmd - 1);
		ici.nShow = SW_SHOWNORMAL;
		ici.ptInvoke = pt; // 提供调用点
		ici.fMask |= CMIC_MASK_PTINVOKE;

		// 优先使用 IContextMenu3 或 IContextMenu2 的 InvokeCommand (它们通常只是调用基类的)
		if (pContextMenu3) {
			hr = pContextMenu3->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
		}
		else if (pContextMenu2) {
			hr = pContextMenu2->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
		}
		else {
			hr = pContextMenu->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
		}

		if (FAILED(hr)) {
			std::wcerr << L"InvokeCommand failed for command " << cmd << L". Error: " << std::hex << hr << std::endl;
		}
	}

	// 9. 清理全局指针 (在 TrackPopupMenuEx 之后，InvokeCommand 之前或之后都可以)
	// 全局指针的 AddRef 是在上面 QueryInterface 时隐式完成的，或者如果手动 AddRef 则需要手动 Release
	// 这里 pContextMenu2/3 是局部变量，它们会在函数结束时 Release。
	// g_p... 指向的是与 pContextMenu2/3 相同的对象，所以当 pContextMenu2/3 Release 时，
	// 如果引用计数为0，对象会被销毁。
	// 我们应该在 TrackPopupMenuEx 之后，但在局部 pContextMenu2/3 Release 之前，
	// 将全局指针置空，以防止 WindowProc 在菜单已销毁后还尝试使用它们。
	// 实际上，QueryInterface 返回的指针需要我们自己 Release。
	// g_p... 只是一个副本，不需要单独 AddRef/Release，除非你把它们用作独立的生命周期管理。
	// 为了安全，我们显式 Release 全局变量引用的接口，并将它们置空。
	// 但由于它们只是局部接口的别名，所以当局部接口被释放时它们也会失效。
	// 正确的做法是，如果全局变量要“拥有”这个接口一段时间，它应该 AddRef。
	// 在这个模型中，g_p... 的生命周期严格绑定到 ShowFileContextMenu 的执行期间，
	// 并且在 TrackPopupMenuEx 期间被 WindowProc 使用。

	// 将全局指针置空，这样WindowProc在菜单结束后不会再使用它们
	// 它们指向的实际COM对象将由下面的局部变量Release
	g_pCurrentContextMenu3 = NULL;
	g_pCurrentContextMenu2 = NULL;

	// 10. 释放资源
	DestroyMenu(hMenu);
	if (pContextMenu3) pContextMenu3->Release();
	if (pContextMenu2) pContextMenu2->Release();
	if (pContextMenu) pContextMenu->Release(); // 总是释放基础接口
	if (pParentFolder) pParentFolder->Release();
	if (pidlFull) CoTaskMemFree(pidlFull); // pidlChild 是 pidlFull 的一部分
	return true;
}

bool FileContextMenu::execMultiFiles(POINT pos)
{
	if (files.empty())
	{
		std::wcerr << L"No files provided to show context menu." << std::endl;
		return false;
	}
	IShellFolder* pDesktopFolder = NULL;
	std::vector<LPITEMIDLIST> childPidlsVector; // Store PIDLs that need to be freed
	IContextMenu* pContextMenu = NULL;
	IContextMenu2* pContextMenu2 = NULL;
	IContextMenu3* pContextMenu3 = NULL;
	HMENU hMenu = NULL;

	// 1. 获取桌面 IShellFolder 接口
	HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
	if (FAILED(hr))
	{
		std::wcerr << L"SHGetDesktopFolder failed: " << std::hex << hr << std::endl;
		return false;
	}
	// 自动释放的字符串
	class AutoReleaseWCHARPointer {
	private:
		wchar_t* p = nullptr;
	public:
		~AutoReleaseWCHARPointer() {
			delete[] p;
		}
		AutoReleaseWCHARPointer(size_t size) {
			p = new wchar_t[size];
			ZeroMemory(p, size * sizeof(wchar_t)); // 初始化为0
		}
		operator wchar_t* () {
			return p;
		}
	};
	//IShellItemArray* itemsArray = nullptr;
	//std::vector<LPITEMIDLIST> itemsList;
	//for (auto file : files)
	//{
	//	IShellItem* item = nullptr;
	//	hr = SHCreateItemFromParsingName(file.c_str(), NULL, IID_IShellItem, reinterpret_cast<void**>(&item));
	//	if (FAILED(hr))
	//	{
	//		std::wcerr << L"SHCreateItemFromParsingName failed for file: " << file << std::endl;
	//		break;
	//	}
	//	LPITEMIDLIST idList = nullptr;
	//	SHGetIDListFromObject(item, &idList);
	//	itemsList.emplace_back(idList);
	//}
	//if (FAILED(hr))
	//{
	//	for (auto& item : itemsList)
	//		CoTaskMemFree(item);
	//	if (pDesktopFolder)
	//		pDesktopFolder->Release();
	//	return false;
	//}
	//hr = SHCreateShellItemArrayFromIDLists(files.size(), const_cast<LPCITEMIDLIST*>(itemsList.data()), &itemsArray);
	//if (FAILED(hr))
	//{
	//	std::wcerr << L"SHCreateShellItemArrayFromIDLists failed, Error: " << std::hex << hr << std::endl;
	//	for (auto& item : itemsList)
	//		CoTaskMemFree(item);
	//	if (pDesktopFolder)
	//		pDesktopFolder->Release();
	//	return false;
	//}
	//hr = itemsArray->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARGS(&pContextMenu));
	//if (FAILED(hr))
	//{
	//	std::wcerr << L"BindToHandler failed, Error: " << std::hex << hr << std::endl;
	//	for (auto& item : itemsList)
	//		CoTaskMemFree(item);
	//	if (pDesktopFolder)
	//		pDesktopFolder->Release();
	//	return false;
	//}

	// 2. 为每个文件路径获取其相对于桌面的子 PIDL
	for (const auto& fullPath : files)
	{
		// ParseDisplayName 需要一个可写的字符串作为它的第三个参数 LPOLESTR (LPWSTR)
		// PathFindFileName 返回 const WCHAR*, 所以需要复制它
		AutoReleaseWCHARPointer mutableFileName(fullPath.size() + 1);
		LPCWSTR lpcFileName = PathFindFileName(fullPath.c_str());
		std::wstring fileName = lpcFileName;
		if (!lpcFileName || !*lpcFileName)
		{
			std::wcerr << L"Could not extract file name from path: " << fullPath << std::endl;
			hr = E_FAIL; // Mark as failed
			break;
		}
		wcscpy_s(mutableFileName, fileName.size() + 1, fileName.c_str());
		LPITEMIDLIST pidlChild = NULL;
		SFGAOF attributes = 0; // Not strictly needed here, can be NULL for last param
		hr = pDesktopFolder->ParseDisplayName(hwnd, NULL, mutableFileName, NULL, &pidlChild, &attributes);
		if (FAILED(hr) || !pidlChild)
		{
			std::wcerr << L"pParentFolder->ParseDisplayName failed for file: " << fileName << L" Error: " << std::hex << hr << std::endl;
			// 错误方案：
			// 如果失败，说明文件可能属于该文件夹的子文件夹
			// 先获取该文件的父文件夹
			//AutoReleaseWCHARPointer mutableFilePath(fullPath.size() + 1);
			//wcscpy_s(mutableFilePath, fullPath.size() + 1, fullPath.c_str());
			//if (!PathRemoveFileSpec(mutableFilePath))
			//{
			//    std::wcerr << L"Could not remove file spec from path: " << fullPath << std::endl;
			//    hr = E_FAIL; // Mark as failed
			//    break;
			//}
			//// 获取parentFolder的pidlParent(LPITEMIDLIST)
			//LPITEMIDLIST pidlParent = NULL;
			//hr = pDesktopFolder->ParseDisplayName(hwnd, NULL, mutableFilePath, NULL, &pidlParent, NULL);
			//if (FAILED(hr))
			//{
			//    std::wcerr << L"pDesktopFolder->ParseDisplayName failed for file: " << fileName << L" Error: " << std::hex << hr << std::endl;
			//    break;
			//}
			//// 获取parentFolder的IShellFolder
			//IShellFolder* pParentFolder = nullptr;
			//hr = pDesktopFolder->BindToObject(pidlParent, NULL, IID_IShellFolder, (void**)&pParentFolder);
			//CoTaskMemFree(pidlParent);//资源释放
			//hr = pParentFolder->ParseDisplayName(hwnd, NULL, mutableFileName, NULL, &pidlChild, &attributes);
			//if (FAILED(hr) || !pidlChild)
			//{
			//    std::wcerr << L"pParentFolder->ParseDisplayName failed for file: " << fileName << L" Error: " << std::hex << hr << std::endl;
			//}

			// 正确方案
			// 使用相对路径转换，从而获取基于当前目录下的文件名，统一使用pDesktopFolder来获取文件的pidl
			// 参数：
			// 1: 接收相对路径的缓冲区
			// 2: 基础目录路径
			// 3: 基础目录属性（FILE_ATTRIBUTE_DIRECTORY表示是目录, 否则视为文件）
			// 4: 要转换的目标路径
			// 5: 目标路径属性
			PWSTR pszPath = NULL;
			hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &pszPath);
			if (FAILED(hr))
			{
				std::wcerr << L"SHGetKnownFolderPath failed for file: " << fullPath << L" Error: " << std::hex << hr << std::endl;
				break;
			}
			AutoReleaseWCHARPointer mutableRelativePathFileName(fullPath.size() + 1);
			BOOL bSuccess = PathRelativePathTo(mutableRelativePathFileName, pszPath, FILE_ATTRIBUTE_DIRECTORY, fullPath.c_str(), NULL);
			if (!bSuccess)
			{
				hr = E_FAIL;
				std::wcerr << L"PathRelativePathTo failed for file: " << fullPath << std::endl;
				break;
			}
			std::wstring relativePathFileName;
			relativePathFileName = mutableRelativePathFileName;
			relativePathFileName = relativePathFileName.substr(2);
			wcscpy_s(mutableRelativePathFileName, fullPath.size() + 1, relativePathFileName.c_str());
			hr = pDesktopFolder->ParseDisplayName(hwnd, NULL, mutableRelativePathFileName, NULL, &pidlChild, &attributes);
			if (FAILED(hr) || !pidlChild)
			{
				std::wcerr << L"pParentFolder->ParseDisplayName failed for file: " << fileName << L" Error: " << std::hex << hr << std::endl;
				break;
			}
		}
		childPidlsVector.push_back(pidlChild);
	}
	// 遍历完成
	if (FAILED(hr))
	{
		// If any ParseDisplayName failed
		for (LPITEMIDLIST pidl : childPidlsVector)
			CoTaskMemFree(pidl);
		if (pDesktopFolder)
			pDesktopFolder->Release();
		return false;
	}	
	// 列表中所有的子PIDL都成功解析
	// 3. 获取 IContextMenu 接口 (针对所有选中的文件)
	// pParentFolder 是这些文件的父文件夹
	// childPidlsVector.data() 是子PIDL数组
	// childPidlsVector.size() 是文件数量
	hr = pDesktopFolder->GetUIObjectOf(hwnd,
		static_cast<UINT>(childPidlsVector.size()),
		const_cast<LPCITEMIDLIST*>(childPidlsVector.data()),
		IID_IContextMenu,
		NULL,
		(void**)&pContextMenu
	);


	// 无法获取IContextMenu
	if (FAILED(hr))
	{
		std::wcerr << L"GetUIObjectOf (IContextMenu) failed: " << std::hex << hr << std::endl;
		for (LPITEMIDLIST pidl : childPidlsVector)
			CoTaskMemFree(pidl);
		if (pDesktopFolder)
			pDesktopFolder->Release();
		return false;
	}

	// 4. 尝试获取 IContextMenu3 或 IContextMenu2
	if (SUCCEEDED(pContextMenu->QueryInterface(IID_IContextMenu3, (void**)&pContextMenu3)))
	{
		g_pCurrentContextMenu3 = pContextMenu3;
		g_pCurrentContextMenu2 = NULL;
	}
	else if (SUCCEEDED(pContextMenu->QueryInterface(IID_IContextMenu2, (void**)&pContextMenu2)))
	{
		g_pCurrentContextMenu2 = pContextMenu2;
		g_pCurrentContextMenu3 = NULL;
	}
	else
	{
		g_pCurrentContextMenu2 = NULL;
		g_pCurrentContextMenu3 = NULL;
	}

	// 5. 创建菜单
	hMenu = CreatePopupMenu();
	if (!hMenu)
	{
		std::wcerr << L"CreatePopupMenu failed." << std::endl;
		// goto cleanup equivalent
		if (pContextMenu3)
			pContextMenu3->Release();
		if (pContextMenu2)
			pContextMenu2->Release();
		if (pContextMenu)
			pContextMenu->Release();
		for (LPITEMIDLIST pidl : childPidlsVector)
			CoTaskMemFree(pidl);
		if (pDesktopFolder)
			pDesktopFolder->Release();
		return false;
	}

	// 6. 填充菜单项
	hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXPLORE | CMF_CANRENAME);
	if (FAILED(hr))
	{
		std::wcerr << L"QueryContextMenu failed: " << std::hex << hr << std::endl;
		DestroyMenu(hMenu);
		// goto cleanup equivalent
		if (pContextMenu3)
			pContextMenu3->Release();
		if (pContextMenu2)
			pContextMenu2->Release();
		if (pContextMenu)
			pContextMenu->Release();
		for (LPITEMIDLIST pidl : childPidlsVector)
			CoTaskMemFree(pidl);
		if (pDesktopFolder)
			pDesktopFolder->Release();
		return false;
	}

	// 7. 显示菜单
	POINT pt = pos;
	int cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, msgWindow, NULL);

	// 8. 处理菜单命令
	if (cmd > 0)
	{
		//if (cmd - 1 == 0x11)
		//{
		//	// 处理删除操作
		//	// 可以InvokeCommand或直接调用删除函数
		//}
		if (cmd - 1 == 0x12)// 重命名的谓词：0x12
		{
			// 处理重命名操作
			if (handleRenameFunction)
				handleRenameFunction();
		}
		else// 非重命名操作
		{
			CMINVOKECOMMANDINFOEX ici = { sizeof(CMINVOKECOMMANDINFOEX) };
			ici.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE; // PTINVOKE is good practice
			if (GetKeyState(VK_CONTROL) < 0)
				ici.fMask |= CMIC_MASK_CONTROL_DOWN;
			if (GetKeyState(VK_SHIFT) < 0)
				ici.fMask |= CMIC_MASK_SHIFT_DOWN;

			ici.hwnd = hwnd;
			ici.lpVerb = MAKEINTRESOURCEA(cmd - 1);
			ici.lpVerbW = MAKEINTRESOURCEW(cmd - 1);
			ici.nShow = SW_SHOWNORMAL;
			ici.ptInvoke = pt; // The point where the menu was invoked

			IContextMenu* pInvoker = pContextMenu; // Default to base IContextMenu
			if (g_pCurrentContextMenu3)
				pInvoker = g_pCurrentContextMenu3;
			else if (g_pCurrentContextMenu2)
				pInvoker = g_pCurrentContextMenu2;

			hr = pInvoker->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
			if (FAILED(hr))
			{
				std::wcerr << L"InvokeCommand failed for command(cmd - 1): " << cmd - 1 << L". Error: " << std::hex << hr << std::endl;
			}
		}
	}

	// 9. 清理全局指针
	g_pCurrentContextMenu3 = NULL;
	g_pCurrentContextMenu2 = NULL;

	// 10. 释放资源
	DestroyMenu(hMenu);
	if (pContextMenu3)
		pContextMenu3->Release();
	if (pContextMenu2)
		pContextMenu2->Release();
	if (pContextMenu)
		pContextMenu->Release();
	for (LPITEMIDLIST pidl : childPidlsVector)
		CoTaskMemFree(pidl);
	if (pDesktopFolder)
		pDesktopFolder->Release();

	return true;
}

void FileContextMenu::show()
{
	POINT pos;
	GetCursorPos(&pos);
	std::thread([pos, this]() {
		exec(pos);
	}).detach();
}

// 实现隐藏消息窗口类
FileContextMenu::MenuMessageWindow::MenuMessageWindow(IContextMenu2*& iContextMenu2, IContextMenu3*& iContextMenu3)
	: iContextMenu2(iContextMenu2), iContextMenu3(iContextMenu3)
{
	Create();
}

FileContextMenu::MenuMessageWindow::~MenuMessageWindow()
{
	Destroy();
}

bool FileContextMenu::MenuMessageWindow::Create()
{
	if (m_hWnd)
		return true;

	WNDCLASSEXW wc = { sizeof(WNDCLASSEX) };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = L"FileContextMenu_MsgWnd";

	if (!RegisterClassExW(&wc)) 
		return false;

	m_hWnd = CreateWindowExW(0, wc.lpszClassName, nullptr, 0,
		0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, this);

	return m_hWnd != NULL;
}

void FileContextMenu::MenuMessageWindow::Destroy() {
	if (m_hWnd) {
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
		UnregisterClassW(L"FileContextMenu_MsgWnd", GetModuleHandle(nullptr));
	}
}

LRESULT CALLBACK FileContextMenu::MenuMessageWindow::WndProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 用户自定义数据
	auto pThis = reinterpret_cast<MenuMessageWindow*>(
		GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	//创建窗口时，添加用户数据
	if (uMsg == WM_CREATE)
	{
		auto pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		pThis = reinterpret_cast<MenuMessageWindow*>(pCreate->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		return 0;
	}
	//处理窗口消息，菜单处理
	switch (uMsg)
	{
	case WM_INITMENUPOPUP:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_MENUSELECT: // WM_MENUCHAR 也可以考虑
	{
		LRESULT lres = 0; // 用于 HandleMenuMsg2 的结果
		BOOL bHandled = FALSE;
		if (pThis->iContextMenu3) {
			if (SUCCEEDED(pThis->iContextMenu3->HandleMenuMsg2(uMsg, wParam, lParam, &lres))) {
				bHandled = TRUE;
			}
		}
		else if (pThis->iContextMenu2) {
			if (SUCCEEDED(pThis->iContextMenu2->HandleMenuMsg(uMsg, wParam, lParam))) {
				bHandled = TRUE;
				// HandleMenuMsg (ICM2) 通常不修改 lres，其返回值更多是 S_OK/S_FALSE
				// 对于特定消息，可能需要根据 uMsg 决定 WindowProc 的返回值
				if (uMsg == WM_INITMENUPOPUP) lres = 0;
				else if (uMsg == WM_DRAWITEM || uMsg == WM_MEASUREITEM) lres = TRUE;
				// else lres = 0; // 默认
			}
		}

		if (bHandled) {
			// 如果 HandleMenuMsg2 被调用且成功，lres 包含了应该返回的值
			// 如果 HandleMenuMsg (ICM2) 被调用且成功，需要根据消息类型决定返回值
			return lres;
		}

		// 如果未被 IContextMenu2/3 处理，则可能需要默认处理
		// DefWindowProc 通常会处理这些消息，但对于弹出菜单，
		// 如果扩展不处理，通常意味着没什么特别的要做。
		// 对于 WM_INITMENUPOPUP，如果未处理，子菜单可能为空。
	}
	break; // 重要：从 case 出来，让 DefWindowProc 处理其他情况或默认行为
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
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
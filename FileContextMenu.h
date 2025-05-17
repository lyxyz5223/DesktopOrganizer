#pragma once
#ifndef FILECONTEXTMENU_H
#define FILECONTEXTMENU_H
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <atlbase.h>
#include <iostream>
#include <qwidget.h>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")

class FileContextMenu {
private:
    HWND hwnd = NULL;
	std::vector<std::wstring> files;
	std::function<void()> handleRenameFunction = nullptr;
	std::function<void()> handleDeleteFunction = nullptr;
	std::function<void()> handleCopyFunction = nullptr;
	std::function<void()> handleCutFunction = nullptr;
	std::function<void()> handlePasteFunction = nullptr;

public:
    FileContextMenu();
    ~FileContextMenu();
    bool exec(POINT pos);
	bool execMultiFiles(POINT pos);
    bool exec(HWND hwnd, POINT pos) {
        setParent(hwnd);
        return exec(pos);
    }
    bool exec(QWidget* parent, POINT pos) {
		setParent(parent);
		return exec(pos);
	}
    bool exec(HWND hwnd, int x, int y) {
        setParent(hwnd);
		POINT pos;
		pos.x = x;
		pos.y = y;
		return exec(hwnd, pos);
	}
    bool exec(QWidget* parent, int x, int y) {
        setParent(parent);
        POINT pos;
        pos.x = x;
        pos.y = y;
        return exec(parent, pos);
    }
    bool exec(QWidget* parent, std::wstring filePath, POINT pos) {
		setParent(parent);
		setFilesArray({ filePath });
		return exec(pos);
	}
    bool exec(HWND hwnd, std::wstring filePath, POINT pos) {
        setParent(hwnd);
		setFilesArray({ filePath });
        return exec(pos);
    }
    void setParent(HWND hwnd) {
		this->hwnd = hwnd;
	}
    void setParent(QWidget* parent) {
        this->hwnd = (HWND)parent->winId();
    }
	void setFilesArray(std::vector<std::wstring> filesArray) {
        this->files = filesArray;
	}

    void show();

	// 设置菜单项的回调函数
	void setHandleRenameFunction(std::function<void()> handleRenameFunc) {
		this->handleRenameFunction = handleRenameFunc;
	}
	void setHandleDeleteFunction(std::function<void()> handleDeleteFunc) {
		this->handleDeleteFunction = handleDeleteFunc;
	}
	void setHandleCopyFunction(std::function<void()> handleCopyFunc) {
		this->handleCopyFunction = handleCopyFunc;
	}
	void setHandleCutFunction(std::function<void()> handleCutFunc) {
		this->handleCutFunction = handleCutFunc;
	}
	void setHandlePasteFunction(std::function<void()> handlePasteFunc) {
		this->handlePasteFunction = handlePasteFunc;
	}

private:
    // 禁止拷贝
    FileContextMenu(const FileContextMenu&) = delete;
    FileContextMenu& operator=(const FileContextMenu&) = delete;
    //处理高级/动态加载的菜单
    IContextMenu2* g_pCurrentContextMenu2 = NULL;
    IContextMenu3* g_pCurrentContextMenu3 = NULL;
	class MenuMessageWindow {
	public:
		MenuMessageWindow(IContextMenu2*& iContextMenu2, IContextMenu3*& iContextMenu3);
		~MenuMessageWindow();
		bool Create();
		void Destroy();
		HWND hwnd() const {
			return m_hWnd;
		}
		HWND operator=(const MenuMessageWindow& msgWindow) {
			return msgWindow.hwnd();
		}
		operator HWND() const {
			return hwnd();
		}

	private:
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		HWND m_hWnd = NULL;
		IContextMenu2*& iContextMenu2;
		IContextMenu3*& iContextMenu3;
	} msgWindow = MenuMessageWindow(g_pCurrentContextMenu2, g_pCurrentContextMenu3);

    [[deprecated]]bool msgHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg)
		{
		case WM_INITMENUPOPUP:
		case WM_DRAWITEM:
		case WM_MEASUREITEM:
		case WM_MENUSELECT: // WM_MENUCHAR 也可以考虑
		{
			LRESULT lres = 0; // 用于 HandleMenuMsg2 的结果
			BOOL bHandled = FALSE;
			if (g_pCurrentContextMenu3) {
				if (SUCCEEDED(g_pCurrentContextMenu3->HandleMenuMsg2(uMsg, wParam, lParam, &lres))) {
					bHandled = TRUE;
				}
			}
			else if (g_pCurrentContextMenu2) {
				if (SUCCEEDED(g_pCurrentContextMenu2->HandleMenuMsg(uMsg, wParam, lParam))) {
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
	}
};

#endif // !FILECONTEXTMENU_H
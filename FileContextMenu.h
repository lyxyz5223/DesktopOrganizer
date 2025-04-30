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
    std::wstring filePath;
public:
    FileContextMenu();
    ~FileContextMenu();
    bool exec(POINT pos);
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
		setFilePath(filePath);
		return exec(pos);
	}
    bool exec(HWND hwnd, std::wstring filePath, POINT pos) {
        setParent(hwnd);
        setFilePath(filePath);
        return exec(pos);
    }
    void setParent(HWND hwnd) {
		this->hwnd = hwnd;
	}
    void setParent(QWidget* parent) {
        this->hwnd = (HWND)parent->winId();
    }
	void setFilePath(std::wstring filePath) {
        this->filePath = filePath;
	}
    void setFilePath(const wchar_t* filePath) {
		this->filePath = filePath;
	}
    void show();
private:
    // 禁止拷贝
    FileContextMenu(const FileContextMenu&) = delete;
    FileContextMenu& operator=(const FileContextMenu&) = delete;
};

#endif // !FILECONTEXTMENU_H
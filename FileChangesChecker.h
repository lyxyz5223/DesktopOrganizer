#pragma once
#include <string>
#include <thread>
#include <iostream>
#include <Windows.h>
#include <functional>

class FileChangesChecker
{
public:
	enum Action {
		Added,
		Removed,
		Modified,
		RenamedOldName,
		RenamedNewName,
		Unknown
	};
	struct FileChanges {
		std::wstring oldName;
		std::wstring newName;
		std::wstring filePath;
		Action action = Action::Unknown;
	};
	struct OVERLAPPED_CONTEXT : OVERLAPPED {
		HANDLE hDirectory = 0;
		DWORD bytesReturned = 0;
		WCHAR buffer[1024];
		int bufferSize = 1024;
		FILE_NOTIFY_INFORMATION* fileInformation = (FILE_NOTIFY_INFORMATION*)buffer;
	};

private:
	std::wstring path;
	void IoCompletionRoutine(DWORD BytesTransferred, OVERLAPPED* Overlapped, DWORD InFlags);
	HANDLE hDirectory = 0;
	//过滤器，此处为监测文件名和目录名的变化
	DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
	//I/O完成端口
	HANDLE iocp = 0;
	//工作线程
	std::thread workerThread;
	//上一个修改内容
	OVERLAPPED_CONTEXT* latestContext = nullptr;
	FileChanges fileChanges;
	void* callbackFunctionParameter = nullptr;
	bool executeWhileRenameOldName = false;
public:
	//析构函数
	~FileChangesChecker() {
		stop();
	}
	//构造函数
	FileChangesChecker() {}
	FileChangesChecker(std::wstring path);
	//设置监测路径
	void setPath(std::wstring path) {
		this->path = path;
	}
	void setExecuteWhileRenameOldName(bool judge) {
		executeWhileRenameOldName = judge;
	}
	//获取监测路径
	std::wstring getPath() const {
		return path;
	}
	void start();
	void stop();
	const OVERLAPPED_CONTEXT* getChangesContext() const {
		return latestContext;
	}
	FileChanges getLatestFileChanges() const {
		return fileChanges;
	}
	//设置回调函数
	void setCallback(std::function<void(FileChangesChecker* checker, FileChanges fileChanges, void* parameter)> func, void* parameter/*用户自定义参数*/) {
		callbackFunction = func;
		callbackFunctionParameter = parameter;
	}
	//Callback函数
	std::function<void(FileChangesChecker* checker, FileChanges fileChanges, void* parameter)> callbackFunction;
protected:
	//工作线程函数
	void _WorkerThreadProc(HANDLE ioCompletionPort);
};


#include "FileChangesChecker.h"
#include "StringProcess.h"

FileChangesChecker::FileChangesChecker(std::wstring path)
{
	this->path = path;
}

void FileChangesChecker::start()
{
	// 创建 I/O 完成端口
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp == NULL)
	{
		std::cerr << "Failed to create I/O completion port." << std::endl;
		MessageBox(0, L"Failed to create I/O completion port.\n检测文件更改失败，软件可能无法正常运行", L"错误", MB_ICONERROR);
		return;
	}
	// 打开目录句柄
	hDirectory = CreateFile(
		path.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		NULL
	);
	if (hDirectory == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Failed to open directory." << std::endl;
		CloseHandle(iocp);
		MessageBox(0, L"Failed to open directory.\n检测文件更改失败，软件可能无法正常运行", L"错误", MB_ICONERROR);
		return;
	}
	// 将目录句柄关联到 I/O 完成端口
	CreateIoCompletionPort(hDirectory, iocp, 0, 0);
	// 初始化 I/O 上下文
	OVERLAPPED_CONTEXT* context = new OVERLAPPED_CONTEXT();
	latestContext = context;

	context->hDirectory = hDirectory;
	// 发起第一个 ReadDirectoryChangesW 请求
	if (!ReadDirectoryChangesW(
		hDirectory,
		context->buffer,
		context->bufferSize,
		FALSE, // 监视子目录
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
		&context->bytesReturned,
		(OVERLAPPED*)context,
		NULL /*不需要完成例程，由 I/O 完成端口管理*/))
	{
		std::cerr << "Error in ReadDirectoryChangesW: " << GetLastError() << std::endl;
		MessageBox(0, (L"Error in ReadDirectoryChangesW: " + std::to_wstring(GetLastError()) + L"\n检测文件更改失败，软件可能无法正常运行").c_str(), L"错误", MB_ICONERROR);
		return;
	}

	// 创建工作线程来处理 I/O 完成通知
	if (!workerThread.joinable())
	{
		workerThread = std::thread(&FileChangesChecker::_WorkerThreadProc, this, iocp);
		workerThread.detach();
	}
}

void FileChangesChecker::stop()
{
	PostQueuedCompletionStatus(iocp, 0, 0, NULL);
	std::thread waitThread;//等待线程结束
	waitThread.swap(workerThread);
	if (waitThread.joinable())
		waitThread.join();

	// 关闭目录句柄
	CloseHandle(hDirectory);
	hDirectory = 0;
	// 关闭 I/O 完成端口
	CloseHandle(iocp);
	iocp = 0;
	// 释放 I/O 上下文
	delete latestContext;
	latestContext = 0;
}

void FileChangesChecker::_WorkerThreadProc(HANDLE ioCompletionPort)
{
	while (true)
	{
		ULONG_PTR completionKey;
		DWORD bytesTransferred;
		OVERLAPPED* overlapped;
		BOOL result = GetQueuedCompletionStatus(
			ioCompletionPort,
			&bytesTransferred,
			&completionKey,
			&overlapped,
			INFINITE
		);
		if (result && !overlapped && !completionKey && !bytesTransferred) {
			// 特殊完成包被接收，退出循环
			break;
		}
		if (result && overlapped)
		{
			// 调用完成例程
			IoCompletionRoutine(bytesTransferred, overlapped, 0);
		}
		else
		{
			// 错误处理
			std::cerr << "Error in GetQueuedCompletionStatus: " << GetLastError() << std::endl;
			MessageBox(0, (L"Error in GetQueuedCompletionStatus: " + std::to_wstring(GetLastError()) + L"\n检测文件更改失败，软件可能无法正常运行").c_str(), L"错误", MB_ICONERROR);
			break;
		}
	}
}
// I/O 完成例程
void FileChangesChecker::IoCompletionRoutine(DWORD BytesTransferred, OVERLAPPED* Overlapped, DWORD InFlags)
{
	OVERLAPPED_CONTEXT* context = (OVERLAPPED_CONTEXT*)Overlapped;
	if (BytesTransferred > 0)
	{
		// 处理目录变化
		std::cout << "Directory change detected, bytes transferred: " << BytesTransferred << std::endl;
		FILE_NOTIFY_INFORMATION* pNotify = context->fileInformation;
		unsigned long long count = 0;
		while (true)
		{
			if (pNotify->FileNameLength > 0)
			{
				count++;
				std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
				//std::cout << count << " Action: " << pNotify->Action << ", FileName: " << wstr2str_2ANSI(fileName) << std::endl;
				switch (pNotify->Action)
				{
				case FILE_ACTION_ADDED:
				{
					fileChanges.action = Action::Added; // 文件添加
					fileChanges.oldName = fileChanges.newName = fileName;
					std::cout << "File added: " << wstr2str_2ANSI(fileName) << std::endl;
					std::thread(callbackFunction, this, fileChanges, callbackFunctionParameter).detach();
					break;
				}
				case FILE_ACTION_REMOVED:
				{
					fileChanges.action = Action::Removed; // 文件删除
					fileChanges.oldName = fileChanges.newName = fileName;
					std::cout << "File removed: " << wstr2str_2ANSI(fileName) << std::endl;
					std::thread(callbackFunction, this, fileChanges, callbackFunctionParameter).detach();
					break;
				}
				case FILE_ACTION_MODIFIED:
				{
					fileChanges.action = Action::Modified; // 文件修改
					fileChanges.oldName = fileChanges.newName = fileName;
					std::cout << "File modified: " << wstr2str_2ANSI(fileName) << std::endl;
					std::thread(callbackFunction, this, fileChanges, callbackFunctionParameter).detach();
					break;
				}
				case FILE_ACTION_RENAMED_OLD_NAME:
				{
					fileChanges.action = Action::RenamedOldName; // 文件重命名
					fileChanges.oldName = fileName;
					std::cout << "File renamed (old name): " << wstr2str_2ANSI(fileName) << std::endl;
					//此时不需要调用回调函数，因为需要先获取新文件名
					//此处的while循环是为了获取新文件名
					//可以执行一下回调
					if (executeWhileRenameOldName)
						std::thread(callbackFunction, this, fileChanges, callbackFunctionParameter).detach();
					break;
				}
				case FILE_ACTION_RENAMED_NEW_NAME:
				{
					fileChanges.action = Action::RenamedNewName; // 文件重命名
					fileChanges.newName = fileName;
					std::cout << "File renamed (new name): " << wstr2str_2ANSI(fileName) << std::endl;
					std::thread(callbackFunction, this, fileChanges, callbackFunctionParameter).detach();
					break;
				}
				default:
				{
					fileChanges.action = Action::Unknown; // 未知操作
					fileChanges.newName = fileName;
					std::cout << "Unknown action: " << pNotify->Action << std::endl;
					std::thread(callbackFunction, this, fileChanges, callbackFunctionParameter).detach();
					break;
				}
				}
			}
			if (pNotify->NextEntryOffset)
			{
				// 新文件
				pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);
			}
			else
				break;
		}
	}
	// 重新发起 ReadDirectoryChangesW 请求以继续监控
	if (!ReadDirectoryChangesW(
		context->hDirectory,
		context->buffer,
		context->bufferSize,
		FALSE, // 监视子目录
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
		&context->bytesReturned,
		Overlapped,
		NULL /*不需要完成例程，由 I/O 完成端口管理*/))
	{
		std::cerr << "Error in ReadDirectoryChangesW: " << GetLastError() << std::endl;
		MessageBox(0, (L"Error in ReadDirectoryChangesW: " + std::to_wstring(GetLastError()) + L"\n检测文件更改失败，软件可能无法正常运行").c_str(), L"错误", MB_ICONERROR);
	}

}


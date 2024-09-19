#include <string>
//Windows
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <strsafe.h>
#pragma comment(lib,"Shlwapi.lib")
#include "fileProc.h"
//获取文件数目
intptr_t GetFileNum(std::wstring path, bool isCountDirectory)
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
unsigned long GetFilesArray(_In_ std::wstring path, _Out_ std::wstring**& filearray, _In_ intptr_t fileNum)//返回错误代码
{
    using namespace std;
    if (fileNum == 0)
        fileNum = GetFileNum(path, true);
    if (fileNum <= 0)
    {
        MessageBox(0, L"错误，文件数目统计失败！程序即将退出", 0, MB_ICONERROR);
        exit(1);
    }
    filearray = new wstring * [fileNum + 1];//用的时候指的是前面的中括号filearray[1][2];前面那个中括号里面的索引！
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

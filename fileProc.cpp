#include <string>
//Windows
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <strsafe.h>
#pragma comment(lib,"Shlwapi.lib")
#include "fileProc.h"
#include <vector>
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
        //DisplayErrorBox(TEXT("FindFirstFile"));
        //return dwError;
        return -1;
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
        //DisplayErrorBox(TEXT("FindFirstFile"));
        return -1;
    }
    ZeroMemory(&ffd, sizeof(ffd));

    FindClose(hFind);
    //return dwError;
    return fileNum;
}
//微软官方示例，展示错误
//void DisplayErrorBox(const wchar_t* lpszFunction1)
//{
//    // Retrieve the system error message for the last-error code
//    LPTSTR lpszFunction = (LPTSTR)lpszFunction1;
//    LPVOID lpMsgBuf;
//    LPVOID lpDisplayBuf;
//    DWORD dw = GetLastError();
//
//    FormatMessage(
//        FORMAT_MESSAGE_ALLOCATE_BUFFER |
//        FORMAT_MESSAGE_FROM_SYSTEM |
//        FORMAT_MESSAGE_IGNORE_INSERTS,
//        NULL,
//        dw,
//        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//        (LPTSTR)&lpMsgBuf,
//        0, NULL);
//
//    // Display the error message and clean up
//
//    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
//        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
//    StringCchPrintf((LPTSTR)lpDisplayBuf,
//        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
//        TEXT("%s failed with error %d: %s"),
//        lpszFunction, dw, lpMsgBuf);
//    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
//
//    LocalFree(lpMsgBuf);
//    LocalFree(lpDisplayBuf);
//}

//unsigned long GetFilesArray(std::wstring path, std::wstring**& filearray,intptr_t fileNum)//返回错误代码
//{
//    using namespace std;
//    if (fileNum == 0)
//        fileNum = GetFileNum(path, true);
//    if (fileNum < 0)
//    {
//        MessageBox(0, L"错误，文件数目统计失败！程序即将退出", 0, MB_ICONERROR);
//        exit(1);
//    }
//    if (fileNum == 0)
//        return 0;
//    filearray = new wstring * [fileNum];//用的时候指的是前面的中括号filearray[1][2];前面那个中括号里面的索引！
//    for (int i = 0; i < fileNum; i++)
//    {
//        filearray[i] = new wstring[2];//后面的中括号储存判断是文件还是文件夹
//    }
//    WCHAR* cFilePath;
//    cFilePath = (WCHAR*)path.c_str();
//
//    intptr_t FileIndex = 0;
//    WIN32_FIND_DATA ffd;
//    LARGE_INTEGER filesize;
//    //TCHAR szDir[MAX_PATH];
//    HANDLE hFind = INVALID_HANDLE_VALUE;
//    DWORD dwError = 0;
//    if (path.back() != L'\\')
//        path += L"\\";
//    path += L"*";
//    // Find the first file in the directory.
//    hFind = FindFirstFile(path.c_str(), &ffd);
//
//    if (INVALID_HANDLE_VALUE == hFind)
//    {
//        //DisplayErrorBox(TEXT("FindFirstFile"));
//        return dwError;
//    }
//    // List all the files in the directory with some info about them.
//    do
//    {
//        if (wcscmp(ffd.cFileName, L".") != 0
//            && wcscmp(ffd.cFileName, L"..") != 0)
//        {
//            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
//            {
//                //wprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
//                //这是一个文件夹，名称为：ffd.cFileName
//                filearray[FileIndex][0] = ffd.cFileName;
//                filearray[FileIndex][1] = L"Directory";
//            }
//            else
//            {
//                filesize.LowPart = ffd.nFileSizeLow;
//                filesize.HighPart = ffd.nFileSizeHigh;
//                //wprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
//                //这是一个文件，名称为：ffd.cFileName，大小为filesize.QuadPart
//                filearray[FileIndex][0] = ffd.cFileName;
//                filearray[FileIndex][1] = L"File";
//            }
//            FileIndex++;
//        }
//    } while (FindNextFile(hFind, &ffd) != 0);
//
//    dwError = GetLastError();
//    if (dwError != ERROR_NO_MORE_FILES)
//    {
//        //DisplayErrorBox(TEXT("FindFirstFile"));
//    }
//    FindClose(hFind);
//    return dwError;
//}

// parameter:
// path: 文件夹路径
// filesVector: 包含一系列文件的信息，每个文件元素都是一个vector<wstring>，包含[0]:名称，[1]:类型（文件夹还是文件)
// return: errorCode错误代码
long long GetFilesArray(std::wstring path, std::vector<std::vector<std::wstring>> &filesVector)//返回错误代码
{
    using namespace std;
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
        //DisplayErrorBox(TEXT("FindFirstFile"));
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
                //wprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
                //这是一个文件夹，名称为：ffd.cFileName
                vector<wstring> tmpwstringarray(2);
                tmpwstringarray[0] = ffd.cFileName;
                tmpwstringarray[1] = L"Directory";
                filesVector.push_back(tmpwstringarray);
            }
            else
            {
                filesize.LowPart = ffd.nFileSizeLow;
                filesize.HighPart = ffd.nFileSizeHigh;
                //wprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
                //这是一个文件，名称为：ffd.cFileName，大小为filesize.QuadPart
                vector<wstring> tmpwstringarray(2);
                tmpwstringarray[0] = ffd.cFileName;
                tmpwstringarray[1] = L"File";
                filesVector.push_back(tmpwstringarray);
            }
            FileIndex++;
        }
    } while (FindNextFile(hFind, &ffd) != 0);
    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
    {
        //DisplayErrorBox(TEXT("FindFirstFile"));
    }
    FindClose(hFind);
    return filesVector.size();
}

std::vector<std::wstring> GetFilesArrayForMultiFilePath(std::vector<std::wstring> pathVector)//标记不同位置的文件及文件夹使用空的元素，空元素的下一个元素即为文件夹位置
{
    using namespace std;
    vector<wstring> resultVector;
    if (pathVector.empty())//如果路径为空，则使用默认桌面路径，用户桌面和公用桌面
    {
        WCHAR cFilePath[MAX_PATH];
        SHGetFolderPath(NULL, CSIDL_DESKTOP, 0, 0, cFilePath);
        //MessageBox(HWND_thisApp, cFilePath, L"", 0);
        pathVector.push_back(cFilePath);
        SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, 0, 0, cFilePath);
        pathVector.push_back(cFilePath);
        
    }
    for (wstring path : pathVector)
    {
        if (path.back() == L'\\' || path.back() == L'/')
            path.pop_back();
        resultVector.push_back(L"");
        resultVector.push_back(path);//文件夹路径末尾不包含斜杠或者反斜杠
        //判断文件夹是否存在，不存在自动创建
        if (PathFileExists(path.c_str()) == TRUE)
        {
            //exist
        }
        else
            //do not exist
            if (CreateDirectory(path.c_str(), 0) == FALSE)
            {
                MessageBox(0, L"文件夹不存在且创建失败，请检查你是否有权限，或者尝试以管理员身份运行。程序将退出。", 0, MB_ICONERROR);
                exit(2);
            }
        if (path.back() != L'\\' && path.back() != L'/')
            path += L"\\";
        std::vector<std::vector<std::wstring>> filesList;
        GetFilesArray(path, filesList);
        for (intptr_t i = 0; i < filesList.size(); i++)
        {
            wstring filesListElement = filesList[i][0];
            resultVector.push_back(filesListElement);
        }
        filesList.clear();
        filesList.shrink_to_fit();
    }
    pathVector.clear();
    pathVector.shrink_to_fit();
    return resultVector;
}
std::vector<intptr_t> GetDirectoryFromFilesVector(std::vector<std::wstring> filesVector)
{
    //返回的路径末尾不带有斜杠或者反斜杠，此函数仅对于GetFilesArrayForMultiFilePath函数返回的vector有效
    using namespace std;
    vector<intptr_t> resultVec;
    bool mask = false;
    for (size_t i = 0; i < filesVector.size(); i++)
    {
        if (mask)
        {
            resultVec.push_back(i);
            mask = false;
        }
        if (filesVector[i] == L"")
        {
            mask = true;
        }
    }
    return resultVec;
}
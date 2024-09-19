#pragma once
#include <string>
//获取文件数目
intptr_t GetFileNum(std::wstring path, bool isCountDirectory);
unsigned long GetFilesArray(_In_ std::wstring path, _Out_ std::wstring**& filearray, _In_ intptr_t fileNum = 0);//返回错误代码
void DisplayErrorBox(const wchar_t* lpszFunction);

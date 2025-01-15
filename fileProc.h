#pragma once
#include <string>
#include <vector>
//获取文件数目
intptr_t GetFileNum(std::wstring path, bool isCountDirectory);
//unsigned long GetFilesArray(std::wstring path, std::wstring**& filearray, intptr_t fileNum = 0);//返回错误代码
long long GetFilesArray(std::wstring path, std::vector<std::vector<std::wstring>> &filesarray);//返回文件个数
//void DisplayErrorBox(const wchar_t* lpszFunction);
std::vector<std::wstring> GetFilesArrayForMultiFilePath(std::vector<std::wstring> pathVector = std::vector<std::wstring>());
std::vector<intptr_t> GetDirectoryFromFilesVector(std::vector<std::wstring> filesVector);

#pragma once
#include <string>
#include <vector>
//请勿使EscapeString参数与delimiter值相等，否则处理将出现异常！！(2025-2-24待修复)
std::vector<std::string> split(std::string text, std::vector<std::string> delimiter/*separator,分隔符*/, std::string EscapeString = "" /*char EscapeCharacter*/);
std::vector<std::string> split(std::string text, std::string delimiter = " "/*separator,分隔符*/, std::string EscapeString = "" /*char EscapeCharacter*/);
std::vector<std::wstring> split(std::wstring text, std::vector<std::wstring> delimiter/*separator,分隔符*/, std::wstring EscapeString = L"" /*char EscapeCharacter*/);
std::vector<std::wstring> split(std::wstring text, std::wstring delimiter = L" "/*separator,分隔符*/, std::wstring EscapeString = L"" /*char EscapeCharacter*/);

std::string join(std::vector<std::string> textVec, std::string delimiter);
std::wstring join(std::vector<std::wstring> textVec, std::wstring delimiter);

std::string wstr2str_2UTF8(std::wstring text);
std::string wstr2str_2ANSI(std::wstring text);
std::wstring str2wstr_2UTF8(std::string text);
std::wstring str2wstr_2ANSI(std::string text);
std::string UTF8ToANSI(std::string utf8Text);
std::string ANSIToUTF8(std::string ansiText);
std::wstring ANSIToUTF8(std::wstring ansiText);
std::wstring UTF8ToANSI(std::wstring utf8Text);

std::wstring boolToWString(bool Bool);
std::string boolToString(bool Bool);
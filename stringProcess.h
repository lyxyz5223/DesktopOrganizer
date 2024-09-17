#pragma once
#include <string>
#include <vector>
std::vector<std::string> split(std::string text, std::vector<std::string> delimiter/*separator,分隔符*/, std::string EscapeString = "" /*char EscapeCharacter*/);
std::vector<std::string> split(std::string text, std::string delimiter = " "/*separator,分隔符*/, std::string EscapeString = "" /*char EscapeCharacter*/);
std::string join(std::vector<std::string> textVec, std::string delimiter);

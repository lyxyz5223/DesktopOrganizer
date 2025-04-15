#pragma once
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H
#include <iostream>
#include <fstream>
#include <string>

class ConfigManager
{
private:
	std::wstring pathSeparator = L"\\";
	std::wstring path;
	std::wstring fileName;
	std::fstream fileStream;

public:
	~ConfigManager();
	ConfigManager();
	ConfigManager(std::wstring path);
	ConfigManager(std::wstring path, std::wstring fileName);

	void setPath(std::wstring path) {
		this->path = path;
	}
	std::wstring getPath() const {
		return path;
	}
	
	void setFileName(std::wstring fileName) {
		this->fileName = fileName;
	}

	std::wstring getFileName() const {
		return fileName;
	}

	void setPathAndFileName(std::wstring path, std::wstring fileName) {
		this->path = path;
		this->fileName = fileName;
	}

	std::wstring getFileNameWithPath() const {
		return path + fileName;
	}

	void setPathAndFileName(std::wstring pathAndFileName);

	std::wstring getPathSeparator() const {
		return pathSeparator;
	}

	bool open();
	void close();

	void writeString(const std::string& data);
	void writeWString(const std::wstring data);

	std::wstring readWString();
	std::string readString();
};

class ConfigFactory {
public:
	ConfigManager* createConfigManager() {

	}
};

#endif //!CONFIGMANAGER_H
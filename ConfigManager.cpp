#include "ConfigManager.h"
#include <codecvt>
#include "stringProcess.h"



ConfigManager::~ConfigManager()
{
	// Destructor implementation (if needed)
}

ConfigManager::ConfigManager()
{

}

ConfigManager::ConfigManager(std::wstring path, std::wstring fileName)
{
	this->path = path;
	this->fileName = fileName;
}

void ConfigManager::setPathAndFileName(std::wstring pathAndFileName)
{
	size_t pos = pathAndFileName.find_last_of(L"/\\");
	if (pos != std::wstring::npos) {
		this->path = pathAndFileName.substr(0, pos + 1);
		this->fileName = pathAndFileName.substr(pos + 1);
	}
	else {
		this->path = L"";
		this->fileName = pathAndFileName;
	}
}
bool ConfigManager::open()
{
	fileStream.open(getFileNameWithPath(), std::ios::in | std::ios::out | std::ios::app);
	if (fileStream.is_open())
		return true;
	else {
		std::cerr << "ConfigManager: Failed to open file: " << wstr2str_2ANSI(getFileNameWithPath()) << std::endl;
		return false;
	}
}

void ConfigManager::close()
{
	if (fileStream.is_open()) {
		fileStream.close();
	}
}
void ConfigManager::writeString(const std::string& data)
{
	if (fileStream.is_open()) {
		fileStream << data;
		fileStream.flush();
	}
	else {
		std::cerr << "ConfigManager: File is not open." << std::endl;
	}
}
void ConfigManager::writeWString(const std::wstring data)
{
	if (fileStream.is_open()) {
		std::string(*encodeType)(std::wstring) = wstr2str_2UTF8;
		fileStream << encodeType(data);
		fileStream.flush();
	}
	else {
		std::cerr << "ConfigManager: File is not open." << std::endl;
	}
}

std::wstring ConfigManager::readWString()
{
	std::wstring wdata;
	if (fileStream.is_open())
	{
		if (fileStream.peek() != std::ifstream::traits_type::eof())
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			std::string lineStr;
			while (std::getline(fileStream, lineStr))
			{
				std::wstring lineWStr = converter.from_bytes(lineStr);
				wdata += lineWStr + L'\n';
			}
			if (!wdata.empty())
				wdata.pop_back();
		}
	}
	else {
		std::cerr << "ConfigManager: File is not open." << std::endl;
	}
	return wdata;
}

std::string ConfigManager::readString()
{
	std::string data;
	if (fileStream.is_open())
	{
		if (fileStream.peek() != std::ifstream::traits_type::eof())  
		{
			std::string lineStr;
			while (std::getline(fileStream, lineStr))
				data += lineStr + '\n';
			if (!data.empty())
				data.pop_back();
		}
	}
	else {
		std::cerr << "ConfigManager: File is not open." << std::endl;
	}
	return data;
}
ConfigManager::ConfigManager(std::wstring path)
{
	this->path = path;
	this->fileName = L"";
}
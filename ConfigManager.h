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

//class ConfigFactory {
//public:
//	ConfigManager* createConfigManager() {
//
//	}
//};
#include "stringProcess.h"
#include "FunctionWrapper.h"
#include "./lib/SQLite/sqlite3.h"

#include <tuple>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>

class AbstractDatabaseConfigManager abstract {

public:
	typedef std::function<void(std::vector<std::any>, void*)> CallBackFunction;
	typedef std::function<void(void*)> CallBackFunctionOnlyUserData;

	class SQLite3DataType {
	public:
		enum Type {
			INTEGER,/*int*/
			BIGINT,/*long long*/
			TEXT/*TEXT: read时生成的std::any中为std::string类型*/
		};
		SQLite3DataType(Type type) {
			this->type = type;
		}
		std::wstring toWString() const {
			switch (type)
			{
			case SQLite3DataType::INTEGER:
				return L"INTEGER";
			case SQLite3DataType::BIGINT:
				return L"BIGINT";
			case SQLite3DataType::TEXT:
				return L"TEXT";
			default:
				return L"";
			}
		}
		std::string toString() {
			return wstr2str_2UTF8(toWString());
		}

		operator std::string() {
			return toString();
		}
		operator std::wstring() {
			return toWString();
		}
		constexpr operator Type() const {
			return type;
		}
	private:
		Type type;
	};

	struct DatabaseColumn {
		std::string name;
		SQLite3DataType type;
	};
	struct TableStruct {
		std::vector<DatabaseColumn> columns;
		std::vector<unsigned long long> primaryKeyIndex;
		bool dropRowId = true;//是否去除隐藏的rowId列
	};
	class Transaction {
	public:
		enum State {
			Begin,
			End
		};
	private:
		sqlite3*& db;
		State state = State::End;
		void executeSqlWithCheck(sqlite3* db, const char* sql) const;
	public:
		Transaction(sqlite3*& db) : db(db) {
			//this->db = db;
		}
		//void setDatabase(sqlite3* db) {
		//	this->db = db;
		//}
		void begin();
		void commit();
		void rollback();
		const State getState() const {
			return state;
		}
	};
private:
	//写入
	class WriteCommand {
	public:
		enum Command {
			INSERT,
			REPLACE,
			INSERTORREPLACE,
			UPSERT = INSERTORREPLACE,
			REMOVE
		};
		WriteCommand(Command cmd) {
			this->cmd = cmd;
		}
		std::wstring toWString() const {
			switch (cmd)
			{
			case WriteCommand::INSERT:
				return L"INSERT";
			case WriteCommand::REPLACE:
				return L"REPLACE";
			case WriteCommand::INSERTORREPLACE:
				return L"INSERT OR REPLACE";
				//case WriteCommand::UPSERT:
				//	break;
			case WriteCommand::REMOVE:
				return L"REMOVE";
			default:
				return L"";
			}
		}
		std::string toString() {
			return wstr2str_2UTF8(toWString());
		}

		operator std::string() {
			return toString();
		}
		operator std::wstring() {
			return toWString();
		}
		constexpr operator Command() const {
			return cmd;
		}
	private:
		Command cmd;
	};
	virtual int addLineExecute(std::wstring tableName, TableStruct tableStruct, WriteCommand command, std::vector<std::any> data);
	virtual int addLinesExecute(WriteCommand command, std::vector<std::any> data);
	int bind(sqlite3_stmt*& stmt, SQLite3DataType type, int column, std::any data);
	int prepare(sqlite3_stmt*& stmt, std::string prepareText);

private:
	//属性定义区
	std::function<void()> cbar;
	std::function<void(std::vector<std::any>)> cbwr;
	std::function<void()> cbbr;
	//std::function<void()> cbaw;
	//std::function<void(std::vector<std::any>)> cbww;
	//std::function<void()> cbbw;
	sqlite3* db = nullptr;//数据库
	sqlite3_stmt* statement = nullptr;//statement
	WriteCommand stmtOperation = WriteCommand::INSERTORREPLACE;
	Transaction transaction = Transaction(db);
	std::wstring tableName;
	TableStruct tableStruct;

public:
	void initialize();
	~AbstractDatabaseConfigManager();
	AbstractDatabaseConfigManager();
	AbstractDatabaseConfigManager(std::wstring dbFile);
	AbstractDatabaseConfigManager(std::string dbFile);
	[[deprecated]]virtual const Transaction& getTransaction() const {
		return transaction;
	}
	virtual int open(std::wstring dbFile) {
		using namespace std;
		string file = wstr2str_2UTF8(dbFile);
		if (db)
			sqlite3_close(db);
		db = nullptr;
		int code = sqlite3_open(file.c_str(), &db);
		if (code != SQLITE_OK)
		{
			std::cerr << UTF8ToANSI("无法打开数据库: ") << sqlite3_errmsg(db) << std::endl;
			sqlite3_free(db);
			return code;
		}
	}
	virtual int open(std::string dbFile) {
		return open(str2wstr_2UTF8(dbFile));
	}
	virtual void close() {
		sqlite3_close(db);
	}
	virtual int createTable(std::wstring tableName, TableStruct tableStruct, bool bCoverOldTable = false, bool bSetTransactionInformation = false);
	virtual int removeTable(std::wstring tableName);
	virtual void setTableNameAndStruct(std::wstring tableName, TableStruct ts) {
		this->tableName = tableName;
		this->tableStruct = ts;
	}
	virtual int read(std::wstring tableName, TableStruct tableStruct);
	virtual int read() {
		return read(tableName, tableStruct);
	}

	//读取过程中出错将不再执行该函数
	virtual void setCallbackAfterRead(const CallBackFunctionOnlyUserData& fc, void* userData) {
		cbar = std::bind(fc, userData);
	}
	virtual void setCallbackWhileReading(const CallBackFunction& fc, void* userData) {
		cbwr = std::bind(fc, std::placeholders::_1, userData);
	}
	//prepare之后，开始Read前调用该函数
	virtual void setCallbackBeforeRead(const CallBackFunctionOnlyUserData& fc, void* userData) {
		cbbr = std::bind(fc, userData);
	}

	virtual void transactionBegin();
	virtual void transactionCommit();
	virtual void transactionRollback();
	virtual int insertLine(std::wstring tableName, TableStruct tableStruct, std::vector<std::any> data) {
		return addLineExecute(tableName, tableStruct, WriteCommand::INSERT, data);
	}
	virtual int replaceLine(std::wstring tableName, TableStruct tableStruct, std::vector<std::any> data) {
		return addLineExecute(tableName, tableStruct, WriteCommand::REPLACE, data);
	}
	virtual int insertOrReplaceLine(std::wstring tableName, TableStruct tableStruct, std::vector<std::any> data) {
		return addLineExecute(tableName, tableStruct, WriteCommand::INSERTORREPLACE, data);
	}
	virtual int removeLine(std::wstring tableName, TableStruct tableStruct, std::vector<std::any> data);


	//多行处理（事务）
	virtual int insertLines(std::vector<std::any> data) {
		return addLinesExecute(WriteCommand::INSERT, data);
	}
	virtual int replaceLines(std::vector<std::any> data) {
		return addLinesExecute(WriteCommand::REPLACE, data);
	}
	virtual int insertOrReplaceLines(std::vector<std::any> data) {
		return addLinesExecute(WriteCommand::INSERTORREPLACE, data);
	}
	virtual int removeLines(std::vector<std::any> data);


	//virtual void setCallbackAfterWrite(const CallBackFunctionOnlyUserData& fc, void* userData) {
	//	cbaw = std::bind(fc, userData);
	//}
	//virtual void setCallbackWhileWriting(const CallBackFunction& fc, void* userData) {
	//	cbww = std::bind(fc, std::placeholders::_1, userData);
	//}
	//virtual void setCallbackBeforeWrite(const CallBackFunctionOnlyUserData& fc, void* userData) {
	//	cbbw = std::bind(fc, userData);
	//}
};
class DatabaseConfigManager : public AbstractDatabaseConfigManager {
private:
public:
	~DatabaseConfigManager() {}
	DatabaseConfigManager() : AbstractDatabaseConfigManager() {}
	DatabaseConfigManager(std::wstring dbFile)
		: AbstractDatabaseConfigManager(dbFile) {
	}
	DatabaseConfigManager(std::string dbFile)
		: AbstractDatabaseConfigManager(dbFile) {
	}
};





#endif //!CONFIGMANAGER_H




////窗口配置文件
//struct WindowsConfig {
//	long long id;
//	std::string title;
//	int x;
//	int y;
//	int width;
//	int height;
//};
//
////注册ORM元数据
//DEFINE_ORM(WindowsConfig,
//	REGISTER_MEMBER(WindowsConfig, long long, id),
//	REGISTER_MEMBER(WindowsConfig, std::string, title),
//	REGISTER_MEMBER(WindowsConfig, int, x),
//	REGISTER_MEMBER(WindowsConfig, int, y),
//	REGISTER_MEMBER(WindowsConfig, int, width),
//	REGISTER_MEMBER(WindowsConfig, int, height)
//)
//
////item配置文件
//struct ItemConfig {
//	long long id;
//	std::string name;
//	std::string path;
//	long long position;
//};
//
//DEFINE_ORM(ItemConfig,
//	REGISTER_MEMBER(ItemConfig, long long, id),
//	REGISTER_MEMBER(ItemConfig, std::string, name),
//	REGISTER_MEMBER(ItemConfig, std::string, path),
//	REGISTER_MEMBER(ItemConfig, long long, position)
//)

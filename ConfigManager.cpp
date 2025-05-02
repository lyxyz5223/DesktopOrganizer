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

int AbstractDatabaseConfigManager::addLineExecute(std::wstring tableName, TableStruct tableStruct, WriteCommand command, std::vector<std::any> data)
{
	std::string askSignText, prepareText;
	std::string cmd = command;
	prepareText = cmd + " INTO " + wstr2str_2UTF8(tableName) + " (";
	for (auto i = tableStruct.columns.begin(); i != tableStruct.columns.end(); i++)
	{
		prepareText += i->name + ',';
		askSignText += "?,";
	}
	if (tableStruct.columns.size())
	{
		prepareText.pop_back();
		askSignText.pop_back();
	}
	prepareText += ") VALUES (";
	prepareText += askSignText;
	prepareText += ")";
	//编译语句为字节码
	sqlite3_stmt* stmt = nullptr;
	int code = prepare(stmt, prepareText);
	if (code != SQLITE_OK)
		return code;
	//执行预编译语句
	//写入时bind是从1开始递增
	for (int column = 0; column < tableStruct.columns.size(); column++)
	{
		code = bind(stmt, tableStruct.columns[column].type, column + 1, data[column]);
		if (code != SQLITE_OK)
			return code;
	}
	code = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return code;
}

//添加多行时使用
int AbstractDatabaseConfigManager::addLinesExecute(WriteCommand command, std::vector<std::any> data)
{
	int code = SQLITE_OK;
	if (!statement)
	{
		std::string askSignText, prepareText;
		std::string cmd = command;
		prepareText = cmd + " INTO " + wstr2str_2UTF8(tableName) + " (";
		for (auto i = tableStruct.columns.begin(); i != tableStruct.columns.end(); i++)
		{
			prepareText += i->name + ',';
			askSignText += "?,";
		}
		if (tableStruct.columns.size())
		{
			prepareText.pop_back();
			askSignText.pop_back();
		}
		prepareText += ") VALUES (";
		prepareText += askSignText;
		prepareText += ")";
		//编译语句为字节码
		code = prepare(statement, prepareText);
		if (code != SQLITE_OK)
			return code;
	}
	//执行预编译语句
	//写入时bind是从1开始递增
	for (int column = 0; column < tableStruct.columns.size(); column++)
	{
		code = bind(statement, tableStruct.columns[column].type, column + 1, data[column]);
		if (code != SQLITE_OK)
			return code;
	}
	code = sqlite3_step(statement);
	sqlite3_reset(statement);
	return code;
}

int AbstractDatabaseConfigManager::bind(sqlite3_stmt*& stmt, SQLite3DataType type, int column, std::any data)
{
	switch (type)
	{
	case SQLite3DataType::INTEGER:
		return sqlite3_bind_int(stmt, column, std::any_cast<int>(data));
	case SQLite3DataType::BIGINT:
		return sqlite3_bind_int64(stmt, column, std::any_cast<long long>(data));
	case SQLite3DataType::TEXT:
	{
		std::string text;
		if (data.type() == typeid(std::wstring))
			text = wstr2str_2UTF8(std::any_cast<std::wstring>(data));
		else if (data.type() == typeid(std::string))
			text = std::any_cast<std::string>(data);
		else// if (data.type() == typeid(const char*))
			text = std::any_cast<const char*>(data);
		return sqlite3_bind_text(stmt, column, text.c_str(), -1/*-1表示以空字符结尾*/,
			SQLITE_TRANSIENT/*析构函数*/
		);
	}
	default:
		return -1;
	}
}

int AbstractDatabaseConfigManager::createTable(std::wstring tableName, TableStruct tableStruct, bool bCoverOldTable, bool bSetTransactionInformation)
{
	if (bSetTransactionInformation)
		setTableNameAndStruct(tableName, tableStruct);

	std::string tableStructText;
	for (auto i = tableStruct.columns.begin(); i != tableStruct.columns.end(); i++)
		tableStructText += i->name + ' ' + i->type.toString() + ',';
	if (tableStructText.size())
	{
		if (tableStruct.primaryKeyIndex.size())
		{
			tableStructText += "PRIMARY KEY (";
			for (auto i : tableStruct.primaryKeyIndex)
				tableStructText += tableStruct.columns[i].name + ',';
			if (tableStruct.primaryKeyIndex.size())
				tableStructText.pop_back();
			tableStructText += ")";
		}
		else
			tableStructText.pop_back();
	}

	char* errMsg = nullptr;
	if (bCoverOldTable)
	{
		std::ostringstream drop_oss;
		drop_oss << "DROP TABLE IF EXISTS " << wstr2str_2UTF8(tableName) << ";";
		sqlite3_exec(db, drop_oss.str().c_str(), nullptr, nullptr, &errMsg);
	}
	std::string createTableSQL =
		(bCoverOldTable ? std::string("CREATE TABLE ") : std::string("CREATE TABLE IF NOT EXISTS "))
		+ wstr2str_2UTF8(tableName)
		+ " ("
		+ tableStructText
		+ ")"
		+ (tableStruct.dropRowId ? " WITHOUT ROWID" : "")
		+ ";";
	int rc = sqlite3_exec(db, createTableSQL.c_str(), nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK)
	{
		std::cerr << UTF8ToANSI("创建表失败: ") << errMsg << std::endl;
		sqlite3_free(errMsg);
		return rc;
	}
}

int AbstractDatabaseConfigManager::removeTable(std::wstring tableName)
{
	std::wstring sql = L"DROP TABLE IF EXISTS " + tableName;
	sqlite3_stmt* stmt = nullptr;
	int res = prepare(stmt, wstr2str_2UTF8(sql));
	if (res != SQLITE_OK)
		return res;
	res = sqlite3_step(stmt);
	if (res != SQLITE_DONE) {
		std::cerr << UTF8ToANSI("删除表失败: ") << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(stmt);
		return res;
	}
	return res;
}
int AbstractDatabaseConfigManager::read(std::wstring tableName, TableStruct tableStruct)
{
	int code = SQLITE_OK;
	const auto& ts = tableStruct;
	std::string prepareText = "SELECT ";
	for (auto i = ts.columns.begin(); i != ts.columns.end(); i++)
		prepareText += i->name + ',';
	if (ts.columns.size())
		prepareText.pop_back();
	prepareText += " FROM " + wstr2str_2UTF8(tableName);
	sqlite3_stmt* stmt;
	code = prepare(stmt, prepareText);
	if (code != SQLITE_OK)
		return code;
	if (cbbr)
		cbbr();
	while ((code = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		std::vector<std::any> callbackData;
		long long cols = 0;
		for (auto i = ts.columns.begin(); i != ts.columns.end(); i++, cols++)
		{
			switch (i->type)
			{
			case SQLite3DataType::TEXT:
			{
				std::string text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, cols));
				callbackData.push_back(text);
				break;
			}
			case SQLite3DataType::BIGINT:
			{
				long long linte = sqlite3_column_int64(stmt, cols);
				callbackData.push_back(linte);
				break;
			}
			case SQLite3DataType::INTEGER:
			{
				int inte = sqlite3_column_int(stmt, cols);
				callbackData.push_back(inte);
				break;
			}
			default:
				break;
			}
		}
		if (cbwr)
			cbwr(callbackData);
	}
	sqlite3_finalize(stmt);
	if (code != SQLITE_DONE && code != SQLITE_OK)
		return code;
	if (cbar)
		cbar();
	return code;
}

void AbstractDatabaseConfigManager::transactionBegin()
{
	transaction.begin();
}

void AbstractDatabaseConfigManager::transactionCommit()
{
	transaction.commit();
	sqlite3_finalize(statement);
	statement = nullptr;
}

void AbstractDatabaseConfigManager::transactionRollback()
{
	transaction.rollback();
	sqlite3_finalize(statement);
	statement = nullptr;
}

int AbstractDatabaseConfigManager::removeLine(std::wstring tableName, TableStruct tableStruct, std::vector<std::any> data)
{
	// 准备删除语句 - 表名为tableName，复合主键为(name, path)
	std::string deleteSQL =
		std::string("DELETE FROM ")
		+ wstr2str_2UTF8(tableName)
		+ " WHERE ";
	//遍历复合主键，添加到语句中
	if (tableStruct.primaryKeyIndex.size() && tableStruct.columns.size())
	{
		deleteSQL += tableStruct.columns[0].name + " = ?";
		for (auto i = tableStruct.primaryKeyIndex.begin() + 1; i != tableStruct.primaryKeyIndex.end(); i++)
			deleteSQL += " AND " + tableStruct.columns[*i].name + " = ?";
	}
	sqlite3_stmt* stmt  = nullptr;
	int code = prepare(stmt, deleteSQL);
	if (code != SQLITE_OK)
		return code;

	// 绑定参数值
	long long column = 1;
	for (auto i = tableStruct.primaryKeyIndex.begin();
		i != tableStruct.primaryKeyIndex.end(); i++, column++)
	{
		DatabaseColumn col = tableStruct.columns[*i];
		code = bind(stmt, col.type, column, data[column - 1]);
		if (code != SQLITE_OK)
			return code;
	}

	// 执行删除
	code = sqlite3_step(stmt);

	// 清理资源
	sqlite3_finalize(stmt);
	return code;
}

int AbstractDatabaseConfigManager::prepare(sqlite3_stmt*& stmt, std::string prepareText)
{
	int code = sqlite3_prepare_v2(db, prepareText.c_str(), -1, &stmt, nullptr);//编译SQL语句为字节码
	if (code != SQLITE_OK)
		std::cerr << UTF8ToANSI("准备语句失败: ") << sqlite3_errmsg(db) << std::endl;
	return code;
}


AbstractDatabaseConfigManager::~AbstractDatabaseConfigManager()
{
	sqlite3_close(db);
}
void AbstractDatabaseConfigManager::initialize()
{
	sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	sqlite3_initialize();
}

AbstractDatabaseConfigManager::AbstractDatabaseConfigManager()
{
	initialize();
}

AbstractDatabaseConfigManager::AbstractDatabaseConfigManager(std::wstring dbFile)
{
	initialize();
	this->open(dbFile);
}
AbstractDatabaseConfigManager::AbstractDatabaseConfigManager(std::string dbFile)
{
	initialize();
	this->open(str2wstr_2UTF8(dbFile));
}

void AbstractDatabaseConfigManager::Transaction::executeSqlWithCheck(sqlite3* db, const char* sql) const
{
	char* errMsg = nullptr;
	int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
	if (rc != SQLITE_OK)
	{
		std::string error = errMsg ? errMsg : "Unknown error";
		sqlite3_free(errMsg);
		throw std::runtime_error("SQL error: " + error);
	}
}

void AbstractDatabaseConfigManager::Transaction::begin()
{
	try {
		executeSqlWithCheck(db, "BEGIN TRANSACTION");
		state = State::Begin;
	}
	catch (std::runtime_error re) {
		throw std::runtime_error(std::string("Begin Transaction Error: ") + re.what());
	}
}
void AbstractDatabaseConfigManager::Transaction::commit()
{
	try {
		executeSqlWithCheck(db, "COMMIT");
		state = State::End;
	}
	catch (std::runtime_error re) {
		throw std::runtime_error(std::string("Commit Error: ") + re.what());
	}
}

void AbstractDatabaseConfigManager::Transaction::rollback()
{
	try {
		executeSqlWithCheck(db, "ROLLBACK");
	}
	catch (std::runtime_error re) {
		throw std::runtime_error(std::string("Rollback Error: ") + re.what());
	}
}
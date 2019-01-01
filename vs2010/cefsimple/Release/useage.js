 参数： --disable-web-security 可以开启ajax加载本地文件

Buffer createBuffer(int length);
class Buffer {
	int length();
	void* buffer([int pos]);
	void destory();
	int getByte(int pos, [bool sign]);
	int getShort(int pos, [bool sign]);
	int getInt(int pos, [bool sign]);
	void setByte(int pos, byte val);
	void setShort(int pos, short val);
	void setInt(int pos, int val);
	String toString([String charset]);
}

DBDriver openDBDriver(String url, [String userName], [String password]);
class DBDriver {
	void setAutoCommit(bool auto);
	bool commit();
	bool rollback();
	String getError();
	ResultSet executeQuery(String sql);
	int executeUpdate(String sql);
	PrepareStatement prepare(String sql);
}

class PrepareStatement {
	ResultSet executeQuery();
	int executeUpdate();
	int getParameterCount();
	void setDouble(int idx, double val);
	void setInt(int idx, int val);
	void setString(int idx, String val);
	int getInsertId();
	void close();
	ResultSetMetaData getMetaData();
}

class ResultSetMetaData {
	int getColumnCount();
	String getColumnLabel(int idx);
	String getColumnName(int idx);
	int getColumnType(int idx);
}

class ResultSet {
	int findColumn(String colName);
	getBlob(); // TODO:
	double getDouble(int idx);
	float getFloat(int idx);
	int getInt(int idx);
	int getInt64(int idx);
	String getString(int idx);
	bool next();
	void close();
	ResultSetMetaData getMetaData();
}

// i:int  p:pointer  s:Char-String(gbk)  w: Wide-String(unicode)    v:void   
what? callNative(String funcName, String funcDesc, Array funcParams);


class ZIP {
	constructor(filePath); // build  file
	// constructor(memorySize); // build  memory, not implement
	
}


	
	
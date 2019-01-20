 参数： --disable-web-security 可以开启ajax加载本地文件
        --single-process  单进程运行模型，不会加载onWebkit..Init之类的函数,不建议使用
		--inject='C:\a.js;D:\b.js' 注入javascript 多个js之间用分号分隔
		
 注：html 里必须要指明charset <meta charset="UTF-8">   默认Cef用GBK加载
 

class NBuffer {
	// create a native buffer . If address is NULL, Buffer memory auto malloc and clear zero
	static NBuffer create(void * address, int length);
	
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

class DBDriver {
	static DBDriver open(String url, [String userName], [String password]);
	
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
	boolean next();
	void close();
	ResultSetMetaData getMetaData();
}

// i:int  p:pointer  s:Char-String(gbk)  w: Wide-String(unicode)    v:void   
what? callNative(String funcName, String funcDesc, Array funcParams);


class ZIP {
	static ZIP create(String zipFilePath); // create a new zip file
	static ZIP open(String zipFilePath);   // open an exists zip file
	
	boolean close();
	boolean add(String itemName, String filePath);
	Object getItem(int idx);  // Object = {index: 2, name:'abc/tt.xml', compressSize:1200, unCompressSize:3000}
	int getItemCount();
	Object findItem(String itemName);  // Object = {index: 2, name:'abc/tt.xml', compressSize:1200, unCompressSize:3000}
	boolean unzipItem(int idx, String destFilePath);
}

class NFile {
	constructor(path);
	
	String getName();
	boolean exists();
	boolean isDirectory();
	int length();
	Buffer read();
	boolean write(String data or Buffer buf);
	String[] list([String pattern]); // pattern 匹配模式  如：*.txt
}



	

#include "DB.h"
#include "utils/XString.h"

class DBDriverV8Handler;
class RestltSetV8Handler;
class PrepareStatementV8Handler;
class RestltSetMetaDataV8Handler;

static DBDriverV8Handler *s_dbV8;
static RestltSetV8Handler *s_rsV8;
static PrepareStatementV8Handler *s_psV8;
static RestltSetMetaDataV8Handler *s_metaV8;
static SqlConnection *s_con;

CefRefPtr<CefV8Value> WrapResultSet(ResultSet *rs, bool closeStament);
CefRefPtr<CefV8Value> WrapResultSetMetaData(ResultSetMetaData *rs);

class Wrap : public CefBase {
public:
	Wrap(ResultSet *rs) { mRs = rs; mCloseStatement = false;}
	Wrap(PreparedStatement *ps) { mPs = ps; }
	Wrap(ResultSetMetaData *meta) { mMeta = meta; }

	ResultSet *mRs;
	PreparedStatement *mPs;
	ResultSetMetaData *mMeta;
	bool mCloseStatement;

	IMPLEMENT_REFCOUNTING(Wrap);
};

class RestltSetMetaDataV8Handler : public CefV8Handler {
public:
	RestltSetMetaDataV8Handler() {
	}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		CefRefPtr<CefBase> data = object->GetUserData();
		Wrap *wd = (Wrap *)data.get();
		ResultSetMetaData *rs = wd->mMeta;

		if (name == "getColumnCount") {
			int c = rs->getColumnCount();
			retval = CefV8Value::CreateInt(c);
			return true;
		}
		if (name == "getColumnLabel" && arguments.size() == 1 && arguments[0]->IsInt()) {
			int col = arguments[0]->GetIntValue();
			char *c = rs->getColumnLabel(col);
			wchar_t *wc = (wchar_t *)XString::toBytes((void *)c, XString::GBK, XString::UNICODE2);
			retval = CefV8Value::CreateString(CefString(wc));
			return true;
		}
		if (name == "getColumnName" && arguments.size() == 1 && arguments[0]->IsInt()) {
			int col = arguments[0]->GetIntValue();
			char *c = rs->getColumnName(col);
			wchar_t *wc = (wchar_t *)XString::toBytes((void *)c, XString::GBK, XString::UNICODE2);
			retval = CefV8Value::CreateString(CefString(wc));
			return true;
		}
		if (name == "getColumnType" && arguments.size() == 1 && arguments[0]->IsInt()) {
			int col = arguments[0]->GetIntValue();
			int c = rs->getColumnType(col);
			retval = CefV8Value::CreateInt(c);
			return true;
		}
		
		return false;
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(RestltSetMetaDataV8Handler);
};

class ResultSetMetaDataV8Accessor : public CefV8Accessor {
public:
	ResultSetMetaDataV8Accessor(ResultSetMetaData *rs) {
		mMeta = rs;
	}
	virtual bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		if (name == "getColumnCount" || name == "getColumnLabel" || name == "getColumnName" || 
			name == "getColumnType") {

			CefRefPtr<CefV8Handler> hd(s_metaV8);
			retval = CefV8Value::CreateFunction(name, hd);
			return true;
		}

		return false;
	}

	virtual bool Set(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		const CefRefPtr<CefV8Value> value,
		CefString& exception) OVERRIDE {

			// Value does not exist.
			return false;
	}

	IMPLEMENT_REFCOUNTING(ResultSetMetaDataV8Accessor);

	ResultSetMetaData *mMeta;
};


CefRefPtr<CefV8Value> WrapResultSetMetaData(ResultSetMetaData *rs) {
	if (s_metaV8 == NULL) {
		s_metaV8 = new RestltSetMetaDataV8Handler();
	}
	CefRefPtr<CefV8Accessor> accessor =  new ResultSetMetaDataV8Accessor(rs);
	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	obj->SetValue("getColumnCount", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getColumnLabel", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getColumnName", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getColumnType", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	CefRefPtr<Wrap> ptr = new Wrap(rs);
	obj->SetUserData(ptr);
	return obj;
}

class RestltSetV8Handler : public CefV8Handler {
public:
	RestltSetV8Handler() {
	}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		CefRefPtr<CefBase> data = object->GetUserData();
		Wrap *wd = (Wrap *)data.get();
		ResultSet *rs = wd->mRs;

		if (name == "findColumn" && arguments.size() == 1 && arguments[0]->IsString()) {
			CefString v0 = arguments[0]->GetStringValue();
			const wchar_t *s = v0.c_str();
			char *sc = (char *)XString::toBytes((void *)s, XString::UNICODE2, XString::GBK);
			int c = rs->findColumn(sc);
			retval = CefV8Value::CreateInt(c);
			return true;
		}
		if (name == "getBlob") {
			// TODO:
			return true;
		}
		if (name == "getBlobBase64") {
			// TODO:
			return true;
		}
		if (name == "getDouble" && arguments.size() == 1 && arguments[0]->IsInt()) {
			int c = arguments[0]->GetIntValue();
			double d = rs->getDouble(c);
			retval = CefV8Value::CreateDouble(d);
			return true;
		}
		if (name == "getFloat" && arguments.size() == 1 && arguments[0]->IsInt()) {
			int c = arguments[0]->GetIntValue();
			double d = rs->getFloat(c);
			retval = CefV8Value::CreateDouble(d);
			return true;
		}
		if ((name == "getInt" || name == "getInt64") && arguments.size() == 1 && arguments[0]->IsInt()) {
			int c = arguments[0]->GetIntValue();
			int d = rs->getInt(c);
			retval = CefV8Value::CreateInt(d);
			return true;
		}
		if (name == "getString" && arguments.size() == 1 && arguments[0]->IsInt()) {
			int c = arguments[0]->GetIntValue();
			char *s = rs->getString(c);
			wchar_t *ws = (wchar_t *)XString::toBytes((void *)s, XString::GBK, XString::UNICODE2);
			retval = CefV8Value::CreateString(CefString(ws));
			return true;
		}
		if (name == "next") {
			bool b = rs->next();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "close") {
			rs->close();
			if (wd->mCloseStatement) {
				rs->getStatement()->close();
			}
			return true;
		}
		if (name == "getMetaData") {
			ResultSetMetaData *meta = rs->getMetaData();
			retval = WrapResultSetMetaData(meta);
			return true;
		}
		
		return false;
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(RestltSetV8Handler);
};

class ResultSetV8Accessor : public CefV8Accessor {
public:
	ResultSetV8Accessor(ResultSet *rs) {
		mRs = rs;
	}
	virtual bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		if (name == "findColumn" || name == "getBlob" || name == "getBlobBase64" || name == "getDouble" || 
			name == "getFloat" || name == "getInt" || name == "getInt64" || 
			name == "getString" || name == "next" || name == "close" || name == "getMetaData") {

			CefRefPtr<CefV8Handler> hd(s_rsV8);
			retval = CefV8Value::CreateFunction(name, hd);
			return true;
		}

		return false;
	}

	virtual bool Set(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		const CefRefPtr<CefV8Value> value,
		CefString& exception) OVERRIDE {

			// Value does not exist.
			return false;
	}

	IMPLEMENT_REFCOUNTING(ResultSetV8Accessor);

	ResultSet *mRs;
};

class PrepareStatementV8Handler : public CefV8Handler {
public:
	PrepareStatementV8Handler() {
	}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		CefRefPtr<CefBase> data = object->GetUserData();
		Wrap *wd = (Wrap *)data.get();
		PreparedStatement *ps = wd->mPs;

		if (name == "executeQuery" && arguments.size() == 0) {
			ResultSet *rs = ps->executeQuery();
			if (rs == NULL) {
				return false;
			}
			retval = WrapResultSet(rs, false);
			return true;
		}
		if (name == "executeUpdate") {
			int c = ps->executeUpdate();
			retval = CefV8Value::CreateInt(c);
			return true;
		}
		if (name == "getParameterCount") {
			int c = ps->getParameterCount();
			retval = CefV8Value::CreateInt(c);
			return true;
		}
		if (name == "setDouble" && arguments.size() == 2) {
			if (arguments[0]->IsInt() && (arguments[1]->IsDouble() || arguments[1]->IsInt() )) {
				ps->setDouble(arguments[0]->GetIntValue(), arguments[1]->GetDoubleValue());
				return true;
			}
			return false;
		}
		if (name == "setInt" && arguments.size() == 2) {
			if (arguments[0]->IsInt() && arguments[1]->IsInt()) {
				ps->setDouble(arguments[0]->GetIntValue(), arguments[1]->GetIntValue());
				return true;
			}
			return false;
		}
		if (name == "setString" && arguments.size() == 2) {
			if (arguments[0]->IsInt() && arguments[1]->IsString()) {
				CefString v1 = arguments[1]->GetStringValue();
				const wchar_t *s = v1.c_str();
				char *sc = (char *)XString::toBytes((void *)s, XString::UNICODE2, XString::GBK);
				ps->setString(arguments[0]->GetIntValue(), sc);
				return true;
			}
			return false;
		}
		if (name == "getInsertId") {
			int c = ps->getInsertId();
			retval = CefV8Value::CreateInt(c);
			return true;
			return true;
		}
		if (name == "close") {
			ps->close();
			return true;
		}
		if (name == "getMetaData") {
			ResultSetMetaData *meta = ps->getMetaData();
			retval = WrapResultSetMetaData(meta);
			return true;
		}
		
		return false;
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(PrepareStatementV8Handler);
};

class PrepareStatementV8Accessor : public CefV8Accessor {
public:
	PrepareStatementV8Accessor(PreparedStatement *ps) {
		mPs = ps;
	}
	virtual bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		if (name == "executeQuery" || name == "executeUpdate" || name == "getMetaData" || 
			name == "getParameterCount" || name == "setDouble" || name == "setInt" || 
			name == "setString" || name == "getInsertId" || name == "close" ) {

			retval = CefV8Value::CreateFunction(name, CefRefPtr<CefV8Handler>(s_psV8));
			return true;
		}
		return false;
	}

	virtual bool Set(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		const CefRefPtr<CefV8Value> value,
		CefString& exception) OVERRIDE {

			// Value does not exist.
			return false;
	}

	IMPLEMENT_REFCOUNTING(PrepareStatementV8Accessor);
private:
	PreparedStatement *mPs;
};


CefRefPtr<CefV8Value> WrapResultSet(ResultSet *rs, bool closeStament) {
	if (s_rsV8 == NULL) {
		s_rsV8 = new RestltSetV8Handler();
	}
	CefRefPtr<CefV8Accessor> accessor =  new ResultSetV8Accessor(rs);
	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	obj->SetValue("findColumn", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getBlob", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getBlobBase64", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getDouble", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getFloat", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getInt", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getInt64", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getString", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("next", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("close", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getMetaData", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	CefRefPtr<Wrap> ptr = new Wrap(rs);
	ptr->mCloseStatement = closeStament;
	obj->SetUserData(ptr);
	return obj;
}

CefRefPtr<CefV8Value> WrapPrepareStatement(PreparedStatement *ps) {
	if (s_psV8 == NULL) {
		s_psV8 = new PrepareStatementV8Handler();
	}
	CefRefPtr<CefV8Accessor> accessor =  new PrepareStatementV8Accessor(ps);
	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	obj->SetValue("executeQuery", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("executeUpdate", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getMetaData", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getParameterCount", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("setString", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getInsertId", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	CefRefPtr<Wrap> ptr = new Wrap(ps);
	obj->SetUserData(ptr);
	return obj;
}

class DBDriverV8Handler : public CefV8Handler {
public:
	DBDriverV8Handler() {
	}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {
		if (name == "getAutoCommit") {
			retval = CefV8Value::CreateBool(s_con->getAutoCommit());
			return true;
		}
		if (name == "setAutoCommit") {
			if (arguments.size() == 1 && arguments[0]->IsBool()) {
				s_con->setAutoCommit(arguments[0]->GetBoolValue());
				return true;
			}
			return false;
		}
		if (name == "commit") {
			bool c = s_con->commit();
			retval = CefV8Value::CreateBool(c);
			return true;
		}
		if (name == "rollback") {
			bool c = s_con->rollback();
			retval = CefV8Value::CreateBool(c);
			return true;
		}
		if (name == "getError") {
			char *c = s_con->getError();
			wchar_t *wc = (wchar_t *)XString::toBytes((void *)c, XString::GBK, XString::UNICODE2);
			retval = CefV8Value::CreateString(CefString(wc));
			return true;
		}
		if (name == "executeQuery" && arguments.size() == 1 && arguments[0]->IsString()) {
			Statement *s = s_con->createStatement();
			CefString v0 = arguments[0]->GetStringValue();
			const wchar_t *sql = v0.c_str();
			char *sqlc = (char *)XString::toBytes((void *)sql, XString::UNICODE2, XString::GBK);
			ResultSet *rs = s->executeQuery(sqlc);
			if (rs == NULL) {
				return false;
			}
			retval = WrapResultSet(rs, true);
			return true;
		}
		if (name == "executeUpdate" && arguments.size() == 1 && arguments[0]->IsString()) {
			Statement *s = s_con->createStatement();
			CefString v0 = arguments[0]->GetStringValue();
			const wchar_t *sql = v0.c_str();
			char *sqlc = (char *)XString::toBytes((void *)sql, XString::UNICODE2, XString::GBK);
			int num = s->executeUpdate(sqlc);
			retval = CefV8Value::CreateInt(num);
			s->close();
			return true;
		}
		if (name == "prepare" && arguments.size() == 1 && arguments[0]->IsString()) {
			CefString v0 = arguments[0]->GetStringValue();
			const wchar_t *sql = v0.c_str();
			char *sqlc = (char *)XString::toBytes((void *)sql, XString::UNICODE2, XString::GBK);
			PreparedStatement *ps = s_con->prepareStatement(sqlc);
			if (ps == NULL) {
				return false;
			}
			retval = WrapPrepareStatement(ps);
			return true;
		}

		return false;
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(DBDriverV8Handler);
};

class DBDriverV8Accessor : public CefV8Accessor {
public:
	DBDriverV8Accessor() {
	}
	virtual bool Get(const CefString& name,
                   const CefRefPtr<CefV8Value> object,
                   CefRefPtr<CefV8Value>& retval,
                   CefString& exception) OVERRIDE {
		
		if (name == "setAutoCommit" || name == "getAutoCommit" || name == "commit" ||
			name == "rollback" || name == "getError" || name == "executeQuery" ||
			name == "executeUpdate" || name == "prepare") {

			retval = CefV8Value::CreateFunction(name, s_dbV8);
			return true;
		}

		return false;
	}

	virtual bool Set(const CefString& name,
                   const CefRefPtr<CefV8Value> object,
                   const CefRefPtr<CefV8Value> value,
                   CefString& exception) OVERRIDE {
		
		// Value does not exist.
		return false;
	}
	
	IMPLEMENT_REFCOUNTING(DBDriverV8Accessor);
};

std::string GetDbExtention() {

	std::string ext = "var app;"
		"if (!app)"
		"    app = {};"
		"(function() {"
		"    app.GetId = function(a, b) {"
		"        native function GetMId(a, b);"
		"        return GetMId(a, b);"
		"    };"
		"})();";

	return ext;
}

CefRefPtr<CefV8Value> WrapDBDriver(SqlConnection *con) {
	s_con = con;
	if (s_dbV8 == NULL) {
		s_dbV8 = new DBDriverV8Handler();
	}
	CefRefPtr<CefV8Accessor> accessor = new DBDriverV8Accessor();
	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	obj->SetValue("setAutoCommit", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getAutoCommit", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("commit", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("rollback", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getError", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("executeQuery", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("executeUpdate", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("prepare", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	return obj;
}
#include "MyV8Handler.h"
#include "db/SqlDriver.h"
#include "utils/XString.h"
#include "DB.h"
#include "FileHandler.h"
#include "call.h"

MyV8Handler::MyV8Handler()
{
}

bool MyV8Handler::Execute( const CefString& name, CefRefPtr<CefV8Value> object, 
	const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception )
{
	static SqlConnection *sCon = NULL;
	// OpenDBDriver(url, userName, password)
	if (name == "openDBDriver") {
		printf("MyV8Handler::Execute -> openDBDriver \n");
		if (sCon != NULL && !sCon->isClosed()) {
			retval = WrapDBDriver(sCon);
			return true;
		}
		if (arguments.size() >= 1) {
			CefString url = arguments[0]->GetStringValue();

			CefString userName, password;
			if (arguments.size() >= 2) {
				userName = arguments[1]->GetStringValue();
			}
			if (arguments.size() >= 3) {
				password = arguments[2]->GetStringValue();
			}
			char *urlStr = (char *)XString::toBytes((void *)url.c_str(), XString::UNICODE2, XString::GBK);
			char *userNameStr = (char *)XString::toBytes((void *)userName.c_str(), XString::UNICODE2, XString::GBK);
			char *passwordStr = (char *)XString::toBytes((void *)password.c_str(), XString::UNICODE2, XString::GBK);
			
			printf("openDBDriver(%s, %s, %s) \n", urlStr, userNameStr, passwordStr);

			sCon = SqlConnection::open(urlStr, userNameStr, passwordStr);
			if (sCon == NULL) {
				printf("openDBDriver fail \n");
				return false;
			}
			retval = WrapDBDriver(sCon);
			printf("openDBDriver success \n");
			return true;
		}
		return false;
	}

	// callNative(String func-name, String func-desc, Array func-params)
	if (name == "callNative") {
		if (arguments.size() < 2) {
			return false;
		}
		return callNative(object, arguments, retval, exception);
	}

	return false;
}

bool MyV8Handler::callNative(CefRefPtr<CefV8Value> object, 
	const CefV8ValueList& args, CefRefPtr<CefV8Value>& retval, CefString& exception ) {
	
	// AllocConsole();
	// freopen("CONOUT$", "wb", stdout);

	if (args.size() > 3) {
		return false;
	}
	if (args.size() == 3) {
		if (! args[2]->IsArray()) {
			return false;
		}
	}
	if (!args[0]->IsString() || !args[1]->IsString()) {
		return false;
	}

	CefString funcName = args[0]->GetStringValue();
	CefString funcDesc = args[1]->GetStringValue();
	char *fn = XString::unicodeToGbk(funcName.c_str());
	char *fd = XString::unicodeToGbk(funcDesc.c_str());
	if (fn == NULL || fd == NULL) {
		return false;
	}

	int pm = 0;
	CallType ctsType[20];
	CallType retType;
	
	bool b = GetCallInfo(fd, &pm, ctsType, &retType);
	if (! b) {
		return false;
	}

	void *params[20] = {0};
	bool freeParams[20] = {false};
	int paramsCount = 0;
	if (args.size() == 3) {
		paramsCount = args[2]->GetArrayLength();
	}
	if (paramsCount != pm) {
		return false;
	}
	
	// build params
	for (int i = 0; i < paramsCount; ++i) {
		CefRefPtr<CefV8Value> v = args[2]->GetValue(i);
		freeParams[i] = false;

		if (ctsType[i] == CALL_TYPE_CHAR) {
			if (v->IsInt()) {
				params[i] = (void *)(v->GetIntValue());
			} else if (v->IsString()) {
				CefString ss = v->GetStringValue();
				const wchar_t *chs = ss.c_str();
				if (chs == NULL) params[i] = NULL;
				else params[i] = (void *)chs[0];
			} else {
				return false;
			}
		} else if (ctsType[i] == CALL_TYPE_INT) {
			if (v->IsInt()) {
				params[i] = (void *)(v->GetIntValue());
			} else if (v->IsUInt()) {
				params[i] = (void *)(v->GetUIntValue());
			} else {
				return false;
			}
		} else if (ctsType[i] == CALL_TYPE_POINTER) {
			if (v->IsInt()) {
				params[i] = (void *)(v->GetIntValue());
			} else if (v->IsUInt()) {
				params[i] = (void *)(v->GetUIntValue());
			} else {
				return false;
			}
		} else if (ctsType[i] == CALL_TYPE_STRING) {
			if (v->IsNull() || v->IsUndefined()) {
				params[i] = NULL;
			} else if (v->IsString()) {
				CefString cs = v->GetStringValue();
				params[i] = (void *)XString::unicodeToGbk(cs.c_str());
				freeParams[i] = true;
			} else {
				return false;
			}
		} else if (ctsType[i] == CALL_TYPE_WSTRING) {
			if (v->IsNull() || v->IsUndefined()) {
				params[i] = NULL;
			} else if (v->IsString()) {
				CefString cs = v->GetStringValue();
				params[i] = (void *) XString::dupws(cs.c_str());
				freeParams[i] = true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}
	
	// call
	int ret = 0;
	b = Call(fn, fd, params, paramsCount, &ret);
	if (! b) {
		return false;
	}
	
	// free params
	for (int i = 0; i < paramsCount; ++i) {
		if (freeParams[i]) free(params[i]);
	}
	if (retType == CALL_TYPE_VOID) {
		// nothing to do
	} else if (retType == CALL_TYPE_CHAR) {
		retval = CefV8Value::CreateInt(ret);
	} else if (retType == CALL_TYPE_INT) {
		retval = CefV8Value::CreateInt(ret);
	} else if (retType == CALL_TYPE_POINTER) {
		retval = CefV8Value::CreateInt(ret);
	} else if (retType == CALL_TYPE_STRING) {
		char *cr = (char *)ret;
		if (cr == NULL) {
			retval = CefV8Value::CreateNull();
		} else {
			wchar_t *wc = XString::gbkToUnicode(cr);
			CefString cs(wc);
			retval = CefV8Value::CreateString(cs);
			free(wc);
		}
	} else if (retType == CALL_TYPE_WSTRING) {
		wchar_t *cr = (wchar_t *)ret;
		if (cr == NULL) {
			retval = CefV8Value::CreateNull();
		} else {
			CefString cs(cr);
			retval = CefV8Value::CreateString(cs);
		}
	}
	
	return true;
}

// String readFile(String path, [String options]) options: base64 
#include "MyV8Handler.h"
#include "db/SqlDriver.h"
#include "utils/XString.h"
#include "DB.h"
#include "FileHandler.h"
#include "call.h"

class BufferV8Handler;

static BufferV8Handler *s_bufferV8Handler;

class Wrap : public CefBase {
public:
	Wrap(void *buf, int len) { mBuf = buf; mLen = len;}
	
	void *mBuf;
	int mLen;

	IMPLEMENT_REFCOUNTING(Wrap);
};

class BufferV8Handler : public CefV8Handler {
public:
	BufferV8Handler() {
	}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		CefRefPtr<CefBase> ud = object->GetUserData();
		Wrap *wd = static_cast<Wrap*>(ud.get());

		// int length()
		if (name == "length") {
			retval = CefV8Value::CreateInt(wd->mLen);
			return true;
		}

		//void* buffer([int pos])
		if (name == "buffer") {
			if (wd->mBuf == NULL || arguments.size() > 1) {
				return false;
			}
			char *cc = (char *)wd->mBuf;
			if (arguments.size() == 1) {
				if (! arguments[0]->IsInt()) return false;
				int pos = arguments[0]->GetIntValue();
				if (pos < 0 || pos > wd->mLen) return false;
				cc += pos;
			}
			retval = CefV8Value::CreateInt((int)cc);
			return true;
		}

		// void destory()
		if (name == "destroy") {
			if (wd->mBuf) free(wd->mBuf);
			wd->mBuf = NULL;
			wd->mLen = 0;
			return true;
		}

		// int getByte(int pos, [bool sign])
		if (name == "getByte") {
			if (wd->mBuf == NULL || arguments.size() > 2) {
				return false;
			}
			if (arguments.size() >= 1 && arguments[0]->IsInt()) {
				int x = arguments[0]->GetIntValue();
				if (x < 0 || x >= wd->mLen) {
					return false;
				}
				bool sign = false;
				if (arguments.size() == 2) {
					if (arguments[1]->IsBool()) sign = arguments[1]->GetBoolValue();
					else if (arguments[1]->IsInt()) sign = arguments[1]->GetIntValue() != 0;
				}
				if (sign) {
					char *bb = (char *)wd->mBuf + x;
					retval = CefV8Value::CreateInt(*bb);
				} else {
					unsigned char *bb = (unsigned char *)wd->mBuf + x;
					retval = CefV8Value::CreateInt(*bb);
				}
				return true;
			}
			return false;
		}

		// int getShort(int pos, [bool sign])
		if (name == "getShort") {
			if (wd->mBuf == NULL || arguments.size() > 2) {
				return false;
			}
			if (arguments.size() >= 1 && arguments[0]->IsInt()) {
				int x = arguments[0]->GetIntValue();
				if (x < 0 || x >= wd->mLen) {
					return false;
				}
				bool sign = false;
				if (arguments.size() == 2) {
					if (arguments[1]->IsBool()) sign = arguments[1]->GetBoolValue();
					else if (arguments[1]->IsInt()) sign = arguments[1]->GetIntValue() != 0;
				}
				if (sign) {
					char *bb = (char *)wd->mBuf + x;
					short *sb = (short *)bb;
					retval = CefV8Value::CreateInt(*sb);
				} else {
					char *bb = (char *)wd->mBuf + x;
					unsigned short *sb = (unsigned short *)bb;
					retval = CefV8Value::CreateInt(*sb);
				}
				return true;
			}
			return false;
		}

		// int getInt(int pos, [bool sign])
		if (name == "getInt") {
			if (wd->mBuf == NULL || arguments.size() > 2) {
				return false;
			}
			if (arguments.size() >= 1 && arguments[0]->IsInt()) {
				int x = arguments[0]->GetIntValue();
				if (x < 0 || x >= wd->mLen) {
					return false;
				}
				bool sign = false;
				if (arguments.size() == 2) {
					if (arguments[1]->IsBool()) sign = arguments[1]->GetBoolValue();
					else if (arguments[1]->IsInt()) sign = arguments[1]->GetIntValue() != 0;
				}
				if (sign) {
					char *bb = (char *)wd->mBuf + x;
					int *sb = (int *)bb;
					retval = CefV8Value::CreateInt(*sb);
				} else {
					char *bb = (char *)wd->mBuf + x;
					unsigned int *sb = (unsigned int *)bb;
					retval = CefV8Value::CreateUInt(*sb);
				}
				return true;
			}
			return false;
		}

		// void setByte(int pos, byte val)
		if (name == "setByte") {
			if (wd->mBuf == NULL || arguments.size() != 2) {
				return false;
			}
			if (!arguments[0]->IsInt() && !arguments[1]->IsInt()) {
				return false;
			}
			int x = arguments[0]->GetIntValue();
			int v = arguments[1]->GetIntValue();
			if (x < 0 || x >= wd->mLen) {
				return false;
			}
			char *b = (char *)wd->mBuf;
			b[x] = (char)(v & 0xff);
			return true;
		}

		// void setShort(int pos, short val)
		if (name == "setShort") {
			if (wd->mBuf == NULL || arguments.size() != 2) {
				return false;
			}
			if (!arguments[0]->IsInt() && !arguments[1]->IsInt()) {
				return false;
			}
			int x = arguments[0]->GetIntValue();
			int v = arguments[1]->GetIntValue();
			if (x < 0 || x >= wd->mLen) {
				return false;
			}
			char *b = (char *)wd->mBuf + x;
			short *sb = (short *)b;
			*sb = (short)(v & 0xffff);
			return true;
		}

		// void setInt(int pos, int val)
		if (name == "setInt") {
			if (wd->mBuf == NULL || arguments.size() != 2) {
				return false;
			}
			if (!arguments[0]->IsInt() && !arguments[1]->IsInt()) {
				return false;
			}
			int x = arguments[0]->GetIntValue();
			int v = arguments[1]->GetIntValue();
			if (x < 0 || x >= wd->mLen) {
				return false;
			}
			char *b = (char *)wd->mBuf + x;
			int *sb = (int *)b;
			*sb = v;
			return true;
		}

		// String toString([String charset])
		if (name == "toString") {
			if (wd->mBuf == NULL) {
				return false;
			}
			int charset = 0;
			if (arguments.size() == 1 && arguments[0]->IsString()) {
				CefString cs = arguments[0]->GetStringValue();
				if (cs == "gbk" || cs == "GBK" || cs == "gb2312" || cs == "GB2312") {
					charset = 0;
				} else if (cs == "utf8" || cs == "UTF8" || cs == "utf-8" || cs == "UTF-8") {
					charset = 1;
				}
			}
			wchar_t *dd = NULL;
			if (charset == 0) {
				dd = XString::gbkToUnicode((char *)wd->mBuf);
			} else {
				dd = (wchar_t *)XString::toBytes(wd->mBuf, XString::UTF8, XString::UNICODE2);
			}
			retval = CefV8Value::CreateString(CefString(dd));
			if (dd != NULL) free(dd);
			return true;
		}

		return false;
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(BufferV8Handler);
};

static const char * BufferV8Accessor_sNames[] = {
	"length", "buffer", "getByte", "setByte", "getShort",
	"setShort", "getInt", "setInt", "toString", "destroy"};


class BufferV8Accessor : public CefV8Accessor {
public:
	BufferV8Accessor() {
		for (int i = 0; i < sizeof(mFuncs)/sizeof(CefRefPtr<CefV8Value>); ++i) {
			mFuncs[i] = CefRefPtr<CefV8Value>();
		}
	}
	virtual bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		for (int i = 0; i < sizeof(BufferV8Accessor_sNames)/sizeof(const char *); ++i ) {
			if (name != BufferV8Accessor_sNames[i]) {
				continue;
			}
			if (mFuncs[i].get() == NULL) {
				mFuncs[i] = CefV8Value::CreateFunction(name, CefRefPtr<CefV8Handler>(s_bufferV8Handler));
			}
			retval = mFuncs[i];
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
	CefRefPtr<CefV8Value> mFuncs[20];
	IMPLEMENT_REFCOUNTING(BufferV8Accessor);
};


static CefRefPtr<CefV8Value> WrapBuffer(void *buf, int len) {
	static CefRefPtr<CefV8Accessor> accessor =  new BufferV8Accessor();
	if (s_bufferV8Handler == NULL) {
		s_bufferV8Handler = new BufferV8Handler();
	}
	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	for (int i = 0; i < sizeof(BufferV8Accessor_sNames)/sizeof(const char *); ++i ) {
		obj->SetValue(BufferV8Accessor_sNames[i], V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	}
	CefRefPtr<Wrap> ptr = new Wrap(buf, len);
	obj->SetUserData(ptr);
	return obj;
}

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

	if (name == "createBuffer") {
		if (arguments.size() != 1 || !arguments[0]->IsInt()) {
			return false;
		}
		int v = arguments[0]->GetIntValue();
		if (v <= 0 || v > 1024 * 1024 * 100) {
			return false;
		}
		void *buf = malloc(v);
		if (buf == NULL) {
			return false;
		}
		memset(buf, 0, v);
		retval = WrapBuffer(buf, v);
		return true;
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
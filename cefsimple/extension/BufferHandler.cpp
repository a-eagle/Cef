#include "include/cef_v8.h"
#include "db/SqlDriver.h"
#include "utils/XString.h"

class BufferV8Handler;
static bool ExecuteStatic( const CefString& name, CefRefPtr<CefV8Value> object, 
				   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception );

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

		// void setString(int pos, String val)
		if (name == "setString") {
			if (wd->mBuf == NULL || arguments.size() != 2) {
				return false;
			}
			if (!arguments[0]->IsInt() && !arguments[1]->IsString()) {
				return false;
			}
			int x = arguments[0]->GetIntValue();
			CefString v = arguments[1]->GetStringValue();
			if (x < 0 || x >= wd->mLen) {
				return false;
			}
			const wchar_t *wv = v.c_str();
			if (wv == NULL) {
				return false;
			}
			char *b = (char *)wd->mBuf + x;
			int mlen = x + wcslen(wv) * 2 + 2;
			if (mlen < wd->mLen) {
				wcscpy((wchar_t *)b, wv);
			} else {
				int elen = wd->mLen - x - 2;
				memcpy(b, wv, elen);
				char * p = (char *)wd->mBuf + wd->mLen - 2;
				p[0] = p[1] = 0;
			}
			return true;
		}

		// String toString([String charset])
		if (name == "toString") {
			if (wd->mBuf == NULL) {
				return false;
			}
			int charset = 0;
			if (arguments.size() == 0) {
				charset = 1;
			} else if (arguments.size() == 1 && arguments[0]->IsString()) {
				CefString cs = arguments[0]->GetStringValue();
				if (cs == "gbk" || cs == "GBK" || cs == "gb2312" || cs == "GB2312") {
					charset = 2;
				} else if (cs == "utf8" || cs == "UTF8" || cs == "utf-8" || cs == "UTF-8") {
					charset = 3;
				}
			}
			wchar_t *dd = NULL;
			if (charset == 1) {
				dd = (wchar_t *)wd->mBuf;
			} else if (charset == 2) {
				dd = XString::gbkToUnicode((char *)wd->mBuf);
			} else if (charset == 3) {
				dd = (wchar_t *)XString::toBytes(wd->mBuf, XString::UTF8, XString::UNICODE2);
			} else {
				return false;
			}
			retval = CefV8Value::CreateString(CefString(dd));
			if (dd != NULL && charset != 1) free(dd);
			return true;
		}

		return ExecuteStatic(name, object, arguments, retval, exception);
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(BufferV8Handler);
};

static const char * BufferV8Accessor_sNames[] = {
	"length", "buffer", "getByte", "setByte", "getShort",
	"setShort",  "getInt", "setInt", "setString", "toString", "destroy"};


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
		if (name == "__TYPE") {
			retval = CefV8Value::CreateString("NativeBuffer");
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


CefRefPtr<CefV8Value> WrapBuffer(void *buf, int len) {
	static CefRefPtr<CefV8Accessor> accessor =  new BufferV8Accessor();
	
	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	for (int i = 0; i < sizeof(BufferV8Accessor_sNames)/sizeof(const char *); ++i ) {
		obj->SetValue(BufferV8Accessor_sNames[i], V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	}
	obj->SetValue("__TYPE", V8_ACCESS_CONTROL_ALL_CAN_READ, V8_PROPERTY_ATTRIBUTE_READONLY);
	CefRefPtr<Wrap> ptr = new Wrap(buf, len);
	obj->SetUserData(ptr);
	return obj;
}

bool IsNativeBuffer(CefRefPtr<CefV8Value> buf) {
	if (buf == NULL) return false;
	CefRefPtr<CefBase> ud = buf->GetUserData();
	CefRefPtr<CefV8Value> typ = buf->GetValue("__TYPE");
	if (typ == NULL) return false;
	CefString t = typ->GetStringValue();
	return t == "NativeBuffer";
}

void *GetNativeBufer(CefRefPtr<CefV8Value> buf, int *len) {
	CefRefPtr<CefBase> ud = buf->GetUserData();
	*len = 0;
	if (! IsNativeBuffer(buf)) {
		return NULL;
	}
	Wrap *wd = static_cast<Wrap*>(ud.get());
	*len = wd->mLen;
	return wd->mBuf;
}


static bool ExecuteStatic( const CefString& name, CefRefPtr<CefV8Value> object, 
	const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception )
{
	if (name == "NBuffer_create") {
		if (arguments.size() == 0 || arguments.size() > 2) {
			return false;
		}
		
		if (! arguments[0]->IsInt())
			return false;
		void *buf = (void *)arguments[0]->GetIntValue();
		int v = INT_MAX;
		if (arguments.size() == 2 && arguments[1]->IsInt()) {
			v = arguments[1]->GetIntValue();
		}
		if (buf == NULL && v != INT_MAX) {
			buf = malloc(v);
			if (buf == NULL) {
				return false;
			}
			memset(buf, 0, v);
			retval = WrapBuffer(buf, v);
			return true;
		}
		if (buf != NULL) {
			retval = WrapBuffer(buf, v);
			return true;
		}
		return false;
	}

	if (name == "NBuffer_copy") {
		if (arguments.size() != 3 || !arguments[0]->IsInt() || !arguments[1]->IsInt() || !arguments[2]->IsInt()) {
			return false;
		}
		void *dst = (void *)arguments[0]->GetIntValue();
		void *src = (void *)arguments[1]->GetIntValue();
		int len = arguments[2]->GetIntValue();
		if (dst == NULL || src == NULL || len <= 0) {
			return false;
		}
		memcpy(dst, src, len);
		return true;
	}

	return false;
}

void RegisterBufferCode() {
	std::string code = 
		"function NBuffer() {};\n"
		"NBuffer.create = function(addr, len) {native function NBuffer_create(addr, len); return NBuffer_create(addr, len);};\n"
		"NBuffer.copy = function(dest, src, len) {native function NBuffer_copy(); NBuffer_copy(dest, src, len);};\n"
		;
	
	s_bufferV8Handler = new BufferV8Handler();
	s_bufferV8Handler->AddRef();
	CefRegisterExtension("v8/MyV8", code, s_bufferV8Handler);
}
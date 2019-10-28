#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include "include/cef_v8.h"
#include "utils/XString.h"

enum CallType {
	CALL_TYPE_UNDEFIND = -1,
	CALL_TYPE_VOID =    0,        // v
	CALL_TYPE_CHAR =    (1 << 0), // c
	CALL_TYPE_INT =     (1 << 1), // i
	CALL_TYPE_POINTER = (1 << 2), // p
	CALL_TYPE_STRING =  (1 << 3), // s
	CALL_TYPE_WSTRING = (1 << 4), // w

	// Note: bellow types are not support
	CALL_TYPE_DOUBLE =  (1 << 5), // d
	CALL_TYPE_FLOAT =   (1 << 6), // f

	CALL_TYPE_ARRAY =   (1 << 8)  // [
};

static bool GetCallInfo(const char *funcDesc, int *paramNum, CallType *params, CallType *ret);

static bool Call(const char *funcName, const char *funcDesc, void **params, int paramsCount, int *ret);

static CallType GetCallType(char c) {
	switch (c) {
	case 'v': return CALL_TYPE_VOID;
	case 'c': return CALL_TYPE_CHAR;
	case 'i': return CALL_TYPE_INT;
	case 'd': return CALL_TYPE_DOUBLE;
	case 'f': return CALL_TYPE_FLOAT;
	case 'p': return CALL_TYPE_POINTER;
	case 's': return CALL_TYPE_STRING;
	case 'w': return CALL_TYPE_WSTRING;
	case '[': return CALL_TYPE_ARRAY;
	}
	return CALL_TYPE_UNDEFIND;
}

#define  CHECK_TYPE(t) if (t == CALL_TYPE_UNDEFIND) {return false;}

static bool GetCallInfo(const char *funcDesc, int *paramNum, CallType *params, CallType *ret) {
	char tmp[64] = {0};
	char *p = tmp;
	while (*funcDesc != 0) {
		if (*funcDesc != ' ' && *funcDesc != '\t') {
			*p = *funcDesc;
			++p;
		}
		++funcDesc;
	}
	p = tmp;

	// parse return type
	CallType t = GetCallType(*p++);
	CHECK_TYPE(t);
	*ret = t;
	if (t == CALL_TYPE_ARRAY) {
		CallType t2 = GetCallType(*p++);
		CHECK_TYPE(t2);
		*ret = CallType(*ret | t2);
	}

	// parse params
	int pm = 0;
	if (*p++ != '(') {
		return false;
	}
	if (*p == ')') {
		*paramNum = 0;
		return true;
	}
	while (*p) {
		t = GetCallType(*p++);
		CHECK_TYPE(t);
		if (t == CALL_TYPE_VOID) {
			return false;
		}
		params[pm] = t;
		if (t == CALL_TYPE_ARRAY) {
			CallType t2 = GetCallType(*p++);
			CHECK_TYPE(t2);
			params[pm] = CallType(t | t2);
		}
		++pm;
		if (*p == ')') {
			break;
		}
		if (*p != ',') {
			return false;
		}
		++p;
	}
	*paramNum = pm;
	return true;
}

static bool Call(const char *funcName, const char *funcDesc, void **params, int paramsCount, int *ret) {
	static HMODULE kernalModule = GetModuleHandleA("kernel32.dll");
	static HMODULE userModule = GetModuleHandleA("user32.dll");

	int rr = NULL;
	if (funcDesc == NULL || strlen(funcDesc) == 0) {
		return false;
	}

	int paramNum = 0;
	CallType paramsInfo[10];
	CallType retInfo;

	if (! GetCallInfo(funcDesc, &paramNum, paramsInfo, &retInfo)) {
		return false;
	}

	if (paramNum != paramsCount) {
		return false;
	}

	void *addr = (void *)GetProcAddress(kernalModule, funcName);
	if (addr == NULL) {
		addr = (void *)GetProcAddress(userModule, funcName);
	}
	if (addr == NULL) {
		return false;
	}

	// push params
	int v = 0;
	float vf = 0;
	double vd = 0;
	for (int i = paramNum - 1; i >= 0; --i) {
		if (paramsInfo[i] <= CALL_TYPE_WSTRING) {
			v = (int)params[i];
			__asm push v
		} 
#if 0
		else if (paramsInfo[i] == CALL_TYPE_FLOAT) {
			vf = *(float *)params[i];
			__asm push vf
		} else if (paramsInfo[i] == CALL_TYPE_DOUBLE) {
			vd = *(double *)params[i];
			__asm push vd
		}
#endif
	}
	__asm call addr
	__asm mov rr, eax
	if (ret != NULL && retInfo != CALL_TYPE_VOID) {
		*ret = rr;
	}
	
	return true;
}

class CallHandler : public CefV8Handler {
public:
	CallHandler() {}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {
			// callNative(String func-name, String func-desc, Array func-params)
			if (name == "callNative") {
				if (arguments.size() < 2) {
					return false;
				}
				return callNative(object, arguments, retval, exception);
			}
			return false;
	}

	bool callNative(CefRefPtr<CefV8Value> object,
		const CefV8ValueList& args,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) {

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

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(CallHandler);
};


CefRefPtr<CefV8Value> CreateCallNativeFunction() {
	CallHandler *ch = new CallHandler();
	ch->AddRef();
	return CefV8Value::CreateFunction("callNative", ch);
}

int test_main(int argc, char* argv[]) {
	int rr = -1;
	void *params[] = {NULL, "My Text AA", "My Caption AA", (void *)1};
	bool r = Call("MessageBoxA", "i(p, s, s, i)", params, 4, &rr);

	printf("r = %d, rr = %d \n", r, rr);
	// MessageBoxA(NULL, "My Text", "My Title", MB_OK);
	return 0;
}
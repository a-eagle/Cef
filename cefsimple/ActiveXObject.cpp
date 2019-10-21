#include "include/cef_v8.h"
#include <string>
#include <objbase.h>
#include "utils/XString.h"

class ComObject : public CefBase {
public:
	struct Func {
		Func(const wchar_t *name, int dispId, int invokeKind, int paramsNum);
		wchar_t mName[64];
		DISPID mDispID;
		INVOKEKIND mInvokeKind;
		int mParamsNum;
	};
	
	ComObject(IDispatch *dispatch, ITypeInfo *typeInfo);

	void addFunc(Func *func);
	Func* find(const wchar_t *name, INVOKEKIND ik);

	bool invokeGet(const wchar_t *name, VARIANT *result);
	bool invokeSet(const wchar_t *name, const CefRefPtr<CefV8Value> &param);
	bool invokeCall(const wchar_t *name, const CefV8ValueList& arguments, VARIANT *result);
	char *buildJson();

	IDispatch *mDispatch;
	ITypeInfo *mTypeInfo;
	Func* mFuncs[256];
	int mFuncsNum;

	~ComObject();

	static ComObject* create(const wchar_t *pragId);

	IMPLEMENT_REFCOUNTING(ComObject);
};

static CefRefPtr<CefV8Value> WrapComObject(ComObject *com);

ComObject::Func::Func( const wchar_t *name, int dispId, int invokeKind, int paramsNum ) {
	wcscpy(mName, name);
	mDispID = dispId;
	mInvokeKind = INVOKEKIND(invokeKind);
	mParamsNum = paramsNum;
}

ComObject::ComObject( IDispatch *dispatch, ITypeInfo *typeInfo ) {
	mFuncsNum = 0;
	mDispatch = dispatch;
	mTypeInfo = typeInfo;
	memset(mFuncs, 0, sizeof(mFuncs));
}

void ComObject::addFunc( Func *func ) {
	if (mFuncsNum >= sizeof(mFuncs)/sizeof(Func*) - 1) {
		printf("Error: ActiveX has no place to save property \n");
		return;
	}
	mFuncs[mFuncsNum++] = func;
}

ComObject::Func* ComObject::find( const wchar_t *name, INVOKEKIND ik ) {
	for (int i = 0; i < mFuncsNum; ++i) {
		if (ik == mFuncs[i]->mInvokeKind && wcscmp(mFuncs[i]->mName, name) == 0) {
			return mFuncs[i];
		}
	}
	return NULL;
}

static void SetParam(const CefRefPtr<CefV8Value> &param, VARIANT *res) {
	if (param->IsBool()) {
		res->vt = VT_BOOL;
		res->boolVal = param->GetBoolValue() ? -1 : 0;
	} else if (param->IsInt() || param->IsUInt()) {
		res->vt = VT_I4;
		res->lVal = param->GetIntValue();
		printf("SetParam int value = %d \n", res->lVal);
	} else if (param->IsDouble()) {
		res->vt = VT_R8;
		res->dblVal = param->GetDoubleValue();
	} else if (param->IsNull() || param->IsUndefined()) {
		res->vt = VT_NULL;
	} else if (param->IsString()) {
		res->vt = VT_BSTR;
		CefString str = param->GetStringValue();
		res->bstrVal = SysAllocString(str.c_str());
	} else {
		// NOT support
		res->vt = VT_EMPTY;
		printf("SetParam not support param \n");
	}
}

static void DestroyVariant(VARIANT *v, int num) {
	for (int i = 0; i < num; ++i) {
		if (v[i].vt == VT_BSTR && v[i].bstrVal != NULL) {
			SysFreeString(v[i].bstrVal);
		}
	}
}

bool ComObject::invokeGet( const wchar_t *name, VARIANT *result ) {
	ComObject::Func *attr = find(name, INVOKE_PROPERTYGET);
	if (attr == NULL) {
		// not support
		wprintf(L"Error: ComObject.invokeGet has no property: %s \n", name);
		return false;
	}

	VARIANT args[10];
	DISPPARAMS params = {0};
	EXCEPINFO exp = {0};
	UINT argErr = 0;
	params.rgvarg = args;

	params.cArgs = 0;
	// LOCALE_USER_DEFAULT
	HRESULT r = mDispatch->Invoke(attr->mDispID, IID_NULL, GetUserDefaultLCID(), DISPATCH_PROPERTYGET, &params, result, &exp, &argErr);
	if (SUCCEEDED(r)) {
		return true;
	}
	wprintf(L"Error: ComObject.invokeGet get property '%s' fail\n", name);
	return false;
}

bool ComObject::invokeSet( const wchar_t *name, const CefRefPtr<CefV8Value> &param ) {
	ComObject::Func *attr = find(name, INVOKE_PROPERTYPUT);
	if (attr == NULL) {
		attr = find(name, INVOKE_PROPERTYPUTREF);
		// not support
		wprintf(L"Error: ComObject.invokeSet has no property: '%s' \n", name);
		return false;
	}

	VARIANT args;
	DISPPARAMS params = {0};
	VARIANT res = {0};
	EXCEPINFO exp = {0};
	UINT argErr = 0;
	params.rgvarg = &args;

	params.cArgs = 1;

	DISPID dispidPut = DISPID_PROPERTYPUT;
	params.cNamedArgs = 1; // 必须，原因不明
	params.rgdispidNamedArgs = &dispidPut; // 必须，原因不明

	SetParam(param, &args);
	HRESULT r = mDispatch->Invoke(attr->mDispID, IID_NULL, GetUserDefaultLCID(), DISPATCH_PROPERTYPUT, &params, &res, &exp, &argErr);
	DestroyVariant(&args, 1);
	if (SUCCEEDED(r)) {
		return true;
	}
	wprintf(L"Error: ComObject.invokeSet set property '%s' fail\n", name);
	return false;
}

bool ComObject::invokeCall( const wchar_t *name, const CefV8ValueList& arguments, VARIANT *result ) {
	VARIANT args[20];
	DISPPARAMS params = {0};
	// VARIANT res = {0};
	EXCEPINFO exp = {0};
	UINT argErr = 0;
	params.rgvarg = args;

	ComObject::Func *func = find(name, INVOKE_FUNC);
	if (func == NULL) {
		// not support
		printf("Error: ComObject.invokeCall has no method: %s \n", name);
		return false;
	}

	params.cArgs = func->mParamsNum > arguments.size() ? arguments.size() : func->mParamsNum;
	
	for (int i = params.cArgs - 1, j = 0; i >= 0; --i, ++j) {
		VARIANT *arg = &args[j];
		SetParam(arguments[i], arg);
	}

	HRESULT r = mDispatch->Invoke(func->mDispID, IID_NULL, GetUserDefaultLCID(), DISPATCH_METHOD, &params, result, &exp, &argErr);
	DestroyVariant(args, params.cArgs);
	if (SUCCEEDED(r)) {
		return true;
	}
	printf("Error: ComObject.invokeCall  call '%s' fail\n", name);
	return false;
}

static ComObject* LoadTypeLibrary(IDispatch *dispatch) {
	UINT cc = 0;
	HRESULT r = dispatch->GetTypeInfoCount(&cc);
	if (FAILED(r) || cc == 0) {
		// not support
		return NULL;
	}
	ITypeInfo *info = NULL;
	r = dispatch->GetTypeInfo(0, GetUserDefaultLCID(), &info);
	if (FAILED(r)) {
		return NULL;
	}
	TYPEATTR *ta = NULL;
	r = info->GetTypeAttr(&ta);
	if (FAILED(r)) {
		return NULL;
	}

	if (ta->typekind == TKIND_INTERFACE) {
		// 取得第二个接口
		ITypeInfo *info2 = NULL;
		HREFTYPE hRef;
		r = info->GetRefTypeOfImplType(1, &hRef);
		if (FAILED(r)) {
			return NULL;
		}
		info->GetRefTypeInfo(hRef, &info2);
		if (FAILED(r)) {
			return NULL;
		}
		info->ReleaseTypeAttr(ta);
		info = info2;
		r = info->GetTypeAttr(&ta);
		if (FAILED(r)) {
			return NULL;
		}
	}

	ComObject *ci = new ComObject(dispatch, info);
	for (int i = 0; i < ta->cFuncs; ++i) {
		FUNCDESC *fd = NULL;
		r = info->GetFuncDesc(i, &fd);
		if (fd->wFuncFlags & (FUNCFLAG_FRESTRICTED | FUNCFLAG_FHIDDEN)) {
			continue;
		}
		BSTR name = NULL;
		UINT pcn = 0;
		r = info->GetNames(fd->memid, &name, 1, &pcn);
		ci->addFunc(new ComObject::Func(name, fd->memid, fd->invkind, fd->cParams));
	}
	return ci;
}

ComObject::~ComObject() {
	if (mDispatch != NULL) {
		mDispatch->Release();
	}
}

ComObject* ComObject::create( const wchar_t *pragId ) {
	int len = wcslen(pragId);
	CLSID clsid = {0};
	HRESULT r = 0;

	if (len == 38 && pragId[0] == '{' && pragId[37] == '}' && pragId[9] == '-' && pragId[14] == '-' && pragId[19] == '-' && pragId[24] == '-') {
		// it is class id
		r = CLSIDFromString(pragId, &clsid);
	} else {
		r = CLSIDFromProgID(pragId, &clsid);
	}
	if (FAILED(r)) {
		printf("ActiveXObject not found: %s \n ", pragId);
		return NULL;
	}
	IDispatch *dispatch = NULL;
	r = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IDispatch, (void **)&dispatch);
	if (FAILED(r)) {
		return NULL;
	}
	// here load type library
	ComObject *obj = LoadTypeLibrary(dispatch);
	return obj;
}


static void SetReturnVal(CefRefPtr<CefV8Value>& val, VARIANT &res) {
	// printf("SetReturnVal vt= %d \n", res.vt);

	if (res.vt == VT_NULL || res.vt == VT_EMPTY) {
		val = CefV8Value::CreateNull();
	} else if (res.vt == VT_I1 || res.vt == VT_I2 || res.vt == VT_I4 ) {
		long v = res.lVal;
		val = CefV8Value::CreateInt(v);
	} else if (res.vt == VT_UI1 ||  res.vt == VT_UI2 || res.vt == VT_UI4) {
		double v = res.ulVal;
		val = CefV8Value::CreateUInt(v);
	} else if (res.vt == VT_R4) {
		double v = res.fltVal;
		val = CefV8Value::CreateDouble(v);
	} else if (res.vt == VT_R8) {
		double v = res.dblVal;
		val = CefV8Value::CreateDouble(v);
	} else if (res.vt == VT_BOOL) {
		bool b = res.boolVal == 0 ? false : true;
		val = CefV8Value::CreateBool(b);
	} else if (res.vt == VT_BSTR) {
		BSTR str = res.bstrVal;
		val = CefV8Value::CreateString(CefString((wchar_t *)str));
	} else if (res.vt == VT_DISPATCH) {
		if (res.pdispVal == NULL) {
			val = CefV8Value::CreateNull();
		} else {
			ComObject *com = LoadTypeLibrary(res.pdispVal);
			val = WrapComObject(com);
		}
	} else {
		printf("ActiveXObject not support value type : %d\n", res.vt);
	}
}

class ComV8Handler : public CefV8Handler {
public:
	ComV8Handler() {}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE;


	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(ComV8Handler);
};

static ComV8Handler *s_comV8;

class ComV8Accessor : public CefV8Accessor {
public:
	ComV8Accessor(ComObject *com) {
		mCom = com;
	}
	virtual bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

			const wchar_t *bname = name.c_str();
			ComObject::Func *func = mCom->find(bname, INVOKE_PROPERTYGET);
			if (func != NULL) {
				VARIANT res = {0};
				if (mCom->invokeGet(bname, &res)) {
					SetReturnVal(retval, res);
					return true;
				}
				return false;
			}

			func = mCom->find(bname, INVOKE_FUNC);
			if (func != NULL) {
				// static CefRefPtr<CefV8Handler> hd = s_comV8; // fix bug, must use static
				retval = CefV8Value::CreateFunction(name, s_comV8);
				return true;
			}
			return false;
	}

	virtual bool Set(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		const CefRefPtr<CefV8Value> value,
		CefString& exception) OVERRIDE {

			const wchar_t *bname = name.c_str();
			ComObject::Func *func = mCom->find(bname, INVOKE_PROPERTYPUT);
			if(func == NULL) {
				func = mCom->find(bname, INVOKE_PROPERTYPUTREF);
			}
			if (func != NULL) {
				static CefRefPtr<CefV8Handler> hd = s_comV8; // fix bug, must use static
				VARIANT res = {0};
				if (mCom->invokeSet(bname, value)) {
					return true;
				}
				return false;
			}
			
			return false;
	}

	IMPLEMENT_REFCOUNTING(ComV8Accessor);
private:
	ComObject *mCom;
};

static CefRefPtr<CefV8Value> WrapComObject(ComObject *com) {
	CefRefPtr<CefV8Accessor> accessor =  new ComV8Accessor(com);
	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	for (int i = 0; i < com->mFuncsNum; ++i) {
		CefString name((const wchar_t *)com->mFuncs[i]->mName);
		obj->SetValue(name, V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	}
	obj->SetUserData(com);
	return obj;
}

bool ComV8Handler::Execute( const CefString& name, CefRefPtr<CefV8Value> obj, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception ) {
	if (name == "NewActiveXObject") {
		if (! arguments[0]->IsString()) {
			return false;
		}
		CefString  progId = arguments[0]->GetStringValue();
		if (progId.empty()) {
			return false;
		}
		ComObject *com = ComObject::create(progId.c_str());
		if (com == NULL) {
			return false;
		}
		retval = WrapComObject(com);
		return true;
	}

	// wprintf(L"invoke %s \n", name.c_str());
	CefRefPtr<CefBase>  combb = obj->GetUserData();
	ComObject *com = static_cast<ComObject*>(combb.get());
	ComObject::Func *func = com->find(name.c_str(), INVOKE_FUNC);
	if (func == NULL) {
		return false;
	}
	VARIANT result = {0};
	if (com->invokeCall(name.c_str(), arguments, &result)) {
		SetReturnVal(retval, result);
		return true;
	}
	return false;
}

void RegisterComCode() {
	std::string zip_code = 
		"native function NewActiveXObject(progId); \n"
		"function ActiveXObject2(progId) {return NewActiveXObject(progId);} \n"
		;
	s_comV8 = new ComV8Handler();
	s_comV8->AddRef();
	CefRegisterExtension("v8/COM", zip_code, s_comV8);
}

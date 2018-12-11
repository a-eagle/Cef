#include "FileHandler.h"
#include "utils/File.h"

class FileV8Handler;
static FileV8Handler *s_v8Handler;

class Wrap : public CefBase {
public:
	Wrap(File *f) { mFile = f;}

	File *mFile;
	IMPLEMENT_REFCOUNTING(Wrap);
};

class FileV8Handler : public CefV8Handler {
public:
	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

		CefRefPtr<CefBase> data = object->GetUserData();
		Wrap *wd = (Wrap *)data.get();
		File *file = wd->mFile;

		if (name == "getName") {
			XString n = file->getName();
			wchar_t *ws = XString::gbkToUnicode(n.str());
			retval = CefV8Value::CreateString(CefString(ws));
			free(ws);
			return true;
		}
		if (name == "getParent") {
			XString n = file->getParent();
			wchar_t *ws = XString::gbkToUnicode(n.str());
			retval = CefV8Value::CreateString(CefString(ws));
			free(ws);
			return true;
		}
		if (name == "getParentFile") {
			XString pf = file->getParent();
			if (pf.length() == 0) {
				return false;
			}
			wchar_t *ws = XString::gbkToUnicode(pf.str());
			bool b = WrapOpenFile(retval, CefString(ws));
			free(ws);
			return b;
		}
		if (name == "getPath") {
			XString n = file->getPath();
			wchar_t *ws = XString::gbkToUnicode(n.str());
			retval = CefV8Value::CreateString(CefString(ws));
			free(ws);
			return true;
		}
		if (name == "isAbsolute") {
			bool b = file->isAbsolute();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "getAbsolutePath") {
			XString n = file->getAbsolutePath();
			wchar_t *ws = XString::gbkToUnicode(n.str());
			retval = CefV8Value::CreateString(CefString(ws));
			free(ws);
			return true;
		}
		if (name == "exists") {
			bool b = file->exists();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "isDirectory") {
			bool b = file->isDirectory();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "isFile") {
			bool b = file->isFile();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "length") {
			int b = file->length();
			retval = CefV8Value::CreateInt(b);
			return true;
		}
		if (name == "del") {
			bool b = file->del();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "mkdir") {
			bool b = file->mkdir();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "mkdirs") {
			bool b = file->mkdirs();
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "rename") {
			if (arguments.size() != 1 && !arguments[0]->IsString()) {
				retval = CefV8Value::CreateBool(false);
				return false;
			}
			CefString s = arguments[0]->GetStringValue();
			char *cs = XString::unicodeToGbk(s.c_str());
			bool b = file->rename(cs);
			free(cs);
			retval = CefV8Value::CreateBool(b);
			return true;
		}
		if (name == "list") {
			int num = 0;
			XString *ss = file->list(num);
			retval = CefV8Value::CreateArray(num);
			for (int i = 0; i < num; ++i) {
				char *v = ss[i].str();
				wchar_t *wv = XString::gbkToUnicode(v);
				retval->SetValue(i, CefV8Value::CreateString(CefString(wv)));
				free(wv);
			}
			// if (ss != NULL) free(ss);
			return true;
		}
		return false;
	}

	IMPLEMENT_REFCOUNTING(FileV8Handler);
};

class FileV8Accessor : public CefV8Accessor {
public:
	FileV8Accessor() {
	}
	virtual bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {

			if (name == "getName" || name == "getParent" || name == "getParentFile" || 
				name == "getPath" || name == "isAbsolute" || name == "getAbsolutePath" || 
				name == "exists" || name == "isDirectory" || name == "isFile" ||
				name == "length" || name == "del" || name == "mkdir" || name == "mkdirs" || 
				name == "rename" || name == "list") {

					retval = CefV8Value::CreateFunction(name, CefRefPtr<CefV8Handler>(s_v8Handler));
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

	IMPLEMENT_REFCOUNTING(FileV8Accessor);
};

bool WrapOpenFile( CefRefPtr<CefV8Value>& retval, CefString path ) {
	if (path.length() == 0) {
		return false;
	}
	if (s_v8Handler == NULL) {
		s_v8Handler = new FileV8Handler();
	}
	const wchar_t *wp = path.c_str();
	char *sp = (char *)XString::toBytes((void *)wp, XString::UNICODE2, XString::GBK);
	File *file = new File(sp);
	CefRefPtr<CefV8Accessor> accessor =  new FileV8Accessor();
	free(sp);

	CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(accessor);
	obj->SetValue("getName", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getParent", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getParentFile", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getPath", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("isAbsolute", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("getAbsolutePath", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("exists", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("isDirectory", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("isFile", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("length", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("del", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("mkdir", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("mkdirs", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("rename", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	obj->SetValue("list", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
	CefRefPtr<Wrap> ptr = new Wrap(file);
	obj->SetUserData(ptr);
	retval = obj;
	return true;
}

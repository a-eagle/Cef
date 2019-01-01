#include "include/cef_v8.h"
#include <string>
#include "zip.h"
#include "unzip.h"

class ZipV8Handler : public CefV8Handler {
public:
	ZipV8Handler() {}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE;

	
	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(ZipV8Handler);
};

bool ZipV8Handler::Execute( const CefString& name, CefRefPtr<CefV8Value> obj, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception ) 
{
	CefRefPtr<CefV8Value> object = arguments[0];
	CefRefPtr<CefV8Value> pathV = object->GetValue("_path");
	CefRefPtr<CefV8Value> nativeOjbV = object->GetValue("_nativeObj");
	CefRefPtr<CefV8Value> modeV = object->GetValue("_mode");

	if (! pathV->IsString()) {
		return false;
	}
	CefString  path = pathV->GetStringValue();
	if (path.empty()) {
		return false;
	}

	if (name == "ZIP_create") {
		HZIP z = CreateZip(path.c_str(), NULL);
		if (z != NULL) {
			object->SetValue("_nativeObj", CefV8Value::CreateInt((int)z), V8_PROPERTY_ATTRIBUTE_NONE);
		}
		retval = CefV8Value::CreateBool(z != NULL);
		return true;
	}
	if (name == "ZIP_open") {
		HZIP z = OpenZip(path.c_str(), NULL);
		if (z != NULL) {
			object->SetValue("_nativeObj", CefV8Value::CreateInt((int)z), V8_PROPERTY_ATTRIBUTE_NONE);
		}
		retval = CefV8Value::CreateBool(z != NULL);
		return true;
	}
	if (name == "ZIP_close") {
		if (modeV->IsNull()) return false;
		CefString m = modeV->GetStringValue();
		ZRESULT z = 0;
		HZIP hz = (HZIP)nativeOjbV->GetIntValue();

		if (m == "CREATE") z = CloseZipZ(hz);
		else if (m == "OPEN") z = CloseZipU(hz);
		retval = CefV8Value::CreateBool(z == 0);
		return true;
	}
	if (name == "ZIP_add") {
		if (arguments.size() != 3) return false;
		if (!arguments[1]->IsString() || !arguments[2]->IsString()) {
			return false;
		}
		CefString itemName = arguments[1]->GetStringValue();
		CefString filePath = arguments[2]->GetStringValue();
		HZIP hz = (HZIP)nativeOjbV->GetIntValue();
		ZRESULT z = ZipAdd(hz, itemName.c_str(), filePath.c_str());
		retval = CefV8Value::CreateBool(z == 0);
		return true;
	}
	if (name == "ZIP_getItem") {
		if (arguments.size() != 2) return false;
		if (!arguments[1]->IsInt()) {
			return false;
		}
		int idx = arguments[1]->GetIntValue();
		HZIP hz = (HZIP)nativeOjbV->GetIntValue();
		ZIPENTRY en;
		ZRESULT z = GetZipItem(hz, idx, &en);
		
		if (z == 0) {
			retval = CefV8Value::CreateObject(NULL);
			retval->SetValue("index", CefV8Value::CreateInt(en.index), V8_PROPERTY_ATTRIBUTE_NONE);
			retval->SetValue("name", CefV8Value::CreateString(en.name), V8_PROPERTY_ATTRIBUTE_NONE);
			retval->SetValue("compressSize", CefV8Value::CreateInt(en.comp_size), V8_PROPERTY_ATTRIBUTE_NONE);
			retval->SetValue("unCompressSize", CefV8Value::CreateInt(en.unc_size), V8_PROPERTY_ATTRIBUTE_NONE);
		}
		return true;
	}
	if (name == "ZIP_unzipItem") {
		if (arguments.size() != 3) return false;
		if (!arguments[1]->IsInt() || !arguments[2]->IsString()) {
			return false;
		}
		int idx = arguments[1]->GetIntValue();
		CefString filePath = arguments[2]->GetStringValue();
		HZIP hz = (HZIP)nativeOjbV->GetIntValue();
		ZRESULT z = UnzipItem(hz, idx, filePath.c_str());
		retval = CefV8Value::CreateBool(z == 0);
		return true;
	}
	return false;
}

void RegisterZipCode() {
	std::string zip_code = 
		"function ZIP(pathOrSize) {"
		"	if ((typeof pathOrSize) == 'string') this._path = pathOrSize;"
		"	if ((typeof pathOrSize) == 'number') this._bufSize = pathOrSize;"
		"	this._nativeObj = null;"
		"};"
		"ZIP.prototype.create = function() {"
		"	native function ZIP_create();"
		"	this._mode = 'CREATE';"
		"	return ZIP_create(this);"
		"};"
		"ZIP.prototype.open = function() {"
		"	native function ZIP_open();"
		"	this._mode = 'OPEN';"
		"	return ZIP_open(this);"
		"};"
		"ZIP.prototype.close = function() {"
		"	native function ZIP_close();"
		"	return ZIP_close(this);"
		"};"
		"ZIP.prototype.add = function(itemName, filePath) {"
		"	native function ZIP_add();"
		"	return ZIP_add(this, itemName, filePath);"
		"};"
		"ZIP.prototype.getItem = function(idx) {"
		"	native function ZIP_getItem();"
		"	return ZIP_getItem(this, idx);"
		"};"
		"ZIP.prototype.unzipItem = function(idx, filePath) {"
		"	native function ZIP_unzipItem();"
		"	return ZIP_unzipItem(this, idx, filePath);"
		"};"
		"ZIP.prototype.getItemCount = function() {"
		"	native function ZIP_getItem();"
		"	var o = ZIP_getItem(this, -1);"
		"	if (o) return o.index;"
		"	return 0;"
		"};"
		"ZIP.prototype.findItem = function(itemName) {"
		"	var cc = this.getItemCount();"
		"	for (var i = 0; i < cc; ++i) {"
		"		var gi = this.getItem(i);"
		"		if (gi.name == itemName) return gi;"
		"	};"
		"	return null;"
		"};"
		"";
	
	CefRegisterExtension("v8/ZIP", zip_code, new ZipV8Handler());
}

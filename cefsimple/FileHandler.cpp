#include "include/cef_v8.h"
#include <string>
#include <windows.h>
#include <stdlib.h>
#include <vector>
#include "utils/XString.h"

extern CefRefPtr<CefV8Value> WrapBuffer(void *buf, int len);
extern void *GetNativeBufer(CefRefPtr<CefV8Value> buf, int *len);
extern bool IsNativeBuffer(CefRefPtr<CefV8Value> buf);

class FileV8Handler : public CefV8Handler {
public:
	FileV8Handler() {}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE;


	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(FileV8Handler);
};

bool FileV8Handler::Execute( const CefString& name, CefRefPtr<CefV8Value> obj, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception )
{
	CefRefPtr<CefV8Value> object = arguments[0];
	CefRefPtr<CefV8Value> pathV = object->GetValue("_path");
	if (pathV == NULL || pathV->IsNull() || !pathV->IsString()) {
		return false;
	}
	CefString path = pathV->GetStringValue();

	if (name == "FILE_exists") {
		WIN32_FIND_DATAW  fd;
		HANDLE hd;
		hd = FindFirstFileW(path.c_str(), &fd);
		if (hd != INVALID_HANDLE_VALUE) FindClose(hd);
		retval = CefV8Value::CreateBool(hd != INVALID_HANDLE_VALUE);
		return true;
	}
	if (name == "FILE_isDirectory") {
		WIN32_FIND_DATAW  fd;
		HANDLE hd;
		hd = FindFirstFileW(path.c_str(), &fd);

		bool isd = false;
		if (hd != INVALID_HANDLE_VALUE) {
			isd = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			FindClose(hd);
		}
		retval = CefV8Value::CreateBool(isd);
		return true;
	}
	if (name == "FILE_length") {
		WIN32_FIND_DATAW  fd;
		HANDLE hd;
		hd = FindFirstFileW(path.c_str(), &fd);

		int len = 0;
		if (hd != INVALID_HANDLE_VALUE) {
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				len = fd.nFileSizeLow;
			}
			FindClose(hd);
		}
		retval = CefV8Value::CreateInt(len);
		return true;
	}
	if (name == "FILE_read") {
		HANDLE hd = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hd) return false;
		DWORD dwSize = GetFileSize(hd, NULL);
		char *buf = (char *)malloc(dwSize + 2);
		DWORD rr = 0;
		BOOL b = ReadFile(hd, buf, dwSize, &rr, NULL);
		buf[rr] = buf[rr + 1] = 0;
		CloseHandle(hd);
		if (b) retval = WrapBuffer(buf, dwSize + 2);
		return b;
	}
	if (name == "FILE_write") {
		HANDLE hd = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS|TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hd) return false;
		CefRefPtr<CefV8Value> data = arguments[1];
		if (data == NULL) return false;
		
		bool b = false;
		if (data->IsString()) {
			CefString str = data->GetStringValue();
			void *bdata = XString::toBytes((void *)str.c_str(), XString::UNICODE2, XString::UTF8);
			if (bdata != NULL) {
				int len = strlen((char *)bdata);
				DWORD wlen = 0;
				b = WriteFile(hd, bdata, len, &wlen, NULL);
				free(bdata);
			}
		} else if (IsNativeBuffer(data)) {
			// data is Buffer
			int len = 0;
			void *bdata = GetNativeBufer(data, &len);
			if (bdata != NULL) {
				DWORD wlen = 0;
				b = WriteFile(hd, bdata, len, &wlen, NULL);
			}
		}
		CloseHandle(hd);
		return b;
	}
	if (name == "FILE_list") {
		WIN32_FIND_DATAW  fd;
		HANDLE hd;
		WCHAR spath[MAX_PATH];
		wcscpy(spath, path.c_str());
		int slen = wcslen(spath);
		if (slen >= 0) {
			if (spath[slen] != '\\') wcscat(spath, L"\\");
		}
		bool add = false;
		if (arguments.size() == 2) {
			if (arguments[1] != NULL && arguments[1]->IsString()) {
				CefString pa = arguments[1]->GetStringValue();
				const WCHAR *wp = pa.c_str();
				if (wp != NULL && pa.size() > 0) {
					add = true;
					wcscat(spath, wp);
				}
			}
		}
		if (!add) wcscat(spath, L"*");
		hd = FindFirstFileW(spath, &fd);
		if (hd != INVALID_HANDLE_VALUE) {
			std::vector<CefRefPtr<CefV8Value> > vec;
			do {
				if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) {
					continue;
				}
				vec.push_back(CefV8Value::CreateString(fd.cFileName));
			} while (FindNextFile(hd, &fd));

			retval = CefV8Value::CreateArray(vec.size());
			for (int i = 0; i < vec.size(); ++i) {
				retval->SetValue(i, vec[i]);
			}
		}
		FindClose(hd);
		return true;
	}


	return false;
}

void RegisterFileCode() {
	std::string zip_code = 
		"function FILE(path) {"
		"	this._path = path.replace('/', '\\\\');"
		"};"
		"FILE.prototype.getName = function() {var idx = this._path.lastIndexOf('\\\\'); if (idx < 0) return this._path; return this._path.substring(idx + 1);};"
		"FILE.prototype.exists = function() {native function FILE_exists(); return FILE_exists(this);};"
		"FILE.prototype.isDirectory = function() {native function FILE_isDirectory(); return FILE_isDirectory(this);};"
		"FILE.prototype.length = function() {native function FILE_length(); return FILE_length(this);};"
		"FILE.prototype.read = function() {native function FILE_read(); return FILE_read(this);};"
		"FILE.prototype.write = function(data) {native function FILE_write();return FILE_write(this, data);};"
		"FILE.prototype.list = function(pattern) {native function FILE_list();return FILE_list(this, pattern);};"
		"\n";

	CefRegisterExtension("v8/LocalFile", zip_code, new FileV8Handler());
}
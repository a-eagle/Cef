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

static bool FILE_exists(const wchar_t *path) {
	WIN32_FIND_DATAW  fd;
	HANDLE hd;
	hd = FindFirstFileW(path, &fd);
	if (hd != INVALID_HANDLE_VALUE) FindClose(hd);
	return (hd != INVALID_HANDLE_VALUE);
}

static bool FILE_isDirectory(const wchar_t *path) {
	WIN32_FIND_DATAW  fd;
	HANDLE hd;
	hd = FindFirstFileW(path, &fd);

	bool isd = false;
	if (hd != INVALID_HANDLE_VALUE) {
		isd = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		FindClose(hd);
	}
	return isd;
}

static int FILE_length(const wchar_t *path) {
	WIN32_FIND_DATAW  fd;
	HANDLE hd;
	hd = FindFirstFileW(path, &fd);

	int len = 0;
	if (hd != INVALID_HANDLE_VALUE) {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			len = fd.nFileSizeLow;
		}
		FindClose(hd);
	}
	return len;
}

static bool FILE_read(const wchar_t *path, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval) {
	HANDLE hd = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

static bool FILE_write(const wchar_t *path, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval) {
	HANDLE hd = CreateFileW(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hd) {
		return false;
	}
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
	retval = CefV8Value::CreateBool(b);
	return b;
}

static void FILE_list(const wchar_t *path, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval) {
	WIN32_FIND_DATAW  fd;
	HANDLE hd;
	WCHAR spath[MAX_PATH];
	wcscpy(spath, path);
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
}

static bool FILE_mkdirs(const wchar_t *wpath) {
	wchar_t pb[MAX_PATH];
	const wchar_t *pos = wcsstr(wpath, L":\\");
	const wchar_t *from = wpath;

	if (pos != NULL) from = pos + 2;
	while ((pos = wcschr(from, '\\')) != NULL) {
		int len = pos - wpath;
		memcpy(pb, wpath, len * 2);
		pb[len] = 0;
		from = pos + 1;
		
		if (! FILE_exists(pb)) {
			bool ok = CreateDirectory(pb, NULL);
			if (! ok) return false;
		} else if (! FILE_isDirectory(pb)) {
			return false;
		}
	}
	return false;
}

static bool FILE_mkdir(const wchar_t *wpath) {
	return CreateDirectory(wpath, NULL);
}

static bool FILE_remove(const wchar_t *path) {
	if (! FILE_exists(path)) {
		return true;
	}
	if (FILE_isDirectory(path)) {
		return RemoveDirectory(path);
	}
	return DeleteFile(path);
}


bool FileV8Handler::Execute( const CefString& name, CefRefPtr<CefV8Value> obj, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception )
{
	CefRefPtr<CefV8Value> object = arguments[0];
	CefRefPtr<CefV8Value> pathV = object->GetValue("_path");
	if (pathV == NULL || pathV->IsNull() || !pathV->IsString()) {
		return false;
	}
	CefString cpath = pathV->GetStringValue();
	const wchar_t *path = cpath.c_str();

	if (name == "FILE_exists") {
		bool e = FILE_exists(path);
		retval = CefV8Value::CreateBool(e);
		return true;
	}
	if (name == "FILE_isDirectory") {
		bool e = FILE_isDirectory(path);
		retval = CefV8Value::CreateBool(e);
		return true;
	}
	if (name == "FILE_length") {
		int len = FILE_length(path);
		retval = CefV8Value::CreateInt(len);
		return true;
	}
	if (name == "FILE_read") {
		FILE_read(path, arguments, retval);
		return true;
	}
	if (name == "FILE_write") {
		FILE_write(path, arguments, retval);
		return true;
	}
	if (name == "FILE_list") {
		FILE_list(path, arguments, retval);
		return true;
	}
	if (name == "FILE_mkdirs") {
		bool b = FILE_mkdirs(path);
		retval = CefV8Value::CreateBool(b);
		return true;
	}
	if (name == "FILE_mkdir") {
		bool b = FILE_mkdir(path);
		retval = CefV8Value::CreateBool(b);
		return true;
	}
	if (name == "FILE_remove") {
		bool b = FILE_remove(path);
		retval = CefV8Value::CreateBool(b);
		return true;
	}

	return false;
}

void RegisterFileCode() {
	std::string zip_code = 
		"function NFile(path) {"
		"	this._path = path.replace(/[/]/g, '\\\\');"
		"};"
		"NFile.prototype.getName = function() {var idx = this._path.lastIndexOf('\\\\'); if (idx < 0) return this._path; return this._path.substring(idx + 1);};"
		"NFile.prototype.exists = function() {native function FILE_exists(); return FILE_exists(this);};"
		"NFile.prototype.isDirectory = function() {native function FILE_isDirectory(); return FILE_isDirectory(this);};"
		"NFile.prototype.length = function() {native function FILE_length(); return FILE_length(this);};"
		"NFile.prototype.read = function() {native function FILE_read(); return FILE_read(this);};"
		"NFile.prototype.write = function(data) {native function FILE_write();return FILE_write(this, data);};"
		"NFile.prototype.list = function(pattern) {native function FILE_list();return FILE_list(this, pattern);};"
		"NFile.prototype.mkdirs = function() {native function FILE_mkdirs();return FILE_mkdirs(this);};"
		"NFile.prototype.mkdir = function() {native function FILE_mkdir();return FILE_mkdir(this);};"
		"NFile.prototype.remove = function() {native function FILE_remove();return FILE_remove(this);};"
		"\n";

	CefRegisterExtension("v8/LocalFile", zip_code, new FileV8Handler());
}
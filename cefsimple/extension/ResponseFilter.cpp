#include "ResponseFilter.h"
#include "include/cef_response.h"
#include "utils/XString.h"

CefString ResponseFilter::mArgs;
void * ResponseFilter::mFilterAddr;

ResponseFilter::ResponseFilter( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, 
							   CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response ) {
	mUrl = request->GetURL();
}

bool ResponseFilter::InitFilter() {
	return true;
}

typedef bool (*FilterResponse_t)(const wchar_t *url, void* data_in, size_t data_in_size, size_t& data_in_read, void* data_out, size_t data_out_size, size_t& data_out_written );

static FILE *f;
CefResponseFilter::FilterStatus ResponseFilter::Filter( void* data_in, size_t data_in_size, size_t& data_in_read, void* data_out, size_t data_out_size, size_t& data_out_written ) {
	/*
	if (f == NULL) f = fopen("debug_f.log", "wb");
	char buf[256];
	char *url = (char *)XString::toBytes((void *)mUrl.c_str(), XString::UNICODE2, XString::UTF8);
	sprintf(buf, "\nFilter Url: %s \n", url);
	fwrite(buf, strlen(buf), 1, f);
	sprintf(buf, "filter: data_in_size=%d data_in_read=%d data_out_size=%d data_out_written=%d || data_in=%p  data_out=%p\n", 
		data_in_size, data_in_read, data_out_size, data_out_written, data_in, data_out);
	fwrite(buf, strlen(buf), 1, f);
	fwrite(data_in, data_in_size, 1, f);
	const char *s = "\n----------------------------------------\n\n\n";
	fwrite(s, strlen(s), 1, f);
	fflush(f);
	*/

	bool done = false;
	if (mFilterAddr != NULL && data_in_size > 0) {
		FilterResponse_t t = (FilterResponse_t)mFilterAddr;
		done = t(mUrl.c_str(), data_in, data_in_size, data_in_read, data_out, data_out_size, data_out_written);
	} 
	if (! done) {
		data_in_read = data_in_size;
		memcpy(data_out, data_in, data_in_size);
		data_out_written = data_in_size;
	}

	return RESPONSE_FILTER_DONE;
	// return RESPONSE_FILTER_NEED_MORE_DATA;
}

void ResponseFilter::initArgs( CefString &args ) {
	if (args == mArgs) {
		return;
	}
	mArgs = args;
	HMODULE module = LoadLibraryW(mArgs.c_str());
	if (module == NULL) {
		return;
	}
	void *addr = GetProcAddress(module, "FilterResponse");
	if (addr == NULL) {
		return;
	}
	mFilterAddr = addr;
}

bool ResponseFilter::enableFilter() {
	return mFilterAddr != NULL;
}

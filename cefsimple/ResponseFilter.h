#pragma  once
#include "include/cef_response_filter.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_request.h"
#include "include/cef_response.h"

class ResponseFilter : public CefResponseFilter {
public:
	ResponseFilter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response);
	virtual bool InitFilter();

	virtual FilterStatus Filter(void* data_in,
		size_t data_in_size,
		size_t& data_in_read,
		void* data_out,
		size_t data_out_size,
		size_t& data_out_written);

	static void initArgs(CefString &args);
protected:
	static CefString mArgs;
	static void *mFilterAddr;
	CefString mUrl;
	

	IMPLEMENT_REFCOUNTING(ResponseFilter)
};
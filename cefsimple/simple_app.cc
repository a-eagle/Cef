// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefsimple/simple_app.h"

#include <string>

#include "cefsimple/simple_handler.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#include "utils/XString.h"

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate {
 public:
  explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {
  }

  void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
    // Add the browser view and show the window.
    window->AddChildView(browser_view_);
    window->Show();

    // Give keyboard focus to the browser view.
    browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
    browser_view_ = NULL;
  }

  bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
    // Allow the window to close if the browser says it's OK.
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser)
      return browser->GetHost()->TryCloseBrowser();
    return true;
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

}  // namespace

SimpleApp::SimpleApp() {
}

void SimpleApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

#if defined(OS_WIN) || defined(OS_LINUX)
  // Create the browser using the Views framework if "--use-views" is specified
  // via the command-line. Otherwise, create the browser using the native
  // platform framework. The Views framework is currently only supported on
  // Windows and Linux.
  const bool use_views = command_line->HasSwitch("use-views");
#else
  const bool use_views = false;
#endif

  // SimpleHandler implements browser-level callbacks.
  CefRefPtr<SimpleHandler> handler(new SimpleHandler(use_views));

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;

  std::string url;

  // Check if a "--url=" value was provided via the command-line. If so, use
  // that instead of the default URL.
  url = command_line->GetSwitchValue("url");
  if (url.empty()) {
	  url = "file:///index.html";
  }

  handler->mInjectJsUrls = command_line->GetSwitchValue("inject");

  if (use_views) {
    // Create the BrowserView.
    CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
        handler, url, browser_settings, NULL, NULL);

    // Create the Window. It will show itself after creation.
    CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
  } else {
    // Information used when creating the native window.
    CefWindowInfo window_info;

#if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    window_info.SetAsPopup(NULL, "cefsimple");
#endif

    // Create the first browser window.
    CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings,
                                  NULL);
  }
}

static void adjustCommandLine(CefRefPtr<CefCommandLine> command_line) {
	command_line->AppendSwitch(L"disable-web-security");
	command_line->AppendSwitchWithValue(L"remote_debugging_port ", L"8085");
	// command_line->AppendSwitch(L"persist_session_cookies");
	command_line->AppendSwitch(L"ignore-certificate-errors");
	command_line->AppendSwitch(L"allow-external-pages");
	command_line->AppendSwitch(L"allow-file-access-from-files");
	command_line->AppendSwitch(L"allow-http-background-page");
	command_line->AppendSwitch(L"allow-outdated-plugins");
	command_line->AppendSwitch(L"allow-running-insecure-content");
}

void SimpleApp::OnBeforeCommandLineProcessing( const CefString& process_type, CefRefPtr<CefCommandLine> command_line ) {
	adjustCommandLine(command_line);
}

ClientAppRenderer::ClientAppRenderer() {
}

extern CefRefPtr<CefV8Value> CreateCallNativeFunction();

void ClientAppRenderer::OnContextCreated( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context )
{
	printf("ClientAppRenderer.OnContextCreated() \n");
	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();
	
	CefRefPtr<CefV8Value> func2 = CreateCallNativeFunction();
	object->SetValue("callNative", func2, V8_PROPERTY_ATTRIBUTE_NONE);
}

extern void RegisterZipCode();
extern void RegisterFileCode();
extern void RegisterBufferCode();
extern void RegisterComCode();
extern void RegisterDBCode();

void ClientAppRenderer::OnWebKitInitialized()
{
	printf("ClientAppRenderer.OnWebKitInitialized() \n");

	RegisterZipCode();
	RegisterFileCode();
	RegisterBufferCode();
	RegisterComCode();
	RegisterDBCode();
}

void ClientAppRenderer::OnUncaughtException( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace )
{
	CefString urlw = frame->GetURL();
	CefString msgw = exception->GetMessage();
	CefString srcw = exception->GetSourceLine();
	char *url = XString::unicodeToGbk(urlw.c_str());
	char *msg = XString::unicodeToGbk(msgw.c_str());
	char *src = XString::unicodeToGbk(srcw.c_str());

	printf("%s\n", msg);
	for (int i = 0; i < stackTrace->GetFrameCount() && i < 5; ++i) {
		CefRefPtr<CefV8StackFrame> f = stackTrace->GetFrame(i);
		CefString funcw = f->GetFunctionName();
		CefString scriptw = f->GetScriptName();
		char *func = XString::unicodeToGbk(funcw.c_str());
		char *script = XString::unicodeToGbk(scriptw.c_str());
		printf("   at %s ", func);
		printf("(%s:%d)  \n", script, f->GetLineNumber());
		free(func);
		free(script);
	}
	
	free(url);
	free(msg);
	free(src);
}

void ClientAppRenderer::OnBeforeCommandLineProcessing( const CefString& process_type, CefRefPtr<CefCommandLine> command_line ) {
	adjustCommandLine(command_line);
}



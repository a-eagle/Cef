#pragma once
#include "include/cef_v8.h"

bool WrapOpenFile(CefRefPtr<CefV8Value>& retval, CefString path);
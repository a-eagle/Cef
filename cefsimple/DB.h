#pragma once
#include <string>
#include "include/cef_v8.h"
#include "db/SqlDriver.h"


std::string GetDbExtention();

CefRefPtr<CefV8Value> WrapDBDriver(SqlConnection *con);


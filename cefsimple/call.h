#pragma once

enum CallType {
	CALL_TYPE_UNDEFIND = -1,
	CALL_TYPE_VOID =    0,        // v
	CALL_TYPE_CHAR =    (1 << 0), // c
	CALL_TYPE_INT =     (1 << 1), // i
	CALL_TYPE_POINTER = (1 << 2), // p
	CALL_TYPE_STRING =  (1 << 3), // s
	CALL_TYPE_WSTRING = (1 << 4), // w

	// Note: bellow types are not support
	CALL_TYPE_DOUBLE =  (1 << 5), // d
	CALL_TYPE_FLOAT =   (1 << 6), // f

	CALL_TYPE_ARRAY =   (1 << 8)  // [
};

bool GetCallInfo(const char *funcDesc, int *paramNum, CallType *params, CallType *ret);

bool Call(const char *funcName, const char *funcDesc, void **params, int paramsCount, int *ret);
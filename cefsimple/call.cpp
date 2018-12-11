#include <stdlib.h>
#include <windows.h>
#include <stdio.h>

enum CallType {
	CALL_TYPE_UNDEFIND = -1,
	CALL_TYPE_VOID =    0,        // v
	CALL_TYPE_CHAR =    (1 << 0), // c
	CALL_TYPE_INT =     (1 << 1), // i
	CALL_TYPE_POINTER = (1 << 2), // p
	CALL_TYPE_STRING =  (1 << 3), // s
	CALL_TYPE_WSTRING = (1 << 4), // w

	CALL_TYPE_DOUBLE =  (1 << 5), // d
	CALL_TYPE_FLOAT =   (1 << 6), // f

	CALL_TYPE_ARRAY =   (1 << 8)  // [
};

static CallType GetCallType(char c) {
	switch (c) {
	case 'v': return CALL_TYPE_VOID;
	case 'c': return CALL_TYPE_CHAR;
	case 'i': return CALL_TYPE_INT;
	case 'd': return CALL_TYPE_DOUBLE;
	case 'f': return CALL_TYPE_FLOAT;
	case 'p': return CALL_TYPE_POINTER;
	case 's': return CALL_TYPE_STRING;
	case 'w': return CALL_TYPE_WSTRING;
	case '[': return CALL_TYPE_ARRAY;
	}
	return CALL_TYPE_UNDEFIND;
}

#define  CHECK_TYPE(t) if (t == CALL_TYPE_UNDEFIND) {return false;}

static bool GetDescInfo(const char *funcDesc, int *paramNum, CallType *params, CallType *ret) {
	char tmp[64] = {0};
	char *p = tmp;
	while (*funcDesc != 0) {
		if (*funcDesc != ' ' && *funcDesc != '\t') {
			*p = *funcDesc;
			++p;
		}
		++funcDesc;
	}
	p = tmp;

	// parse return type
	CallType t = GetCallType(*p++);
	CHECK_TYPE(t);
	*ret = t;
	if (t == CALL_TYPE_ARRAY) {
		CallType t2 = GetCallType(*p++);
		CHECK_TYPE(t2);
		*ret = CallType(*ret | t2);
	}

	// parse params
	int pm = 0;
	if (*p++ != '(') {
		return false;
	}
	while (*p) {
		t = GetCallType(*p++);
		CHECK_TYPE(t);
		if (t == CALL_TYPE_VOID) {
			return false;
		}
		params[pm] = t;
		if (t == CALL_TYPE_ARRAY) {
			CallType t2 = GetCallType(*p++);
			CHECK_TYPE(t2);
			params[pm] = CallType(t | t2);
		}
		++pm;
		if (*p == ')') {
			break;
		}
		if (*p != ',') {
			return false;
		}
		++p;
	}
	*paramNum = pm;
	return true;
}

bool Call(const char *funcName, const char *funcDesc, void **params, int *ret) {
	static HMODULE kernalModule = GetModuleHandleA("kernel32.dll");
	static HMODULE userModule = GetModuleHandleA("user32.dll");

	int rr = NULL;
	if (funcDesc == NULL || strlen(funcDesc) == 0) {
		return false;
	}

	int paramNum = 0;
	CallType paramsInfo[10];
	CallType retInfo;

	if (! GetDescInfo(funcDesc, &paramNum, paramsInfo, &retInfo)) {
		return false;
	}

	void *addr = (void *)GetProcAddress(kernalModule, funcName);
	if (addr == NULL) {
		addr = (void *)GetProcAddress(userModule, funcName);
	}
	if (addr == NULL) {
		return false;
	}

	// push params
	int v = 0;
	for (int i = paramNum - 1; i >= 0; --i) {
		if (paramsInfo[i] <= CALL_TYPE_WSTRING) {
			v = (int)params[i];
			__asm push v
		}
	}
	__asm call addr;
	__asm mov rr, eax
	if (ret != NULL && retInfo != CALL_TYPE_VOID) {
		*ret = rr;
	}
	
	return true;
}

int main(int argc, char* argv[]) {
	int rr = -1;
	void *params[] = {NULL, "My Text AA", "My Caption AA", (void *)1};
	bool r = Call("MessageBoxA", "i(p, s, s, i)", params, &rr);

	printf("r = %d, rr = %d \n", r, rr);
	// MessageBoxA(NULL, "My Text", "My Title", MB_OK);
	return 0;
}
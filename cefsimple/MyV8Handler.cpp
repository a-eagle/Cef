#include "MyV8Handler.h"
#include "db/SqlDriver.h"
#include "utils/XString.h"
#include "DB.h"
#include "FileHandler.h"

MyV8Handler::MyV8Handler()
{
}

bool MyV8Handler::Execute( const CefString& name, CefRefPtr<CefV8Value> object, 
	const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception )
{
	static SqlConnection *sCon = NULL;
	// OpenDBDriver(url, userName, password)
	if (name == "openDBDriver") {
		printf("MyV8Handler::Execute -> openDBDriver \n");
		if (sCon != NULL && !sCon->isClosed()) {
			retval = WrapDBDriver(sCon);
			return true;
		}
		if (arguments.size() >= 1) {
			CefString url = arguments[0]->GetStringValue();

			CefString userName, password;
			if (arguments.size() >= 2) {
				userName = arguments[1]->GetStringValue();
			}
			if (arguments.size() >= 3) {
				password = arguments[2]->GetStringValue();
			}
			char *urlStr = (char *)XString::toBytes((void *)url.c_str(), XString::UNICODE2, XString::GBK);
			char *userNameStr = (char *)XString::toBytes((void *)userName.c_str(), XString::UNICODE2, XString::GBK);
			char *passwordStr = (char *)XString::toBytes((void *)password.c_str(), XString::UNICODE2, XString::GBK);
			
			printf("openDBDriver(%s, %s, %s) \n", urlStr, userNameStr, passwordStr);

			sCon = SqlConnection::open(urlStr, userNameStr, passwordStr);
			if (sCon == NULL) {
				printf("openDBDriver fail \n");
				return false;
			}
			retval = WrapDBDriver(sCon);
			printf("openDBDriver success \n");
			return true;
		}
		return false;
	}
	if (name == "callNative") {
		if (arguments.size() < 1) {
			return false;
		}
		return callNative(object, arguments, retval, exception);
	}
	

	return false;
}

bool MyV8Handler::callNative(CefRefPtr<CefV8Value> object, 
	const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception ) {
	CefString funcName = arguments[0]->GetStringValue();
	
	// String readFile(String path, [String options]) options: base64 
	if (funcName == "readFile") {
		
	}
	
	return false;
}

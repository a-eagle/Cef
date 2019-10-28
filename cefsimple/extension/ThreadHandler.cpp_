#include "include/cef_v8.h"
#include <string>
#include <windows.h>
#include <stdlib.h>
#include <vector>
#include "utils/XString.h"

class ThreadV8Handler : public CefV8Handler {
public:
	ThreadV8Handler() {}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE;


	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(ThreadV8Handler);
};

class NTask {
public:
	CefRefPtr<CefV8Value> mFunc;
	CefRefPtr<CefV8Value> mParams;
	DWORD mExecTime;
	CefRefPtr<CefV8Context> mCurCtx;

	void run();
};

void NTask::run() {
	if (! mCurCtx->IsValid()) {
		printf("NTask run fail: context is invalid\n");
		return;
	}
	if (! mFunc->IsFunction()) {
		printf("NTask run fail: not a function\n");
		return;
	}
	CefV8ValueList args;
	if (mParams != NULL && mParams->IsArray()) {
		for (int i = 0; i < mParams->GetArrayLength(); ++i) {
			args.push_back(mParams->GetValue(i));
		}
	}
	mFunc->ExecuteFunctionWithContext(mCurCtx, NULL, args);
}

class NThread {
public:
	NThread(const wchar_t *name);
	wchar_t *getName();
	void post(NTask *t);
	void start();
	void run();

	static NThread *find(const wchar_t *name);
	static bool add(NThread *t);
	static bool canNew() {return mThreadNum < 10;}
protected:
	wchar_t mName[50];
	std::vector<NTask*> mTasks;
	HANDLE mHandle, mEvent;
	DWORD mThreadId;
	BOOL mRunning;
	static NThread* s_threads[10];
	static int mThreadNum;
};

NThread* NThread::s_threads[10];
int NThread::mThreadNum = 0;

NThread::NThread( const wchar_t *name ) {
	wcsncpy(mName, name, 50);
	mHandle = INVALID_HANDLE_VALUE;
	mEvent = INVALID_HANDLE_VALUE;
	mThreadId = 0;
	mRunning = TRUE;
}

wchar_t * NThread::getName() {
	return mName;
}

DWORD WINAPI PTHREAD_START_ROUTINE (LPVOID lpThreadParameter) {
	NThread *thread = static_cast<NThread*>(lpThreadParameter);
	thread->run();
}

void NThread::start() {
	if (mHandle != INVALID_HANDLE_VALUE) {
		return;
	}
	mEvent = CreateEvent(NULL, FALSE, FALSE, mName);
	mHandle = CreateThread(NULL, 0, PTHREAD_START_ROUTINE, this, 0, &mThreadId);
}

void NThread::run() {
	while (mRunning) {
		if (mTasks.size() == 0) {
			WaitForSingleObject(mEvent, INFINITE);
		}
		int curIdx = -1;
		for (int i = 0; i < mTasks.size(); ++i) {
			NTask *t = mTasks[i];
			if (t->mExecTime <= GetTickCount()) {
				curIdx = i;
				break;
			}
		}
		if (curIdx == -1) {
			Sleep(50);
		} else {
			// exec task
			NTask *t = mTasks[curIdx];
			mTasks.erase(mTasks.begin() + curIdx);
			t->run();
		}
	}
}

NThread * NThread::find( const wchar_t *name ) {
	if (name == NULL) {
		return NULL;
	}
	for (int i = 0; i < mThreadNum; ++i) {
		if (wcscmp(name, s_threads[i]->getName())) {
			return s_threads[i];
		}
	}
	return NULL;
}

bool NThread::add( NThread *t ) {
	if (mThreadNum >= 10) {
		return false;
	}
	s_threads[mThreadNum++] = t;
	return true;
}

void NThread::post(NTask *t) {
	mTasks.push_back(t);
	SetEvent(mEvent);
}

bool ThreadV8Handler::Execute( const CefString& name, CefRefPtr<CefV8Value> obj, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception )
{
	CefRefPtr<CefV8Value> object = arguments[0];
	CefRefPtr<CefV8Value> nameV = object->GetValue("_name");
	if (nameV == NULL || nameV->IsNull() || !nameV->IsString()) {
		return false;
	}
	CefString cname = nameV->GetStringValue();
	const wchar_t *threadName = cname.c_str();

	NThread *thread = NThread::find(threadName);
	if (thread == NULL) {
		if (! NThread::canNew()) {
			return false;
		}
		thread = new NThread(threadName);
		NThread::add(thread);
	}

	if (name == "Thread_post" && arguments.size() >= 2) {
		NTask *t = new NTask();
		t->mCurCtx = CefV8Context::GetCurrentContext();
		t->mExecTime = GetTickCount();
		t->mFunc = arguments[1];
		if (arguments.size() == 3) {
			t->mParams = arguments[2];
		} else {
			t->mParams = NULL;
		}
		thread->post(t);
		return true;
	}
	if (name == "Thread_postDelay" && arguments.size() >= 3) {
		NTask *t = new NTask();
		t->mCurCtx = CefV8Context::GetCurrentContext();
		t->mExecTime = GetTickCount();
		if (arguments[1]->IsInt()) {
			t->mExecTime += arguments[1]->GetIntValue();
		}
		
		t->mFunc = arguments[2];
		if (arguments.size() == 3) {
			t->mParams = arguments[3];
		} else {
			t->mParams = NULL;
		}
		thread->post(t);
		return true;
	}

	return false;
}

void RegisterThreadCode() {
	std::string code = 
		"function NThread(name) {"
		"	if (name) this._name = '' + name; else this._name = 'NativeThread';"
		"};\n"
		"NThread.prototype.post = function(task, params) {native function Thread_post(); Thread_post(this, task, params);};\n"
		"NThread.prototype.postDelay = function(delayTime, task, params) {native function Thread_postDelay(); Thread_postDelay(this, delayTime, task, params);};\n"
		"\n";

	CefRegisterExtension("v8/NativeThread", code, new ThreadV8Handler());
}
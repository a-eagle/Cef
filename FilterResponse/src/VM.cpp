#pragma warning( disable : 4996)

#include "VM.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include  <io.h>
#include <windows.h>
#include <algorithm>

#define  equals(a, b) (strcmp(a, b) == 0)
#define  not_equals(a, b) (strcmp(a, b) != 0)
#define  skip_space(p) while(*p == ' ' || *p == '\t') ++p 

struct Buffer {
	Buffer(int defsize);
	~Buffer();
	void insure(int len);
	void append(const char *str);
	void append(const char *p, int len);
	void append(char ch);
	void reset() {mBuf[0] = 0; mLen = 0;}
	char *last() {return mBuf + mLen;}
	
	int mCapacity;
	int mLen;
	char *mBuf;
};

Buffer::Buffer(int df) {
	mCapacity = df;
	mLen = 0;
	mBuf = (char *)malloc(df);
}

Buffer::~Buffer() {
	if (mBuf != NULL)
		free(mBuf);
	mBuf = NULL;
}

void Buffer::insure(int len) {
	int old = mCapacity;
	while (mCapacity - mLen - 4 < len) {
		mCapacity *= 2;
	}
	if (old < mCapacity) {
		mBuf = (char *)realloc(mBuf, mCapacity);
	}
}

void Buffer::append(const char *str) {
	if (str == NULL)
		return;
	int len = strlen(str);
	insure(len);
	strcpy(mBuf + mLen, str);
	mLen += len;
}

void Buffer::append(const char *p, int len) {
	if (p == NULL || len <= 0)
		return;
	insure(len);
	memcpy(mBuf + mLen, p, len);
	mLen += len;
	mBuf[mLen] = 0;
}

void Buffer::append(char ch) {
	insure(1);
	mBuf[mLen++] = ch;
	mBuf[mLen] = 0;
}

Var::Var() {
	mType = TYPE_STRING;
	mArrayVal = NULL;
}

Var::~Var() {
}

Frame::Frame(FrameType t, char *pos, int line) {
	mType = t;
	mStartPos = pos;
	mStartLineNo = line;
	mCondition = false;
}

static std::string numToStr(double d) {
	static char nn[48];
	sprintf(nn, "%f", d);
	char *pe = (char*)nn + strlen(nn) - 1;
	char *pd = strchr(nn, '.');
	while (pe > pd) {
		if (*pe != '0') break;
		*pe = 0;
		--pe;
	}
	if (pe == pd) *pd = 0;
	return nn;
}

static bool isNum(const char *s) {
	if (s == NULL || strlen(s) == 0)
		return false;
	if (s[0] == '-' || s[0] == '+')
		++s;
	while (*s >= '0' && *s <= '9') ++s;
	if (*s == '.') ++s;
	while (*s >= '0' && *s <= '9') ++s;
	return *s == 0;
}

std::string Var::toString() {
	Buffer buf(512);

	if (mType == TYPE_STRING) {
		return mStrVal;
	}
	if (mType == TYPE_ARRAY) {
		if (mArrayVal == NULL) {
			return "()";
		}
		buf.append("(");
		for (int i = 0; i < mArrayVal->size(); ++i) {
			Var &vi = mArrayVal->at(i);
			if (i != 0) {
				buf.append(", ");
			}
			if (vi.mType == TYPE_STRING) {
				buf.append("\"");
				std::string it = vi.toString();
				buf.append(it.c_str());
				buf.append("\"");
			} else {
				std::string it = vi.toString();
				buf.append(it.c_str());
			}
		}
		buf.append(")");
		return buf.mBuf;
	}

	return "{none}";
}

double Var::toNumber() {
	if (mType == TYPE_STRING) {
		return strtod(mStrVal.c_str(), NULL);
	}
	return 0;
}

VM::VM() {
	mHasError = false;
	mError = NULL;
	mFileName = NULL;
	mFileContent = NULL;
	mFileLen = 0;
	mLine[0] = 0;
	mLineNo = 0;
	mNextLinePos = NULL;
	mCurLinePos = NULL;
	mParent = NULL;
}

VM::VM(const char *filePath) {
	mHasError = false;
	mError = (char *)malloc(512);
	mFileName = (char *)malloc(256);
	mFileName[0] = 0;
	mError[0] = 0;
	mFileContent = NULL;
	mFileLen = 0;
	mLine[0] = 0;
	mLineNo = 0;
	mNextLinePos = NULL;
	mCurLinePos = NULL;
	mParent = NULL;

	if (filePath != NULL) {
		const char *p = strrchr(filePath, '/');
		const char *p2 = strrchr(filePath, '\\');
		if (p != NULL && p2 != NULL) {
			if (p < p2) p = p2;
		} else if (p2 != NULL) {
			p = p2;
		}
		if (p == NULL) {
			strcpy(mFileName, filePath);
		} else {
			strcpy(mFileName, p + 1);
		}
		readFile(filePath);
	} else {
		mHasError = true;
		strcpy(mError, "Create VM Error: file path is null");
	}
}

void VM::setContent(const char *fileName,  char *cnt ) {
	if (cnt == NULL) {
		mHasError = true;
		strcpy(mError, "VM Error: File ");
		strcpy(mError, (fileName == NULL ? "(unknown)" : fileName));
		strcpy(mError, " content is null");
		return;
	}

	mFileContent = cnt;
	mFileLen = strlen(mFileContent);
	mNextLinePos = mFileContent;
	mCurLinePos = mFileContent;
}

VM::~VM() {
	if (mFileContent != NULL && isRootVM()) {
		free(mError);
		free(mFileContent);
	}
	mFileContent = NULL;
	mError = NULL;
}

static void trimRight(char *s) {
	int len = strlen(s);
	if (len == 0) return;
	char *e = s + strlen(s) - 1;
	while (e >= s) {
		if  (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n') {
			*e = 0;
			--e;
		} else {
			break;
		}
	}
}

static bool isEmpty(char *s) {
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		++s;
	return *s == 0;
}

bool VM::run() {
	while (! mHasError) {
		if (! readLine())
			break;
		if (! runLineExp())
			return false;
	}
	return true;
}

bool VM::hasError() {
	return mHasError;
}

char * VM::getError() {
	static char SE[256];
	sprintf(SE, "[%s: Line %d]  %s", mFileName, mLineNo, mError);
	return SE;
}

bool VM::readLine() {
	char *npos = mNextLinePos;
	if (mHasError || mFileLen == 0 || npos > mFileContent + mFileLen) {
		return false;
	}
	char *p = npos;
	char *rp = mLine;

	while (*p == ' ' || *p == '\t') {
		++p;
	}
	while (*p != 0 && *p != '\n') {
		*rp ++ = *p ++;
	}
	*rp = 0;
	if (p > npos && p[-1] == '\r') {
		rp[-1] = 0;
	}
	trimRight(mLine);

	mCurLinePos = mNextLinePos;
	mNextLinePos = p + 1;
	++mLineNo;
	return true;
}

void VM::readFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		mHasError = true;
		sprintf(mError, "VM Error: open file [%s] fail", path);
		return;
	}
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);
	mFileContent = (char *)malloc(len + 1);
	fread(mFileContent, len, 1, f);
	mFileContent[len] = 0;
	fclose(f);
	mFileLen = len;
	mNextLinePos = mFileContent;
	mCurLinePos = mFileContent;
}

// OK
bool VM::runLineExp() {
	char *pos = mLine;
	Token *token = nextToken(&pos);
	Frame *cur = topFrame();

	if (token == NULL)
		return false;
	if (token->mType == TOKEN_EOF)
		return true;
	if (token->mType != TOKEN_NORMAL) {
		mHasError = true;
		strcat(mError, " Not a valid command ");
		return false;
	}
	std::string cmd = token->mToken;

	if (cmd[0] == '#') { // Comment line, skip it
		return true;
	}

	if (cmd == "function") {
		return runFuncDefine(&pos);
	}

	if (cmd == "else") {
		return runElseExp(&pos);
	}
	if (cmd == "endif") {
		return runEndIfExp(&pos);
	}
	if (cmd == "done") {
		return runDoneExp(&pos);
	}

	if (cur != NULL && !cur->mCondition) {
		if (cmd == "if") {
			Frame f(Frame::FRAME_IF, mNextLinePos, mLineNo);
			f.mCondition = false;
			mFrameStack.push_back(f);
		} else if (cmd == "while") {
			Frame f(Frame::FRAME_LOOP, mNextLinePos, mLineNo);
			f.mCondition = false;
			mFrameStack.push_back(f);
		}
		return true;
	}

	if (cmd == "if") {
		Frame f(Frame::FRAME_IF, mCurLinePos, mLineNo);
		mFrameStack.push_back(f);
		return runIfExp(&pos);
	}

	if (cmd == "while") {
		Frame f(Frame::FRAME_LOOP, mCurLinePos, mLineNo);
		mFrameStack.push_back(f);
		return runWhileExp(&pos);
	}

	if (cmd == "continue") {
		return runContinueExp(&pos);
	}

	if (cmd == "set") {
		return runSetExp(&pos);
	}

	if (cmd == "call") {
		return runCallExp(&pos);
	}
	
	// other function cmd here
	return runFunctionExp(cmd, &pos);
}

// when error return NULL
VM::Token* VM::nextToken(char **pos) {
	static Buffer TK(512);
	static Token TOKEN;
	char *p = *pos;

	// skip_space(p);
	while(*p == ' ' || *p == '\t') ++p;
	*pos = p;
	TK.reset();
	if (*p == '"') {
		char *str = NULL;
		if (! exceptString(&str, pos)) {
			return NULL;
		}
		TK.append(str);

		TOKEN.mType = TOKEN_STRING;
		TOKEN.mToken = TK.mBuf;
		return &TOKEN;
	}

	char *ep = p;
	while (*ep != 0 && *ep != ' ' && *ep != '\t') ++ep;
	int len = ep - p;
	TK.append(p, len);
	*pos = ep;

	TOKEN.mType = (len != 0) ? TOKEN_NORMAL : TOKEN_EOF;
	TOKEN.mToken = TK.mBuf;
	return &TOKEN;
}

bool VM::runSetExp( char ** pos ) {
	Token *token = nextToken(pos);
	if (token == NULL)
		return false;
	
	std::string type = token->mToken;
	if ((token->mType == TOKEN_EOF) || (token->mType != TOKEN_NORMAL) || 
		((type != "-s") && (type != "-n") && (type != "-a"))) {
		mHasError = true;
		strcat(mError, " set must need one of -s -n -a  ");
		return false;
	}

	Var *v = new Var();
	char *name = NULL;
	if (! exceptVarName(&name, pos))
		return false;
	v->mName = name;

	if (! exceptEq(pos)) {
		return false;
	}
	
	v->mType = Var::TYPE_STRING;
	if (type == "-s") {
		if (! exceptStringSequnce(&name, pos))
			return false;
		v->mStrVal = name;
		mVarMap[v->mName] = v;
		return true;
	}
	
	if (type == "-n") {
		if (! exceptMathSequnce(&name, pos))
			return false;
		v->mStrVal = name;
		mVarMap[v->mName] = v;
		return true;
	}
	
	if (type == "-a") {
		v->mType = Var::TYPE_ARRAY;
		v->mArrayVal = new std::vector<Var>();
		if (! exceptArraySequnce(v, pos))
			return false;
		mVarMap[v->mName] = v;
		return true;
	}
	return false;
}

bool VM::runIfExp( char ** pos ) {
	bool cond = false;
	if (exceptCondition(pos, &cond, "then")) {
		topFrame()->mCondition = cond;
		return true;
	}

	mHasError = true;
	strcat(mError, " invalid if statement ");
	return false;
}

bool VM::runElseExp(char ** pos) {
	Frame *cur = topFrame();
	Token *pre = nextToken(pos);
	if (pre == NULL)
		return false;
	if (cur == NULL || (cur->mType != Frame::FRAME_IF && cur->mType != Frame::FRAME_ELSE_IF)) {
		goto err;
	}
	if (pre->mType == TOKEN_EOF) {
		// its a else block
		Frame f(Frame::FRAME_ELSE, mCurLinePos, mLineNo);
		f.mCondition = ! checkIfFrameAlreadRuned();
		mFrameStack.push_back(f);
		return true;
	} else if (pre->mType == TOKEN_NORMAL && pre->mToken == "if") {
		// its a else if block
		if (! checkIfFrameAlreadRuned()) {
			// should check condition
			Frame f(Frame::FRAME_ELSE_IF, mCurLinePos, mLineNo);
			mFrameStack.push_back(f);
			return runIfExp(pos);
		} else {
			// no need check condition, NO execute this block
			Frame f(Frame::FRAME_ELSE_IF, mCurLinePos, mLineNo);
			f.mCondition = false;
			mFrameStack.push_back(f);
			return true;
		}
	} 

	err:
	mHasError = true;
	strcat(mError, " invalid else statement ");
	return false;
}

bool VM::runEndIfExp(char **pos) {
	Frame *cur = topFrame();
	int idx = mFrameStack.size() - 1;
	
	// pop else block
	if (idx <= 0) goto err;
	if (mFrameStack[idx].mType == Frame::FRAME_ELSE) {
		popFrame();
		--idx;
	}

	// pop else if block
	if (idx <= 0) goto err;
	for (; idx >= 0; --idx) {
		if (mFrameStack[idx].mType != Frame::FRAME_ELSE_IF) {
			break;
		}
		popFrame();
	}

	// pop if block
	if (idx <= 0) goto err;
	if (mFrameStack[idx].mType == Frame::FRAME_IF) {
		popFrame();
		return true;
	}

	err:
	mHasError = true;
	strcat(mError, "endif not match if statement");
	return false;
}

bool VM::checkIfFrameAlreadRuned() {
	bool runed = false;
	Frame *f = NULL;
	int i = mFrameStack.size() - 1;
	for (; i >= 0; --i) {
		if (mFrameStack[i].mType != Frame::FRAME_ELSE_IF)
			break;
		runed = runed || mFrameStack[i].mCondition;
	}
	if (i < 0 || mFrameStack[i].mType != Frame::FRAME_IF) {
		return false;
	}
	return runed || mFrameStack[i].mCondition;
}

bool VM::isInIfBlockList() {
	Frame *cur = topFrame();
	if (cur == NULL)
		return false;
	return (cur->mType == Frame::FRAME_IF) || 
		(cur->mType == Frame::FRAME_ELSE_IF) || 
		(cur->mType == Frame::FRAME_ELSE);
}

bool VM::runWhileExp(char ** pos) {
	bool cond = false;
	if (exceptCondition(pos, &cond, "do")) {
		topFrame()->mCondition = cond;
		return true;
	}

	mHasError = true;
	strcat(mError, " invalid while statement");
	return false;
}

bool VM::runDoneExp(char **pos) {
	Frame *cur = topFrame();
	if (cur == NULL || cur->mType != Frame::FRAME_LOOP) {
		mHasError = true;
		strcat(mError, " done not match while statement");
		return false;
	}
	if (cur->mCondition) { // loop continue
		mLineNo = cur->mStartLineNo - 1;
		mNextLinePos = cur->mStartPos;
	}
	popFrame();
	return true;
}

bool VM::runContinueExp( char ** pos ) {
	if (! exceptLineEOF(*pos)) {
		return false;
	}
	Frame *cur = NULL;
	while ((cur = topFrame()) != NULL) {
		if (cur->mType == Frame::FRAME_LOOP) {
			break;
		}
		popFrame();
	}
	if (cur == NULL || cur->mType != Frame::FRAME_LOOP) {
		mHasError = true;
		strcpy(mError, " continue must in while block");
		return false;
	}

	mLineNo = cur->mStartLineNo - 1;
	mNextLinePos = cur->mStartPos;
	popFrame();
	return true;
}

bool VM::runFuncDefine(char **pos) {
	char *name = NULL;
	if (! exceptVarName(&name, pos))
		return false;
	FuncDefine *def = new FuncDefine(name);
	if (! exceptLineEOF(*pos))
		return false;
	
	char *start = mNextLinePos;
	char *end = mNextLinePos;
	def->mStartLineNo = mLineNo + 1;

	while (! mHasError) {
		if (! readLine())
			goto _e;
		if (strcmp(mLine, "end") == 0) {
			end = mCurLinePos - 1;
			break;
		}
	}

	if (end >= start) {
		int len = end - start;
		def->mCnt = (char *)malloc(len + 1);
		memcpy(def->mCnt, start, len);
		def->mCnt[len] = 0;
		if (len > 0 && def->mCnt[len - 1] == '\r') {
			def->mCnt[len - 1] = 0;
			--len;
		}
		def->mCntLen = len;
		mFuncDefineMap[def->mName] = def;
	}
	return true;

_e:
	mHasError = true;
	strcat(mError, "Need 'end' for function");
	return false;
}

bool VM::runCallExp(char **pos) {
	char *name = NULL;
	if (! exceptVarName(&name, pos))
		return false;
	if (mFuncDefineMap.find(name) == mFuncDefineMap.end()) {
		mHasError = true;
		strcpy(mError, "Not find function name :");
		strcpy(mError, name);
		return false;
	}
	FuncDefine *def = mFuncDefineMap[name];
	FuncVM *cur = new FuncVM(this, def);

	int idx = 0;
	while (true) {
		Token *arg = nextToken(pos);
		if (arg->mType == TOKEN_EOF)
			break;
		if (! cur->addArg(arg, idx))
			return false;
		++idx;
	}

	if (! cur->run()) {
		return false;
	}
	delete cur;
	return true;
}

bool VM::exceptCondition(char ** pos, bool *ret, std::string endToken) {
	char *oldPos = *pos;
	int ifLine = mLineNo;
	Token *token = nextToken(pos);
	ConditionFlag cf = COND_FLAG_NONE;
	std::string op;
	bool cond = false;
	bool first = true;

	if (token == NULL)
		return false;
	if (token->mType == TOKEN_EOF) {
		return false;
	}
	if (token->mType == TOKEN_NORMAL && token->mToken == "-n") {
		cf = COND_FLAG_MAYBE_NUM;
	} else if (token->mType == TOKEN_NORMAL && token->mToken == "-an") {
		cf = COND_FLAG_ALL_NUM;
	} else {
		// back token
		*pos = oldPos;
	}

	// begin run conditions
	while (true) {
		bool cur = false;
		op = "";
		if (! first) { // check op
			token = nextToken(pos);
			if (token == NULL || token->mType == TOKEN_EOF)
				return false;
			if (token->mType == TOKEN_NORMAL && token->mToken == endToken) {
				if (! exceptLineEOF(*pos))
					return false;
				break;
			}
			if (token->mType == TOKEN_NORMAL && (token->mToken == "and" || token->mToken == "or"))
				op = token->mToken;
			else
				return false;
		}
		if (! exceptConditionPart(pos, cf, &cur))
			return false;
		if (first) {
			cond = cur;
		} else {
			if (op == "and") 
				cond = cond && cur;
			else if (op == "or") 
				cond = cond || cur;
		}
		first = false;
	}
	*ret = cond;
	return true;
}

bool VM::exceptConditionPart(char ** pos, ConditionFlag cf, bool *ret) {
	Token *token = NULL;
	std::string opts[3]; // left op right
	
	int PART_NUM = 3;
	for (int i = 0; i < PART_NUM; ++i) {
		token = nextToken(pos);
		if (token == NULL)
			return false;
		if (token->mType == TOKEN_EOF)
			goto err;
		if (i == 0 && token->mType == TOKEN_NORMAL && token->mToken == "exist") {
			PART_NUM = 2;
		}
		if (token->mType == TOKEN_NORMAL && 
			(token->mToken == "and" || token->mToken == "or" || token->mToken ==  "then")) {
			goto err;
		}
		Var *v = evalToken(token);
		if (v == NULL)
			return false;
		opts[i] = v->toString();
	}

	// use number compare
	double left = strtod(opts[0].c_str(), NULL);
	double right = strtod(opts[2].c_str(), NULL);
	bool nn = isNum(opts[0].c_str()) && isNum(opts[2].c_str());

	if (cf == COND_FLAG_ALL_NUM && ! nn) {
		mHasError = true;
		strcat(mError, " all operators must be a number ");
		return false;
	}
	bool useNum = cf == COND_FLAG_ALL_NUM || (cf == COND_FLAG_MAYBE_NUM && nn);
	if (opts[1] == "==") {
		*ret = useNum ? (left == right) : (opts[0] == opts[2]);
		return true;
	}
	if (opts[1] == "!=") {
		*ret = useNum ? (left != right) : (opts[0] != opts[2]);
		return true;
	}
	if (opts[1] == ">") {
		*ret = useNum ? (left > right) : (opts[0] > opts[2]);
		return true;
	}
	if (opts[1] == ">=") {
		*ret = useNum ? (left >= right) : (opts[0] >= opts[2]);
		return true;
	}
	if (opts[1] == "<") {
		*ret = useNum ? (left < right) : (opts[0] < opts[2]);
		return true;
	}
	if (opts[1] == "<=") {
		*ret = useNum ? (left <= right) : (opts[0] <= opts[2]);
		return true;
	}
	if (opts[0] == "exist") {
		*ret = _access(opts[1].c_str(), 0) == 0;
		return true;
	}
	if (opts[0] == "not" && opts[1] == "exist") {
		*ret = _access(opts[2].c_str(), 0) != 0;
		return true;
	}

	err:
	mHasError = true;
	strcat(mError, "invalid condition statement ");
	return false;
}

bool VM::runFunctionExp(std::string cmd, char ** pos ) {
	std::vector<Token> args;

	while (true) {
		Token *token = nextToken(pos);
		if (token == NULL)
			return false;
		if (token->mType == TOKEN_EOF)
			break;
		args.push_back(*token);
	}

	return execFunction(cmd, args);
}

static bool isVarName(std::string sname) {
	const char *name = sname.c_str();
	bool az = (name[0] >= 'a' && name[0] <= 'z');
	bool AZ = (name[0] >= 'A' && name[0] <= 'Z');
	if (!az && !AZ && (name[0] != '_')) {
		return false;
	}
	const char *p = name + 1;
	while (*p) {
		az = (*p >= 'a' && *p <= 'z');
		AZ = (*p >= 'A' && *p <= 'Z');
		bool n = *p >= '0' && *p <= '9';
		if (!az && !AZ && !n && (*p != '_')) {
			return false;
		}
		++p;
	}
	return true;
}

// OK
bool VM::exceptVarName( char **retName, char **pos ) {
	Token *token = nextToken(pos);
	
	if (token == NULL)
		return false;
	
	if (token->mType == TOKEN_EOF || token->mToken == "") {
		mHasError = true;
		strcpy(mError, "Need var name");
		return false;
	}
	// check var name is valid
	if (token->mType == TOKEN_NORMAL && isVarName(token->mToken)) {
		*retName = (char *)token->mToken.c_str();
		return true;
	}

	mHasError = true;
	const char *info = (token->mType == TOKEN_STRING) ? "(it's a 'string')" : "";
	sprintf(mError + strlen(mError), " Invalid var name: %s %s ", token->mToken.c_str(), info);
	return false;
}

// OK
bool VM::exceptString( char ** str, char ** pos ) {
	static Buffer TS(512);
	char *npos = *pos;
	// skip space
	char *p = *pos;
	while (*p == ' ' || *p == '\t') ++p;

	if (*p != '"') {
		mHasError = true;
		strcat(mError, "Except a string ");
		return false;
	}
	++p; // skip "
	int i = 0;
	TS.reset();
	while (*p != 0 && *p != '"') {
		if (*p == '\\') {
			if (p[1] == 'r') {
				i++;
				TS.append('\r');
				++p;
			} else if (p[1] == 'n') {
				i++;
				++p;
				TS.append('\n');
			} else if (p[1] == 't') {
				i++;
				TS.append('\t');
				++p;
			} else if (p[1] == '"') {
				i++;
				TS.append('"');
				++p;
			} else if (p[1] == '\\') {
				i++;
				TS.append('\\');
				++p;
			} else {
				i++;
				TS.append(*p);
			}
		} else {
			i++;
			TS.append(*p);
		}
		++p;
	}
	
	if (*p == 0) {
		mHasError = true;
		strcat(mError, "Except a string end \"  ");
		return false;
	}
	// here *p is "
	*pos = p + 1;
	*str = TS.mBuf;
	return true;
}

// OK
bool VM::exceptStringSequnce( char ** str, char ** pos ) {
	static Buffer SS(512);
	char *npos = *pos;

	SS.reset();
	while (true) {
		Token *token = nextToken(&npos);
		if (token == NULL)
			return false;
		if (token->mType == TOKEN_EOF)
			break;

		Var *v = evalToken(token);
		if (v == NULL)
			return false;
		
		if (v->mType == Var::TYPE_STRING) {
			SS.append(v->mStrVal.c_str());
		} else {
			std::string ts = v->toString();
			SS.append(ts.c_str());
		}
	}
	*pos = npos;
	*str = SS.mBuf;
	return true;
}

struct MathItem {
	MathItem() {mLevel = 0; mOperator = 0; mVal = 0;}
	MathItem(char lev, char op, float val) {
		mLevel = lev;
		mOperator = op;
		mVal = val;
	}
	char mLevel; // 0: number  1: + -  2: * / %
	char mOperator;
	float mVal;
};


bool VM::execMathSequnce( void *vmi, int *num ) {
	MathItem *items = (MathItem *)vmi;
	int itemsNum = *num;

	int lrNum = 0, opNum = 0;
	MathItem *lr[40];
	MathItem *ops[40];

	for (int i = 0; i <= itemsNum; ++i) {
		MathItem *m = NULL;
		if (i < itemsNum) {
			m = &items[i];
			if (m->mLevel == 0 && i < itemsNum) { // is number
				lr[lrNum++] = m;
				continue;
			}
			if (opNum == 0 || ops[opNum - 1]->mLevel < m->mLevel) {
				ops[opNum++] = m;
				continue;
			}
		}

		if (opNum == 0 || lrNum == 1)
			break;

		switch (ops[opNum - 1]->mOperator) {
		case '+':
			lr[lrNum - 2]->mVal +=  lr[lrNum - 1]->mVal;
			break;
		case '-':  
			lr[lrNum - 2]->mVal -=  lr[lrNum - 1]->mVal;
			break;
		case '*':
			lr[lrNum - 2]->mVal *=  lr[lrNum - 1]->mVal;
			break;
		case '/':
			if (lr[lrNum - 1]->mVal == 0)
				goto _div0;
			lr[lrNum - 2]->mVal /=  lr[lrNum - 1]->mVal;
			break;
		case '%':
			if (lr[lrNum - 1]->mVal == 0)
				goto _rem0;
			lr[lrNum - 2]->mVal = (int)(lr[lrNum - 2]->mVal) % (int)(lr[lrNum - 1]->mVal);
			break;
		}

		ops[opNum - 1] = m;
		if (i >= itemsNum) {
			--opNum;
		}
		lr[lrNum - 1] = NULL;
		--lrNum;
	}

	if (lrNum != opNum + 1) {
		goto _err;
	}
	for (int i = 0; i < opNum; ++i) {
		items[2 * i] = *lr[i];
		items[2 * i + 1] = *ops[i];
	}
	items[opNum + lrNum - 1] = *lr[lrNum - 1];
	
	for (int i = lrNum + opNum; i < itemsNum; ++i) {
		items[i] = MathItem();
	}
	*num = lrNum + opNum;
	return true;

_err:
	mHasError = true;
	strcat(mError, "Not math operator and numbers ");
	return false;
_div0:
	mHasError = true;
	strcat(mError, "math sequence div 0 ");
	return false;
_rem0:
	mHasError = true;
	strcat(mError, "math sequence remainder 0 ");
	return false;
}

bool VM::exceptMathSequnce( char ** str, char ** pos ) {
	static char RET[128];
	MathItem items[50];
	int itemsNum = 0;

	while(true) {
		Token *token = nextToken(pos);
		if (token == NULL)
			goto _err;
		if (token->mType == TOKEN_EOF) {
			break;
		}
		Var *cur =  evalToken(token);
		if (cur == NULL)
			goto _err;
		if (cur->mType != Var::TYPE_STRING)
			goto _err;
		const char *p = cur->mStrVal.c_str();
		
		if (itemsNum == 0 && equals(p, "-")) {
			items[0] = MathItem(0, 0, 0);
			items[1] = MathItem(1, '-', 0);
			itemsNum = 2;
			continue;
		}
		char level = 0; // 0: number  1: + -  2: * / %

		if (equals(p, "*") || equals(p, "/") || equals(p, "%")) {
			if (itemsNum > 0 && items[itemsNum - 1].mLevel != 0) 
				goto _err;
			items[itemsNum++] = MathItem(2, *p, 0);
		} else if (equals(p, "+") || equals(p, "-")) {
			if (itemsNum > 0 && items[itemsNum - 1].mLevel != 0) 
				goto _err;
			items[itemsNum++] = MathItem(1, *p, 0);
		} else {
			if (! isNum(p)) {
				mHasError = true;
				strcat(mError, "math sequence error: ");
				strcat(mError, token->mToken.c_str());
				strcat(mError, " not a number ");
				return false;
			}
			if (itemsNum > 0 && items[itemsNum - 1].mLevel == 0) 
				goto _err;
			items[itemsNum++] = MathItem(0, 0, (float)strtod(p, NULL));
		}
	}

	if (itemsNum == 0) {
		strcpy(RET, "0");
		*str = RET;
		return true;
	}

	while (itemsNum != 1) {
		if (! execMathSequnce(items, &itemsNum))
			return false;
	}

	float dd = (float)items[0].mVal;
	sprintf(RET, "%f", dd);
	int len = strlen(RET);
	for (int i = len - 1; i > 0; --i) {
		if (RET[i] == '.') {
			RET[i] = 0;
			break;
		}
		if (RET[i] == '0') {
			RET[i] = 0;
		} else {
			break;
		}
	}
	*str = RET;
	return true;

_err:
	mHasError = true;
	strcat(mError, " Not math operator and numbers ");
	return false;
}

bool VM::exceptArraySequnce(Var *arr, char ** pos ) {
	Token *token = nextToken(pos);

	if (token == NULL || token->mType != TOKEN_NORMAL || (token->mToken != "(")) {
		goto _fail;
	}
	
	while (true) {
		token = nextToken(pos);
		Var *v = evalToken(token);
		if (token == NULL || v == NULL || token->mType == TOKEN_EOF)
			goto _fail;

		if (token->mType == TOKEN_NORMAL && (token->mToken == ")")) {
			if (! exceptLineEOF(*pos))
				goto _fail;
			return true;
		}
		arr->mArrayVal->push_back(*v);
	}

_fail:
	mHasError = true;
	strcat(mError, "array sequence error ");
	return false;
}

bool VM::exceptLineEOF( char *pos ) {
	while (*pos == ' ' || *pos == '\t') ++pos;
	if (*pos == 0) {
		return true;
	}
	mHasError = true;
	sprintf(mError + strlen(mError), "Invalid token: %s", pos);
	return false;
}

// OK
bool VM::exceptEq( char ** pos ) {
	char *npos = *pos;
	TokenType tt = TOKEN_NORMAL;
	Token *token = nextToken(&npos);
	if (token == NULL || token->mType != TOKEN_NORMAL || token->mToken != "=") {
		mHasError = true;
		strcat(mError, "Except token =");
		return false;
	}
	*pos = npos;
	return true;
}

#define EF_CHECK_ARGS_NUM(a)  if (args.size() < a) {mHasError = true; strcat(mError, "args number error"); return false;}
#define EF_CHECK_ARG_VAR(idx)   if (args[idx].mType != TOKEN_NORMAL || !isVarName(args[idx].mToken)) {mHasError = true; sprintf(mError + strlen(mError), "args[%d] must is a valid var name", idx); return false;}

bool VM::execFunction( std::string &cmd, std::vector<Token> &args ) {
	if (cmd == "echo") {
		return execFunction_echo(args);
	}
	if (cmd == "length") {
		return execFunction_length(args);
	}
	if (cmd == "append") {
		return execFunction_append(args);
	}
	if (cmd == "at") {
		return execFunction_at(args);
	}
	if (cmd == "process") {
		return execFunction_process(args);
	}
	if (cmd == "file") {
		return execFunction_file(args);
	}
	if (cmd == "split") {
		return execFunction_split(args);
	}
	if (cmd == "join") {
		return execFunction_join(args);
	}
	if (cmd == "search") {
		return execFunction_search(args);
	}
	if (cmd == "sort") {
		return execFunction_sort(args);
	}
	if (cmd == "substr") {
		return execFunction_substr(args);
	}
	if (cmd == "strchr") {
		return execFunction_strchr(args, true);
	}
	if (cmd == "strrchr") {
		return execFunction_strchr(args, false);
	}
	if (cmd == "pause") {
		system("pause");
		return true;
	}

	mHasError = true;
	sprintf(mError + strlen(mError), "Unknown function : %s ", cmd.c_str());
	return false;
}

bool VM::execFunction_echo(std::vector<Token> &args) {
	for (int i = 0; i < args.size(); ++i) {
		Var *v = evalToken(& args[i]);
		if (v != NULL) {
			std::string sv = v->toString();
			printf("%s", sv.c_str());
		}
	}
	printf("\n");
	return true;
}

bool VM::execFunction_length(std::vector<Token> &args) {
	EF_CHECK_ARGS_NUM(2);
	EF_CHECK_ARG_VAR(0);

	int len = 0;
	Var *v0 = evalToken(&args[1]);
	if (v0 == NULL)
		return false;

	if (v0->mType == Var::TYPE_STRING) {
		const char *st = v0->mStrVal.c_str();
		if (st != NULL) len = strlen(st);
	} else if (v0->mType == Var::TYPE_ARRAY) {
		if (v0->mArrayVal != NULL)
			len = v0->mArrayVal->size();
	}
	Var *v = new Var();
	v->mName = args[0].mToken;
	v->mType = Var::TYPE_STRING;
	char tmp[24];
	sprintf(tmp, "%d", len);
	v->mStrVal = tmp;
	mVarMap[v->mName] = v;
	return true;
}

bool VM::execFunction_append(std::vector<Token> &args) {
	EF_CHECK_ARGS_NUM(2);

	Var *v0 = evalToken(&args[0]);
	if (v0 == NULL || v0->mType != Var::TYPE_ARRAY) {
		mHasError = true;
		strcat(mError, "args[0] must be a exist array var name");
		return false;
	}

	if (v0->mArrayVal == NULL)
		v0->mArrayVal = new std::vector<Var>();

	for (int i = 1; i < args.size(); ++i) {
		Var *v = evalToken(& args[i]);
		if (v == NULL)
			return false;
		if (v->mType == Var::TYPE_ARRAY) {
			std::vector<Var> *sub = v->mArrayVal;
			for (int j = 0; sub != NULL && j < sub->size(); ++j) {
				v0->mArrayVal->push_back(sub->at(j));
			}
		} else {
			v0->mArrayVal->push_back(*v);
		}
	}

	return true;
}

bool VM::execFunction_at(std::vector<Token> &args) {
	EF_CHECK_ARGS_NUM(3);
	EF_CHECK_ARG_VAR(0);

	Var *v1 = evalToken(& args[1]);
	Var *v2 = evalToken(& args[2]);
	if (v1 == NULL || v2 == NULL)
		return false;
	
	int idx = 0;
	if (v2->mType != Var::TYPE_STRING || !isNum(v2->mStrVal.c_str())) {
		mHasError = true;
		strcat(mError, "args[2] must be an number ");
		return false;
	}
	idx = (int)strtod(v2->mStrVal.c_str(), NULL);

	Var *ret = new Var();
	if (v1->mType == Var::TYPE_STRING) {
		ret->mType = Var::TYPE_STRING;
		if (idx < 0) {
			idx += v1->mStrVal.length();
		}
		if (idx >= 0 && idx < v1->mStrVal.length()) {
			ret->mStrVal = v1->mStrVal.at(idx);
		}
	} else if (v1->mType == Var::TYPE_ARRAY) {
		std::vector<Var> *arr = v1->mArrayVal;
		if (idx < 0) {
			idx += arr->size();
		}
		if (idx >=0 && idx < arr->size()) {
			*ret = arr->at(idx);
		}
	}
	ret->mName = args[0].mToken;
	mVarMap[ret->mName] = ret;
	return true;
}

bool VM::execFunction_process(std::vector<Token> &args) {
	Buffer params(1024);
	SHELLEXECUTEINFO see = {0};
	see.cbSize = sizeof(SHELLEXECUTEINFO);
	Var *v = evalToken(& args[0]);
	if (v == NULL)
		return false;
	std::string path = v->toString();
	see.lpFile = path.c_str();
	for (int i = 1; i < args.size(); ++i) {
		v = evalToken(& args[i]);
		if (v == NULL)
			return false;
		std::string va = v->toString();
		const char *p = va.c_str();
		bool hasEd = strchr(p, ' ') != NULL || strchr(p, '\t') != NULL;
		if (hasEd)
			params.append("\"");
		params.append(va.c_str());
		if (hasEd)
			params.append("\"");
		params.append(" ");
	}
	see.lpParameters = params.mBuf;
	see.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;

	if (ShellExecuteEx(&see)) {
		WaitForSingleObject(see.hProcess, INFINITE);
		CloseHandle(see.hProcess);
		return true;
	}

	mHasError = true;
	DWORD ec = GetLastError();
	sprintf(mError + strlen(mError), " Error code:%d ", ec);
	switch (ec) {
	case SE_ERR_FNF: strcat(mError, "file not found");break;
	case SE_ERR_PNF: strcat(mError, "path not found");break;
	case SE_ERR_ACCESSDENIED: strcat(mError, "access denied");break;
	case SE_ERR_DLLNOTFOUND: strcat(mError, "dll not found");break;
	}
	return false;
}

bool VM::execFunction_file(std::vector<Token> &args) {
	if (args[0].mToken == "-read") {
		EF_CHECK_ARGS_NUM(3);
		EF_CHECK_ARG_VAR(1);
		Var *v2 = evalToken(& args[2]);
		if (v2 == NULL)
			return false;
		std::string path = v2->toString();
		FILE *f = fopen(path.c_str(), "rb");
		if (f == NULL) {
			mHasError = true;
			sprintf(mError + strlen(mError), "read file [%s] fail", path.c_str());
			return false;
		}
		fseek(f, 0, SEEK_END);
		int len = ftell(f);
		fseek(f, 0, SEEK_SET);
		char *fcnt = (char *)malloc(len + 1);
		fread(fcnt, len, 1, f);
		fcnt[len] = 0;
		fclose(f);

		Var *nvar = new Var();
		nvar->mName = args[1].mToken;
		nvar->mType = Var::TYPE_STRING;
		nvar->mStrVal = fcnt;
		free(fcnt);
		mVarMap[nvar->mName] = nvar;
		return true;
	}

	if (args[0].mToken == "-write" || args[0].mToken == "-append") {
		EF_CHECK_ARGS_NUM(3);
		Var *v1 = evalToken(& args[1]);
		if (v1 == NULL)
			return false;
		std::string cnd = v1->toString();
		Var *v2 = evalToken(& args[2]);
		if (v2 == NULL)
			return false;
		std::string path = v2->toString();

		const char *mode = (args[0].mToken == "-write") ? "wb" : "ab";
		FILE *f = fopen(path.c_str(), mode);
		if (f == NULL) {
			mHasError = true;
			sprintf(mError + strlen(mError), "write file [%s] fail ", path.c_str());
			return false;
		}
		fwrite(cnd.c_str(), strlen(cnd.c_str()), 1, f);
		fclose(f);

		return true;
	}

	mHasError = true;
	strcat(mError, "invalid file command ");
	return false;
}

bool VM::execFunction_split(std::vector<Token> &args) {
	Buffer sbuf(512);
	EF_CHECK_ARGS_NUM(2);
	std::string splitChar = "\n";

	int na = 0;
	if (args[0].mToken == "-c") {
		Var *v1 = evalToken(& args[1]);
		if (v1 == NULL)
			return false;
		splitChar = v1->toString();
		na = 2;
		EF_CHECK_ARGS_NUM(4);
	}
	EF_CHECK_ARG_VAR(na);
	Var *v = evalToken(& args[na + 1]);
	if (v == NULL)
		return false;
	std::string str = v->toString();

	Var *ret = new Var();
	ret->mType = Var::TYPE_ARRAY;
	ret->mArrayVal = new std::vector<Var>();
	ret->mName = args[na].mToken;

	Var vv;
	vv.mType = Var::TYPE_STRING;
	if (splitChar.empty()) {
		for (int i = 0; i < str.length(); ++i) {
			vv.mStrVal = str[i];
			ret->mArrayVal->push_back(vv);
		}
		mVarMap[ret->mName] = ret;
		return true;
	}

	const char *p = str.c_str();
	while (true) {
		sbuf.reset();
		const char *pp = strstr(p, splitChar.c_str());
		if (*p == 0)
			break;
		if (pp == NULL) {
			sbuf.append(p);
			int len = sbuf.mLen;
			if (splitChar == "\n" && sbuf.mBuf[len - 1] == '\r') {
				sbuf.mBuf[len - 1] = 0;
			}
			vv.mStrVal = sbuf.mBuf;
			ret->mArrayVal->push_back(vv);
			break;
		} 

		int len = pp - p;
		sbuf.append(p, len);
		if (splitChar == "\n" && len > 0 && sbuf.mBuf[len - 1] == '\r') {
			sbuf.mBuf[len - 1] = 0;
		}
		vv.mStrVal = sbuf.mBuf;
		ret->mArrayVal->push_back(vv);
		p = pp + strlen(splitChar.c_str());
	}
	mVarMap[ret->mName] = ret;
	return true;
}

bool VM::execFunction_join(std::vector<Token> &args) {
	static Buffer buf(4096);
	EF_CHECK_ARGS_NUM(3);
	EF_CHECK_ARG_VAR(0);

	Var *v1 = evalToken(& args[1]);
	if (v1 == NULL) {
		return false;
	}
	if (v1->mType != Var::TYPE_ARRAY) {
		mHasError = true;
		strcat(mError, " args[1] must be array ");
		return false;
	}
	std::vector<Var> *arr = v1->mArrayVal;
	Var *v2 = evalToken(& args[2]);
	if (v1 == NULL) {
		return false;
	}
	std::string ss = v2->toString();
	buf.reset();
	for (int i = 0; i < arr->size(); ++i) {
		std::string si = arr->at(i).toString();
		if (i != 0)
			buf.append(ss.c_str());
		buf.append(si.c_str());
	}
	
	Var *ret = new Var();
	ret->mType = Var::TYPE_STRING;
	ret->mName = args[0].mToken;
	ret->mStrVal = buf.mBuf;
	mVarMap[ret->mName] = ret;
	return true;
}

bool VM::execFunction_search( std::vector<Token> &args ) {
	EF_CHECK_ARGS_NUM(3);
	EF_CHECK_ARG_VAR(0);

	Var *v1 = evalToken(& args[1]);
	if (v1 == NULL) {
		return false;
	}
	if (v1->mType != Var::TYPE_ARRAY) {
		mHasError = true;
		strcat(mError, " args[1] must be array ");
		return false;
	}
	std::vector<Var> *arr = v1->mArrayVal;
	Var *v2 = evalToken(& args[2]);
	if (v1 == NULL) {
		return false;
	}
	std::string tag = v2->toString();
	int begin = 0, end = arr->size();

	if (args.size() >= 4) {
		Var *v3 = evalToken(& args[3]);
		if (v3 == NULL) {
			return false;
		}
		begin = (int)v3->toNumber();
	}
	
	if (args.size() >= 5) {
		Var *v4 = evalToken(& args[4]);
		if (v4 == NULL) {
			return false;
		}
		end = (int)v4->toNumber();
	}
	
	if (begin < 0) begin += arr->size();
	if (end < 0) end += arr->size() + 1;

	if (begin < 0) {
		mHasError = true;
		char tmp[128];
		sprintf(tmp, " args[3] invalid number: %d ", begin);
		strcat(mError, tmp);
		return false;
	}
	if (end < 0) {
		mHasError = true;
		char tmp[128];
		sprintf(tmp, " args[4] invalid number: %d ", end);
		strcat(mError, tmp);
		return false;
	}

	int res = -1;
	for (int i = begin; i < end; ++i) {
		std::string si = arr->at(i).mStrVal;
		if (si == tag) {
			res = i;
			break;
		}
	}

	Var *ret = new Var();
	ret->mType = Var::TYPE_STRING;
	ret->mName = args[0].mToken;
	ret->mStrVal = numToStr(res);
	mVarMap[ret->mName] = ret;
	return true;
}

static bool varCmpAsc(const Var &v1, const Var &v2) {
	return v1.mStrVal < v2.mStrVal;
}
static bool varCmpDsc(const Var &v1, const Var &v2) {
	return v1.mStrVal > v2.mStrVal;
}

bool VM::execFunction_sort( std::vector<Token> &args ) {
	EF_CHECK_ARGS_NUM(1);

	Var *v1 = evalToken(& args[0]);
	if (v1 == NULL) {
		return false;
	}
	if (v1->mType != Var::TYPE_ARRAY) {
		mHasError = true;
		strcat(mError, " args[0] must be array ");
		return false;
	}
	std::vector<Var> *arr = v1->mArrayVal;

	bool asc = true;
	if (args.size() >= 2) {
		Var *v2 = evalToken(& args[1]);
		if (v1 == NULL) {
			return false;
		}
		std::string tag = v2->toString();
		asc = tag != "dsc";
	}
	
	if (asc) {
		sort(arr->begin(), arr->end(), varCmpAsc);
	} else {
		sort(arr->begin(), arr->end(), varCmpDsc);
	}
	return true;
}

bool VM::execFunction_substr(std::vector<Token> &args) {
	EF_CHECK_ARGS_NUM(3);
	EF_CHECK_ARG_VAR(0);

	Var *v1 = evalToken(& args[1]);
	if (v1 == NULL)
		return false;
	std::string s1 = v1->toString();
	Var *v2 = evalToken(& args[2]);
	if (v2 == NULL)
		return false;
	std::string s2 = v2->toString();
	int fromIdx = (int)strtod(s2.c_str(), NULL);
	int endIdx = -1;
	if (args.size() >= 4) {
		Var *v3 = evalToken(& args[3]);
		if (v3 == NULL)
			return false;
		std::string s3 = v3->toString();
		endIdx = (int)strtod(s3.c_str(), NULL);
	}
	if (fromIdx < 0) 
		fromIdx += s1.length();
	if (endIdx < 0)
		endIdx += s1.length() + 1;
	if (endIdx > s1.length())
		endIdx = s1.length();

	Var *ret = new Var();
	ret->mName = args[0].mToken;
	ret->mType = Var::TYPE_STRING;
	if (fromIdx < 0 || endIdx < 0 || fromIdx >= endIdx || fromIdx >= s1.length()) {
		ret->mStrVal = "";
	} else {
		ret->mStrVal = s1.substr(fromIdx, endIdx - fromIdx);
	}
	mVarMap[ret->mName] = ret;
	return true;
}

bool VM::execFunction_strreplace(std::vector<Token> &args) {
	return false;
}

bool VM::execFunction_strchr(std::vector<Token> &args, bool flag) {
	EF_CHECK_ARGS_NUM(4);
	EF_CHECK_ARG_VAR(0);

	Var *v1 = evalToken(& args[1]);
	if (v1 == NULL)
		return false;
	std::string src = v1->toString();

	Var *v2 = evalToken(& args[2]);
	if (v2 == NULL)
		return false;
	int idx = (int)v2->toNumber();

	Var *v3 = evalToken(& args[3]);
	if (v3 == NULL)
		return false;
	std::string sub = v3->toString();

	const char *sp = src.c_str();
	const char *ss = sub.c_str();
	const char *d = NULL;

	Var *ret = new Var();
	ret->mName = args[0].mToken;
	mVarMap[ret->mName] = ret;
	ret->mStrVal = "-1";

	if (src.empty() || sub.empty()) {
		return true;
	}
	int len = strlen(sp);
	if (idx < 0) idx += len;
	if (idx < 0 || idx >= len) {
		return true;
	}

	if (flag) {
		d = strstr(sp + idx, ss);
	} else {
		const char *ep = sp + idx;
		int clen = strlen(ss);
		while (ep >= sp) {
			if (strncmp(ep, ss, clen) == 0) {
				d = ep;
				break;
			}
			--ep;
		}
	}
	if (d != NULL) {
		char tmp[24];
		sprintf(tmp, "%d", (d - sp));
		ret->mStrVal = tmp;
	}

	return true;
}

// OK
Var* VM::evalVar( std::string varName ) {
	std::string oldName = varName;
	if (varName.length() == 0) {
		mHasError = true;
		strcat(mError, "Var name is empty");
		return NULL;
	}
	if (varName.at(0) == '$') varName = varName.substr(1);
	
	if (varName.length() > 0 && mVarMap.find(varName) != mVarMap.end()) {
		Var *v = mVarMap[varName];
		return v;
	}
	if (isRootVM()) {
		mHasError = true;
		sprintf(mError + strlen(mError), "Not exist var: %s", oldName.c_str());
		return NULL;
	}

	VM *parent = mParent;
	while (parent != NULL) {
		if (parent->isRootVM()) {
			return parent->evalVar(varName);
		}
		parent = parent->mParent;
	}

	mHasError = true;
	sprintf(mError + strlen(mError), "Unknown var: %s", oldName.c_str());
	return NULL;
}

Var* VM::evalToken(Token *token) {
	static Var V;

	V.mType = Var::TYPE_STRING;
	V.mArrayVal = NULL;

	if (token == NULL)
		return NULL;
	if (token->mType == TOKEN_EOF) {
		V.mStrVal = "";
		return &V;
	}
	if (token->mType == TOKEN_NORMAL && token->mToken[0] == '$') {
		return evalVar(token->mToken);
	}
	V.mStrVal = token->mToken;
	return &V;
}

Frame* VM::topFrame() {
	if (mFrameStack.size() == 0)
		return NULL;
	return &mFrameStack[mFrameStack.size() - 1];
}

bool VM::popFrame() {
	int sz = mFrameStack.size();
	if (sz > 0) {
		mFrameStack.erase(mFrameStack.begin() + sz - 1);
		return true;
	}
	return false;
}

bool VM::isRootVM() {
	return false;
}

FuncVM::FuncVM( VM *parent, FuncDefine *def ) {
	mParent = parent;
	mError = parent->mError;
	mFileContent = def->mCnt;
	mFileName = parent->mFileName;
	mFileLen = def->mCntLen;
	mLineNo = def->mStartLineNo - 1;
	mNextLinePos = mFileContent;
	mCurLinePos = mFileContent;
}

bool FuncVM::addArg( Token *arg, int idx) {
	char name[8];
	sprintf(name, "arg%d", idx);
	Var *v = evalToken(arg);
	if (v != NULL) {
		mVarMap[name] = new Var(*v);
		return true;
	}
	return false;
}

FuncVM::~FuncVM() {

}

RootVM::RootVM( const char *filePath ) : VM(filePath) {
}

bool RootVM::isRootVM() {
	return true;
}

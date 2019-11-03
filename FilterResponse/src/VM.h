#pragma once
#include <map>
#include <string>
#include <vector>

class FuncVM;

class Var {
public:
	enum Type {TYPE_STRING, TYPE_ARRAY};
	Var();

	Type mType;
	std::string mName;
	std::string mStrVal;
	std::vector<Var> *mArrayVal;
	~Var();
	std::string toString();
	double toNumber();
};

struct Frame {
	enum FrameType {FRAME_IF, FRAME_ELSE_IF, FRAME_ELSE, FRAME_LOOP};
	Frame(FrameType t, char *pos, int line);

	int mStartLineNo;
	char *mStartPos;
	FrameType mType;
	bool mCondition;
};

struct FuncDefine {
	FuncDefine(char *name) {
		mName = name;
		mStartLineNo = 0;
		mCnt = NULL;
		mCntLen = 0;
	}
	std::string mName;
	int mStartLineNo;
	char *mCnt;
	int mCntLen;
};


class VM {
public:
	enum TokenType {TOKEN_EOF, TOKEN_NORMAL, TOKEN_STRING};
	enum ConditionFlag {COND_FLAG_NONE, COND_FLAG_MAYBE_NUM, COND_FLAG_ALL_NUM};

	struct Token {
		TokenType mType;
		std::string mToken;
	};
	VM();
	VM(const char *filePath);
	void setContent(const char *fileName, char *cnt);
	virtual ~VM();

	bool run();
	bool hasError();
	char *getError();
	Var *getVar(std::string varName) {return evalVar(varName);}

protected:
	bool readLine();
	void readFile(const char *path);

	bool runLineExp();
	Token *nextToken(char **p);
	bool runSetExp( char ** pos );
	bool runIfExp( char ** pos );
	bool runElseExp(char ** pos );
	bool runEndIfExp(char **pos);
	bool checkIfFrameAlreadRuned();
	bool isInIfBlockList();
	bool runWhileExp(char ** pos);
	bool runDoneExp(char **pos);
	bool runContinueExp(char ** pos);
	bool runFuncDefine(char **pos);
	bool runCallExp(char ** pos);

	bool exceptCondition(char ** pos, bool *ret, std::string endToken );
	bool exceptConditionPart(char ** pos, ConditionFlag flag, bool *ret );
	bool runFunctionExp(std::string cmd, char ** pos );

	bool exceptVarName(char **retName, char **pos);
	bool exceptString( char ** str, char ** pos );
	bool exceptStringSequnce( char ** str, char ** pos );
	bool exceptMathSequnce( char ** str, char ** pos );
	bool execMathSequnce(void *mi, int *num);
	bool exceptArraySequnce(Var *v, char ** pos );
	bool exceptLineEOF(char *pos );
	bool exceptEq( char ** pos );

	bool execFunction( std::string &cmd, std::vector<Token> &args );
	bool execFunction_echo( std::vector<Token> &args );
	bool execFunction_length( std::vector<Token> &args );
	bool execFunction_append( std::vector<Token> &args );
	bool execFunction_at( std::vector<Token> &args );
	bool execFunction_process( std::vector<Token> &args );
	bool execFunction_file( std::vector<Token> &args );
	bool execFunction_split( std::vector<Token> &args );
	bool execFunction_join( std::vector<Token> &args );
	bool execFunction_search( std::vector<Token> &args );
	bool execFunction_sort( std::vector<Token> &args );

	bool execFunction_substr( std::vector<Token> &args );
	bool execFunction_strreplace( std::vector<Token> &args );
	bool execFunction_strchr( std::vector<Token> &args, bool flag );

	Var *evalVar(std::string varName);
	Var *evalToken(Token *token);

	Frame *topFrame();
	bool popFrame();
	virtual bool isRootVM();
protected:
	char *mFileName;
	char *mFileContent;
	int mFileLen;
	bool mHasError;
	char *mError;
	char mLine[512];
	int mLineNo;
	char *mNextLinePos, *mCurLinePos;

	// variant map
	std::map<std::string, Var*> mVarMap;
	std::vector<Frame> mFrameStack;
	std::map<std::string, FuncDefine*> mFuncDefineMap;
	VM *mParent;
	friend class FuncVM;
};

class FuncVM : public VM {
public:
	FuncVM(VM *parent, FuncDefine *def);
	bool addArg(Token *arg, int idx);
	virtual ~FuncVM();
protected:
	std::vector<Var> mArgs;
};

class RootVM : public VM {
public:
	RootVM(const char *filePath);
protected:
	virtual bool isRootVM();
};
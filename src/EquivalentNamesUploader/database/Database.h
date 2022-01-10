#ifndef DATABASE_H
#define DATABASE_H

#ifdef WIN32

// Future note: this is the prefered error handling method; use this macro to interpret and rethrow on exceptional cases
#define RETHROW_ERR			catch(_com_error & e){												\
								throw DatabaseException( exceptionInterpreter( e ).c_str() );	\
							}


#define CATCHERRGET			catch(_com_error &e)\
							{\
								_ErrorHandler(e,m_ErrStr);\
								sprintf(m_ErrStr,"%s\n**For Field Name:%s",m_ErrStr,FieldName);\
								return 0;\
							}

#define CATCHERROR(ptr,a)	catch(_com_error &e)\
							{\
								_ErrorHandler(e,m_ErrStr);\
								ptr=NULL;\
								return a;\
							}

// Backwards compatibility, requires following http://support.microsoft.com/kb/2517589/
#import "c:\program files\common files\system\ado\msado60_Backcompat.tlb" rename ("EOF","adoEOF")

// Standard version, binaries compiled on Windows 7 machines won't work on older machines
//#import "c:\program files\common files\system\ado\msado15.dll" rename ("EOF","adoEOF")

#include <exception>

typedef ADODB::_RecordsetPtr	RecPtr;
typedef ADODB::_ConnectionPtr	CnnPtr; 
typedef ADODB::_CommandPtr		CmdPtr;
typedef ADODB::_ParameterPtr	ParamPtr;

class Database;
class Table;


// generic DB error handling
class DatabaseException : public std::runtime_error {
public: explicit DatabaseException( const char * s ) : std::runtime_error( s ) {}
};


class Database {
public:

	// RAII paradigm; throws on connection error
	Database(const char* UserName, const char* Pwd, const char* CnnStr);
	
	~Database();

	void OpenTbl(int Mode, char* CmdStr, Table& Tbl);
	void Execute(const char* CmdStr);
	void Execute(const wchar_t* CmdStr);
	void Execute(const char* CmdStr, Table& Tbl);
	void Execute(const wchar_t* CmdStr, Table& Tbl);
	
	void Execute(CmdPtr Cmd);
	void Execute(CmdPtr Cmd, Table& Tbl);

    CmdPtr PrepareCmd(const char* CmdStr);
	CmdPtr PrepareCmd(const wchar_t* CmdStr);
	
	// DISABLED DUE TO LACK OF COMPATIBILITY WITH WINDOWS 7
	/*void AddParamToCmd(CmdPtr Cmd, int Val);
	void AddParamToCmd(CmdPtr Cmd, std::string Val);
	void AddParamToCmd(CmdPtr Cmd, std::wstring Val);
	void AddParamToCmd(CmdPtr Cmd, double Val);*/
	
	static void SetDefaultTimeout( int to ){ _default_timeout = to; }
	static int GetDefaultTImeout() { return _default_timeout; }

private:

	CnnPtr m_Cnn;

	static int _default_timeout;
	static std::string exceptionInterpreter( _com_error & e );
};


class Table{
public:
	RecPtr m_Rec;
	char m_ErrStr[500];
	Table();
	void GetErrorErrStr(char* ErrStr);
	int ISEOF();
	HRESULT MoveNext();
	HRESULT MovePrevious();
	HRESULT MoveFirst();
	HRESULT MoveLast();
	int AddNew();
	int Update();
	int Add(const char* FieldName, char* FieldValue);
	int Add(const char* FieldName,int FieldValue);
	int Add(const char* FieldName,float FieldValue);
	int Add(const char* FieldName,double FieldValue);
	int Add(const char* FieldName,long FieldValue);
	bool Get(const char* FieldName, char* FieldValue);
	bool Get(const char* FieldName, char* FieldValue, unsigned int NChar);
	bool Get(const char* FieldName,int& FieldValue);
	bool Get(const char* FieldName,float& FieldValue);
	bool Get(const char* FieldName,double& FieldValue);
	bool Get(const char* FieldName,double& FieldValue,int Scale);
	bool Get(const char* FieldName,long& FieldValue);
	bool Get(const char* FieldName, wchar_t* FieldValue);
	bool Get(const char* FieldName, wchar_t* FieldValue, unsigned int NChar);
private:
	// this method should be made to go away...
	static void _ErrorHandler(_com_error &e, char* ErrStr);
};
#else // Linux
#error "Database.h needs to be rewritten for Linux"
#endif
#endif


#include "common/leak_detection.h" // This must be the first #include

#include <stdio.h>
#include <iostream>
//#include <comdef.h>
//#include <conio.h>
#include <sstream>
#include "Database.h"
#include <wchar.h>

//ADODB::_RecordsetPtr rec1=NULL;

//_variant_t  vtMissing1(DISP_E_PARAMNOTFOUND, VT_ERROR); 

// static instance initialization
int Database::_default_timeout = 600;

using namespace std;


string Database::exceptionInterpreter(_com_error &e)
{
	stringstream estr;
	estr << "Error:\nCode = " << e.Error();
	estr << "\nCode meaning = " << e.ErrorMessage();
	estr << "\nSource = " << e.Source();
	estr << "\nDescription = " << e.Description() << endl;

	return estr.str();
}


// Follows the RAII paradigm
Database::Database(const char* UserName, const char* Pwd, const char* CnnStr)
{
	try{
		::CoInitialize(NULL);
		HRESULT hr;
		hr = m_Cnn.CreateInstance( __uuidof( ADODB::Connection ) );
		m_Cnn->Open(CnnStr, UserName, Pwd, NULL);
		m_Cnn->CommandTimeout = _default_timeout;
	}
	
	RETHROW_ERR;

	return;
}


Database::~Database()
{
	try
	{
      if (m_Cnn)
      {
         m_Cnn->Close();
         m_Cnn=NULL;
      }
	  
	  ::CoUninitialize();
	}
	catch( ... ) {
		// don't allow throws in the destructor
		m_Cnn = NULL;		
	}
}


void Database::OpenTbl(int Mode, char* CmdStr, Table &Tbl)
{

	try{
		RecPtr t_Rec=NULL;
		//t_Rec->putref_ActiveConnection(m_Cnn);
		//vtMissing<<-->>_variant_t((IDispatch *) m_Cnn, true)
		t_Rec.CreateInstance( __uuidof( ADODB::Recordset ) );
		t_Rec->Open(CmdStr,_variant_t((IDispatch *) m_Cnn, true),ADODB::adOpenStatic,ADODB::adLockOptimistic,Mode);

		Tbl.m_Rec=t_Rec;
	}
	
	RETHROW_ERR;
}


void Database::Execute(const char* CmdStr)
{
	try{
		m_Cnn->Execute(CmdStr,NULL,1);
	}
	RETHROW_ERR;
}

void Database::Execute(const wchar_t* CmdStr)
{
	try
	{
		m_Cnn->Execute(CmdStr,NULL,1);
	}
	RETHROW_ERR;
}


void Database::Execute(const char* CmdStr, Table& Tbl)
{
	try	{
		RecPtr t_Rec=NULL;	
		t_Rec=m_Cnn->Execute(CmdStr,NULL,1);
		Tbl.m_Rec=t_Rec;
	}
	RETHROW_ERR;
}

void Database::Execute(const wchar_t* CmdStr, Table& Tbl)
{
	try	{
		RecPtr t_Rec=NULL;
		t_Rec=m_Cnn->Execute(CmdStr,NULL,1);
		Tbl.m_Rec=t_Rec;
	}
	RETHROW_ERR;
}

void Database::Execute(CmdPtr Cmd)
{
	try {
		Cmd->Execute(NULL, NULL, ADODB::adCmdText);
	}
	RETHROW_ERR;
}

void Database::Execute(CmdPtr Cmd, Table& Tbl)
{
	try {
		RecPtr t_Rec=NULL;
		t_Rec=Cmd->Execute(NULL, NULL, ADODB::adCmdText);
		Tbl.m_Rec=t_Rec;
	}
	RETHROW_ERR;
}

CmdPtr Database::PrepareCmd(const char* CmdStr)
{
	CmdPtr cmd;
	try {
		cmd.CreateInstance(__uuidof(ADODB::Command));
		cmd->ActiveConnection = m_Cnn;
		cmd->CommandText = _bstr_t(CmdStr);
	}
	RETHROW_ERR;

	return cmd;
}

CmdPtr Database::PrepareCmd(const wchar_t* CmdStr)
{
	CmdPtr cmd;
	try {
		cmd.CreateInstance(__uuidof(ADODB::Command));
		cmd->ActiveConnection = m_Cnn;
		cmd->CommandText = _bstr_t(CmdStr);
	}
	RETHROW_ERR;

	return cmd;
}


//DISABLED DUE TO LACK OF COMPATIBILITY WITH WINDOWS 7
/*void Database::AddParamToCmd(CmdPtr Cmd, int Val)
{
	try {
		ParamPtr param = Cmd->CreateParameter(_bstr_t(L""), ADODB::adInteger, ADODB::adParamInput,
			-1, _variant_t( (long) Val));
		Cmd->Parameters->Append(param);
	}
	RETHROW_ERR;
}

void Database::AddParamToCmd(CmdPtr Cmd, std::string Val)
{
	try {
		ParamPtr param = Cmd->CreateParameter(_bstr_t(L""), ADODB::adVarChar, ADODB::adParamInput,
			Val.size(), _variant_t(Val.c_str()));
		Cmd->Parameters->Append(param);
	}
	RETHROW_ERR;
}

void Database::AddParamToCmd(CmdPtr Cmd, std::wstring Val)
{
	try {
		ParamPtr param = Cmd->CreateParameter(_bstr_t(L""), ADODB::adVarWChar, ADODB::adParamInput,
			Val.size(), _variant_t(Val.c_str()));
		Cmd->Parameters->Append(param);
	}
	RETHROW_ERR;
}

void Database::AddParamToCmd(CmdPtr Cmd, double Val)
{
	try {
		ParamPtr param = Cmd->CreateParameter(_bstr_t(L""), ADODB::adDouble, ADODB::adParamInput,
			-1, _variant_t(Val));
		Cmd->Parameters->Append(param);
	}
	RETHROW_ERR;
}*/


Table::Table()
{
	m_Rec=NULL;
}

int Table::ISEOF()
{
	int rs;
	if(m_Rec==NULL)
	{
		sprintf(m_ErrStr,"Invalid Record");
		return -1;
	}
	try{
		rs=m_Rec->adoEOF;
	}
	
	CATCHERROR(m_Rec,-2)

	sprintf(m_ErrStr,"Success");
	return rs;
}

bool Table::Get(const char* FieldName, char* FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		sprintf(FieldValue,"%s",(LPCSTR)((_bstr_t)vtValue.bstrVal));
	}

	CATCHERRGET

	sprintf(m_ErrStr,"Success");
	return 1;
}

bool Table::Get(const char* FieldName, char* FieldValue, unsigned int NChar)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		size_t valLen = strlen((_bstr_t) vtValue);
		if (valLen <= NChar) {
			sprintf(FieldValue,"%s",(LPCSTR)((_bstr_t)vtValue.bstrVal));
		}
		else {
			sprintf(m_ErrStr, "Destination char[%d] won't fit char[%d] from database field %s", 
				NChar, valLen, FieldName);
			return 0;
		}
	}

	CATCHERRGET

	sprintf(m_ErrStr,"Success");
	return 1;
}

bool Table::Get(const char* FieldName, wchar_t* FieldValue) {
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		wcscpy(FieldValue,((_bstr_t)vtValue.bstrVal));

	}

	CATCHERRGET

	sprintf(m_ErrStr,"Success");
	return 1;
}

bool Table::Get(const char* FieldName, wchar_t* FieldValue, unsigned int NChar)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		unsigned int valLen = strlen((_bstr_t) vtValue);
		if (valLen <= NChar) {
			wcscpy_s(FieldValue,NChar+1,((_bstr_t)vtValue.bstrVal));
		}
		else {
			_bstr_t step2 = ((_bstr_t)vtValue);
			sprintf(m_ErrStr, "Destination wchar[%d] won't fit wchar[%d] from database wide field %s",
				NChar, valLen, FieldName);
			return 0;

		}
	}

	CATCHERRGET

	sprintf(m_ErrStr,"Success");
	return 1;
}

bool Table::Get(const char* FieldName,int& FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		FieldValue=vtValue.intVal;
	}

	CATCHERRGET

	sprintf(m_ErrStr,"Success");
	return 1;
}

bool Table::Get(const char* FieldName,float& FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		FieldValue=vtValue.fltVal;
	}

	CATCHERRGET

	sprintf(m_ErrStr,"Success");
	return 1;
}

bool Table::Get(const char* FieldName,double& FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		FieldValue=vtValue.dblVal;
		//GetDec(vtValue,FieldValue,3);
	}

	CATCHERRGET

	sprintf(m_ErrStr,"Success");
	return 1;
}

HRESULT Table::MoveNext()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MoveNext();
	}
	catch(_com_error &e)
	{
		_ErrorHandler(e,m_ErrStr);
		//m_Rec=NULL;
		return -2;
	}
	sprintf(m_ErrStr,"Success");
	return hr;
}

HRESULT Table::MovePrevious()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MovePrevious();
	}
	catch(_com_error &e)
	{
		_ErrorHandler(e,m_ErrStr);
		//m_Rec=NULL;
		return -2;
	}
	sprintf(m_ErrStr,"Success");
	return hr;
}

HRESULT Table::MoveFirst()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MoveFirst();
	}
	catch(_com_error &e)
	{
		_ErrorHandler(e,m_ErrStr);
		//m_Rec=NULL;
		return -2;
	}
	sprintf(m_ErrStr,"Success");
	return hr;
}

HRESULT Table::MoveLast()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MoveLast();
	}
	catch(_com_error &e)
	{
		_ErrorHandler(e,m_ErrStr);
		//m_Rec=NULL;
		return -2;
	}
	sprintf(m_ErrStr,"Success");
	return hr;
}


void Table::GetErrorErrStr(char* ErrStr)
{
	sprintf(ErrStr,"%s",m_ErrStr);
}


void Table::_ErrorHandler(_com_error &e, char* ErrStr)
{
	sprintf(ErrStr,"Error:\n");
	sprintf(ErrStr,"%sCode = %08lx\n",ErrStr ,e.Error());
	sprintf(ErrStr,"%sCode meaning = %s\n", ErrStr, (char*) e.ErrorMessage());
	sprintf(ErrStr,"%sSource = %s\n", ErrStr, (char*) e.Source());
	sprintf(ErrStr,"%sDescription = %s",ErrStr, (char*) e.Description());
}

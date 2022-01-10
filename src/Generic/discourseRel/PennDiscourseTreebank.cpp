// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/OutputUtil.h"
#include "boost/regex.hpp"

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/StringTransliterator.h"

#include "Generic/discourseRel/PennDiscourseTreebank.h"
#include "Generic/discourseRel/CrossSentRelation.h"
#include <boost/scoped_ptr.hpp>

#define MAX_PDTB_INFO_LEN 2500
#define PDTBDOCID_LEN 4

// static member initialization
int PennDiscourseTreebank::_num_documents = 0;
PennDiscourseTreebank::ExplicitConnectiveTable *PennDiscourseTreebank::_connective_table = 0;
PennDiscourseTreebank::CrossSentRelationTable *PennDiscourseTreebank::_crossSentRel_table = 0;

void PennDiscourseTreebank::loadDataFromPDTBFileList (const char *listfile){
	
	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;
	
	
	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadDataFromPDTBFile(token.chars());
		_num_documents ++ ;
	}
	
}

void PennDiscourseTreebank::loadDataFromPDTBFileList (const char* listfile, TargetConnectives::ExplicitConnectiveDict *connectiveDict){
	
	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;
	
	
	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadDataFromPDTBFile(token.chars(), connectiveDict);
		_num_documents ++ ;
	}
}

void PennDiscourseTreebank::loadCrossSentDataFromPDTBFileList (const char *listfile){
	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;
	
	
	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadCrossSentDataFromPDTBFile(token.chars());
		_num_documents ++ ;
	}
}


void PennDiscourseTreebank::loadDataFromPDTBFile(const wchar_t *filename){
	char pdtb_file_name_str[501];
	StringTransliterator::transliterateToEnglish(pdtb_file_name_str, filename, 500);
	std::cerr << "Loading data from " << pdtb_file_name_str << "\n";

	/* read in PDTB file and fill in the table */
	//Create a new hash table to store parameters and values
	//Make sure it doesnt already exist for recursive calls
	if (_connective_table == 0)
	{
		//Allocate Memory for the explicit connective Hash Table
	    _connective_table   = _new ExplicitConnectiveTable();
		
	}

	//Open file for reading
	ifstream inputFile (pdtb_file_name_str);
	string strFile = pdtb_file_name_str;
    if (! inputFile.is_open())
    {
		string s = "Can't Open PDTB File: " + strFile;
		throw UnexpectedInputException("PennDiscourseTreebank::loadDataFromPDTBFile()", s.c_str());
		return;
	}

	size_t loc;
	loc = strFile.find( ".doc", 0 );
	if (loc == string::npos){
		
		throw UnexpectedInputException("illegal PDTB file name: ", strFile.c_str());
		return;
	}
	string docName = strFile.substr(loc-PDTBDOCID_LEN,PDTBDOCID_LEN);

	
    while (! inputFile.eof() )
    {
		char buffer[MAX_PDTB_INFO_LEN];   // Temporary line buffer
       
		//Get Current Line
		inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
			
		size_t bufferLen = strlen(buffer);

		if (bufferLen > 0)
		{
			if ((buffer[0] != '\0') && (buffer[0] != '#')){
				findConnLocation (docName, buffer);
			}
			
		}
	}
}

void PennDiscourseTreebank::loadDataFromPDTBFile (const wchar_t *filename, TargetConnectives::ExplicitConnectiveDict * connectiveDict){
	char pdtb_file_name_str[501];
	StringTransliterator::transliterateToEnglish(pdtb_file_name_str, filename, 500);
	std::cerr << "Loading data from " << pdtb_file_name_str << "\n";

	/* read in PDTB file and fill in the table */
	//Create a new hash table to store parameters and values
	//Make sure it doesnt already exist for recursive calls
	if (_connective_table == 0)
	{
		//Allocate Memory for the explicit connective Hash Table
	    _connective_table   = _new ExplicitConnectiveTable();
		
	}

	//Open file for reading
	ifstream inputFile (pdtb_file_name_str);
	string strFile = pdtb_file_name_str;
    if (! inputFile.is_open())
    {
		string s = "Can't Open PDTB File: " + strFile;
		throw UnexpectedInputException("PennDiscourseTreebank::loadDataFromPDTBFile()", s.c_str());
		return;
	}

	size_t loc;
	loc = strFile.find( ".doc", 0 );
	if (loc == string::npos){
		
		throw UnexpectedInputException("illegal PDTB file name: ", strFile.c_str());
		return;
	}
	string docName = strFile.substr(loc-PDTBDOCID_LEN,PDTBDOCID_LEN);

	
    while (! inputFile.eof() )
    {
		char buffer[MAX_PDTB_INFO_LEN];   // Temporary line buffer
       
		//Get Current Line
		inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
			
		size_t bufferLen = strlen(buffer);

		if (bufferLen > 0)
		{
			if ((buffer[0] != '\0') && (buffer[0] != '#')){
				findConnLocation (docName, buffer, connectiveDict);
			}
			
		}
	}

}

void PennDiscourseTreebank::loadCrossSentDataFromPDTBFile (const wchar_t *filename){
	char pdtb_file_name_str[501];
	StringTransliterator::transliterateToEnglish(pdtb_file_name_str, filename, 500);
	std::cerr << "Loading data from " << pdtb_file_name_str << "\n";

	/* read in PDTB file and fill in the table */
	//Create a new hash table to store parameters and values
	//Make sure it doesnt already exist for recursive calls
	if (_crossSentRel_table == 0)
	{
		//Allocate Memory for the cross sentence relation Hash Table
	    _crossSentRel_table   = _new CrossSentRelationTable();
		
	}

	//Open file for reading
	ifstream inputFile (pdtb_file_name_str);
	string strFile = pdtb_file_name_str;
    if (! inputFile.is_open())
    {
		string s = "Can't Open PDTB File: " + strFile;
		throw UnexpectedInputException("PennDiscourseTreebank::loadCrossSentDataFromPDTBFile()", s.c_str());
		return;
	}

	size_t loc;
	loc = strFile.find( ".doc", 0 );
	if (loc == string::npos){
		
		throw UnexpectedInputException("illegal PDTB file name: ", strFile.c_str());
		return;
	}
	string docName = strFile.substr(loc-PDTBDOCID_LEN,PDTBDOCID_LEN);
				
    while (! inputFile.eof() )
    {
		char buffer[MAX_PDTB_INFO_LEN];   // Temporary line buffer
       
		//Get Current Line
		inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
			
		size_t bufferLen = strlen(buffer);

		if (bufferLen > 0)
		{
			if ((buffer[0] != '\0') && (buffer[0] != '#')){
				//findCrossSentRelation (docName, buffer);
				string line=buffer;
				if (line.find("___Explicit___") != string::npos ){
					string relType = "Explicit";
					string connective;
					int relPos;
					string relPosStr;
					int sent1;
					string sent1Str;
					int sent2;
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					string line = buffer;
					// read in connective
					if (line.find ("Connective: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						connective=line.substr(loc+2,strLength-loc-2);
					}
					// skip the line "Connective word: ..."
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
				
					// read in relation position
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line=buffer;
					if (line.find ("Relation Pos: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						//relPos=atoi((line.substr(loc+2,strLength-loc-2)).c_str());
						relPosStr=line.substr(loc+2,strLength-loc-2);
						relPos=atoi(relPosStr.c_str());
					}
					
					// read in Relation Context  Sent1 and Sent 2
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line=buffer;
					if (line.find ("Relation context: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						string sentPair=line.substr(loc+2,strLength-loc-2);
						loc = sentPair.find(" ");
						sent1Str=sentPair.substr(0,loc);
						sent1=atoi(sent1Str.c_str());
						sent2=atoi((sentPair.substr(loc+1,sentPair.size()-loc-1)).c_str());
					}

					// skip linking info for explicit connective
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);

					// generate a CrossSentRelation object
					string PDTBindex = docName+"%"+sent1Str;
					CrossSentRelation csRelObj(PDTBindex, relType, sent1, sent2);
					csRelObj.setConnective(connective);
					csRelObj.setRelationPos(relPos);
					

					// add relation object to _crossSentRel_table
					CrossSentRelationTable::iterator myIterator = _crossSentRel_table->find(PDTBindex);
					if( myIterator == _crossSentRel_table->end()){
						vector<CrossSentRelation> vCSRel;
						(*_crossSentRel_table)[PDTBindex]=vCSRel;
					}	
					((*_crossSentRel_table)[PDTBindex]).push_back(csRelObj);

				}else if (line.find("___Implicit___") != string::npos){
					string relType = "Implicit";
					string connective;
					string semType;
					int relPos;
					string relPosStr;
					int sent1;
					string sent1Str;
					int sent2;
					
					// read in connective
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					string line = buffer;
					if (line.find ("Connective: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						connective=line.substr(loc+2,strLength-loc-2);
					}

					// read in semantic type
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line = buffer;
					if (line.find ("Semantic Type: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						semType=line.substr(loc+2,strLength-loc-2);
					}
					
					// read in relation position
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line = buffer;
					if (line.find ("Relation Pos: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						//relPos=atoi((line.substr(loc+2,strLength-loc-2)).c_str());
						relPosStr=line.substr(loc+2,strLength-loc-2);
						relPos=atoi(relPosStr.c_str());
					}
					
					// read in Relation Context  Sent1 and Sent 2
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line=buffer;
					if (line.find ("Relation context: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						string sentPair=line.substr(loc+2,strLength-loc-2);
						loc = sentPair.find(" ");
						sent1Str=sentPair.substr(0,loc);
						sent1=atoi(sent1Str.c_str());
						sent2=atoi((sentPair.substr(loc+1,sentPair.size()-loc-1)).c_str());
					}

					// generate a CrossSentRelation object
					string PDTBindex = docName+"%"+sent1Str;
					CrossSentRelation csRelObj(PDTBindex, relType, sent1, sent2);
					csRelObj.setConnective(connective);
					csRelObj.setSemType(semType);
					csRelObj.setRelationPos(relPos);
					
					// add relation object to _crossSentRel_table
					CrossSentRelationTable::iterator myIterator = _crossSentRel_table->find(PDTBindex);
					if(myIterator == _crossSentRel_table->end())
					{	
						vector<CrossSentRelation> vCSRel;
						(*_crossSentRel_table)[PDTBindex]=vCSRel;
					}	
					((*_crossSentRel_table)[PDTBindex]).push_back(csRelObj);

				}else if (line.find("___AltLex___") != string::npos){
					string relType = "AltLex";
					string semType;
					string relPosStr;
					int relPos;
					int sent1;
					string sent1Str;
					int sent2;
					
					// read in semantic type
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line = buffer;
					if (line.find ("Semantic Type: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						semType=line.substr(loc+2,strLength-loc-2);
					}
					
					// read in relation position
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line = buffer;
					if (line.find ("Relation Pos: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						//relPos=atoi((line.substr(loc+2,strLength-loc-2)).c_str());
						relPosStr=line.substr(loc+2,strLength-loc-2);
						relPos=atoi(relPosStr.c_str());
					}
					
					// read in Relation Context  Sent1 and Sent 2
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line=buffer;
					if (line.find ("Relation context: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						string sentPair=line.substr(loc+2,strLength-loc-2);
						loc = sentPair.find(" ");
						sent1Str=sentPair.substr(0,loc);
						sent1=atoi(sent1Str.c_str());
						sent2=atoi((sentPair.substr(loc+1,sentPair.size()-loc-1)).c_str());
					}

					// generate a CrossSentRelation object
					string PDTBindex = docName+"%"+sent1Str;
					CrossSentRelation csRelObj(PDTBindex, relType, sent1, sent2);
					csRelObj.setSemType(semType);
					csRelObj.setRelationPos(relPos);
					
					// add relation object to _crossSentRel_table
					CrossSentRelationTable::iterator myIterator = _crossSentRel_table->find(PDTBindex);
					if(myIterator == _crossSentRel_table->end())
					{	
						vector<CrossSentRelation> vCSRel;
						(*_crossSentRel_table)[PDTBindex]=vCSRel;
					}	
					((*_crossSentRel_table)[PDTBindex]).push_back(csRelObj);

				}else if (line.find("___EntRel___") != string::npos){
					string relType = "EntRel";
					int relPos;
					string relPosStr;
					int sent1;
					string sent1Str;
					int sent2;
													
					// read in relation position
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line = buffer;
					if (line.find ("Relation Pos: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						//relPos=atoi((line.substr(loc+2,strLength-loc-2)).c_str());
						relPosStr=line.substr(loc+2,strLength-loc-2);
						relPos=atoi(relPosStr.c_str());
					}
					
					// read in Relation Context  Sent1 and Sent 2
					inputFile.getline(buffer,MAX_PDTB_INFO_LEN);
					line=buffer;
					if (line.find ("Relation context: ") == string::npos){
						throw UnexpectedInputException("invalid format of PDTB file: ", docName.c_str());
					}else{
						size_t loc = line.find(": ");
						size_t strLength = line.size();
						string sentPair=line.substr(loc+2,strLength-loc-2);
						loc = sentPair.find(" ");
						sent1Str=sentPair.substr(0,loc);
						sent1=atoi(sent1Str.c_str());
						sent2=atoi((sentPair.substr(loc+1,sentPair.size()-loc-1)).c_str());
					}

					// generate a CrossSentRelation object
					string PDTBindex = docName+"%"+sent1Str;
					CrossSentRelation csRelObj(PDTBindex, relType, sent1, sent2);
					csRelObj.setRelationPos(relPos);
					
					// add relation object to _crossSentRel_table
					CrossSentRelationTable::iterator myIterator = _crossSentRel_table->find(PDTBindex);
					if(myIterator == _crossSentRel_table->end())
					{	
						vector<CrossSentRelation> vCSRel;
						(*_crossSentRel_table)[PDTBindex]=vCSRel;
					}	
					((*_crossSentRel_table)[PDTBindex]).push_back(csRelObj);
				}
			}
		}
	}
}

void PennDiscourseTreebank::findConnLocation (string docName, const char *onelineOfPDTB){
	//boost::regex expression("^(template[[:space:]]*<[^;:{]+>[[:space:]]*)?(class|struct)[[:space:]]*(\\<\\w+\\>([[:blank:]]*\\([^)]*\\))?[[:space:]]*)*(\\<\\w*\\>)[[:space:]]*(<[^;:{]+>[[:space:]]*)?(\\{|:[^;\\{()]*\\{)"); 
	boost::regex expression("^link info\\: (\\w+) @(\\d+)-(\\d+)");
	string line = onelineOfPDTB;
	string::const_iterator start, end; 
	start = line.begin(); 
	end = line.end(); 
    boost::match_results<string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default;		

	if (regex_search(start, end, what, expression, flags)) { 
		// what[0] contains the string pattern matching the whole reg expression
		// what[1] contains the word 
		// what[2] contains the sent index 
		// what[3] contains the position 
		
		/*
		std::cout << "what-0: "+ string(what[0].first, what[0].second) + "\n";
		std::cout << "what-1: "+ string(what[1].first, what[1].second) + "\n";
		std::cout << "what-2: "+ string(what[2].first, what[2].second) + "\n";
		std::cout << "what-3: "+ string(what[3].first, what[3].second) + "\n";
		*/
				
		string connWord = string(what[1].first, what[1].second);
		string sentIndex = string(what[2].first, what[2].second);
		string synNodeIndex = string(what[3].first, what[3].second) ;
		Symbol label = Symbol (L"CONN");
		string PDTBindex = docName+"%"+connWord+"%"+sentIndex+"%"+synNodeIndex;
		std::cout << "add key-value pair : " + PDTBindex + "-"  << label << "\n"  ;
		(*_connective_table)[PDTBindex]=label;
	}
}

void PennDiscourseTreebank::findConnLocation (string docName, const char *onelineOfPDTB, TargetConnectives::ExplicitConnectiveDict * connectiveDict){
	//boost::regex expression("^(template[[:space:]]*<[^;:{]+>[[:space:]]*)?(class|struct)[[:space:]]*(\\<\\w+\\>([[:blank:]]*\\([^)]*\\))?[[:space:]]*)*(\\<\\w*\\>)[[:space:]]*(<[^;:{]+>[[:space:]]*)?(\\{|:[^;\\{()]*\\{)"); 
	boost::regex expression("^link info\\: (\\w+) @(\\d+)-(\\d+)");
	string line = onelineOfPDTB;
	string::const_iterator start, end; 
	start = line.begin(); 
	end = line.end(); 
    boost::match_results<string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default;		

	if (regex_search(start, end, what, expression, flags)) { 
		// what[0] contains the string pattern matching the whole reg expression
		// what[1] contains the word 
		// what[2] contains the sent index 
		// what[3] contains the position 
		
		/*
		std::cout << "what-0: "+ string(what[0].first, what[0].second) + "\n";
		std::cout << "what-1: "+ string(what[1].first, what[1].second) + "\n";
		std::cout << "what-2: "+ string(what[2].first, what[2].second) + "\n";
		std::cout << "what-3: "+ string(what[3].first, what[3].second) + "\n";
		*/
				
		string connWord = string(what[1].first, what[1].second);
		string sentIndex = string(what[2].first, what[2].second);
		string synNodeIndex = string(what[3].first, what[3].second) ;
		Symbol label = Symbol (L"CONN");
		string PDTBindex = docName+"%"+connWord+"%"+sentIndex+"%"+synNodeIndex;
		
		
		map <string, int>::iterator myIterator = connectiveDict->find(connWord);

		if(myIterator != connectiveDict->end())
		{	
			SessionLogger::info("SERIF") << "add key-value pair : " + PDTBindex + "-"  << label << "\n"  ;
			(*_connective_table)[PDTBindex]=label;
		}
	}
}

Symbol PennDiscourseTreebank::getLabelofExpConnective(string docName, string word, string sentIndex, string SynNodeId){
	string PDTBindex = docName+"%"+word+"%"+sentIndex+"%"+SynNodeId;
	
	map <string, Symbol>::iterator myIterator = _connective_table->find(PDTBindex);

    if(myIterator != _connective_table->end())
    {
        //Its in the table
		Symbol label = myIterator->second;
		return label;
	}else{
		//throw UnexpectedInputException("invalid key for PDTB dictionary: ", PDTBindex.c_str());
		return Symbol(L"-EMPTY-");
	}
}

Symbol PennDiscourseTreebank::getLabelofCrossSentRel(string docName, string sentIndex){
	string PDTBindex = docName+"%"+sentIndex;
	CrossSentRelationTable::iterator myIterator = _crossSentRel_table->find(PDTBindex);
	if (myIterator != _crossSentRel_table->end()){
		return Symbol(L"CS_REL");
	}else{
		return Symbol(L"-EMPTY-");
	}
}

vector<CrossSentRelation>* PennDiscourseTreebank::getCrossSentRel(string docName, string sentIndex){
	string PDTBindex = docName+"%"+sentIndex;
	CrossSentRelationTable::iterator myIterator = _crossSentRel_table->find(PDTBindex);
	if (myIterator != _crossSentRel_table->end()){
		return &(*_crossSentRel_table)[PDTBindex];
	}else{
		return 0;
	}
}

void PennDiscourseTreebank::finalize()
{
	if (_connective_table != 0)
	{
	    delete _connective_table;
	    _connective_table = 0;
	}

	if (_crossSentRel_table != 0)
	{
		delete _crossSentRel_table;
		_crossSentRel_table = 0;
	}
}		

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/NameClassTags.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/theories/EntityType.h"
#include <boost/scoped_ptr.hpp>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


/**
 * NOT USED IN SERIF: initializes NameClassTags from file
 */

NameClassTags::NameClassTags(const char* name_class_file,bool complex) {
	_startInMiddleTagArray = 0;
	trueStartTagArray = 0;
	boost::scoped_ptr<UTF8InputStream> nameClassStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& nameClassStream(*nameClassStream_scoped_ptr);
	nameClassStream.open(name_class_file);

	START = Symbol(L"START");
	CONTINUE = Symbol(L"CONTINUE");

	_n_nestable_tags = 0;
	int numFileTags;
	nameClassStream >> numFileTags;
	Symbol *tempTags = _new Symbol[numFileTags];
	if (complex) { //use standalone format
		std::wstring linetext; //can't use UTF8Token here since it breaks on whitespace
		nameClassStream.getLine(linetext); //get rid of extra \n
		for (int i = 0; i < numFileTags; i++) {
			nameClassStream.getLine(linetext);
			tempTags[i] = linetext.c_str();
		}
		initializeComplexNameClassTagArrays(tempTags, numFileTags);}
	else { //use default serif format
		for (int i = 0; i < numFileTags; i++) {
			UTF8Token token;
			nameClassStream >> token;
			tempTags[i] = token.symValue();
		}
		initializeNameClassTagArrays(tempTags, numFileTags);
	}
	nameClassStream.close();
	delete [] tempTags;

}


/**
 * USED IN SERIF: initializes NameClassTags from EntityType
 */

NameClassTags::NameClassTags() 
{
	_startInMiddleTagArray = 0;
	trueStartTagArray = 0;

	START = Symbol(L"START");
	CONTINUE = Symbol(L"CONTINUE");
	initializeNameClassTagArrays();
}




/*	Used in standalone IdF
	expects text like <ENAMEX TYPE="PER">
*/
NameClassTags::tagInfoType * NameClassTags::getTagInfoFromOpening(wchar_t * buffer) {
#ifdef _DEBUG
	printf("Determining tag info in line: %S\n",buffer);
#endif
	tagInfoType * tag = _new tagInfoType;
	tag->attributes=_new std::vector<tagAttributesType *>;
	std::wstring delimiters(L" \"<>=");
	std::wstring tagstring(buffer);
	std::vector<Symbol *> tokens;
	size_t pos=0;
	size_t newpos=0;
	while (newpos!=std::wstring::npos && pos < tagstring.length()) {
		newpos=tagstring.find_first_not_of(delimiters,pos+1);
		if (newpos!=std::wstring::npos) {
			pos=newpos;
			newpos=tagstring.find_first_of(delimiters,pos);
			tokens.insert(tokens.end(),new Symbol(tagstring.substr(pos,newpos-pos).c_str()));
			pos=newpos;
		}
	}
	tag->tag = tokens.at(0);
	for (size_t i=1;i<tokens.size();i=i+2) {
		if (i+1>=tokens.size()) {
			std::stringstream errMsg;
			errMsg << "The name class tags file " << UnicodeUtil::toUTF8StdString(buffer) << " appears to be malformed";
			throw UnexpectedInputException("Name Class Tags File", errMsg.str().c_str());
		}
		tagAttributesType * att = _new tagAttributesType;
		att->name=tokens.at(i);
		att->value=tokens.at(i+1);
		tag->attributes->insert(tag->attributes->end(),att);
	}
	return tag;
}

void NameClassTags::initializeNameClassTagArrays(Symbol *tags, int numFileTags) {
	// each type gets a solo instance
	// NOTE: nests NOT by this version!
	
	// every other type, for both st and co, plus start and end.
	_n_file_tags = numFileTags;
	_n_tags = (numFileTags * 2) + 2 + 2;

	_tags = _new MultiTag*[_n_tags];
	// only pure starts, that is to say top level st (no stst here)
	trueStartTagArray = _new int[_n_tags/2];

	int index = 0;
	int startTagIndex = 0;
	// because no nest is allowed parent is set as -1
	for (int i = 0; i < numFileTags; i++) {
		wchar_t buffer[100];

		int cont_idx = index;
		swprintf(buffer, 100, L"%ls-CO", tags[i].to_string());
		_tags[index] = _new MultiTag(Symbol(buffer), tags[i], false, -1, cont_idx);		
		index++;		

		swprintf(buffer, 100, L"%ls-ST", tags[i].to_string());
		_tags[index] = _new MultiTag(Symbol(buffer), tags[i], true, -1, cont_idx);
		trueStartTagArray[startTagIndex++] = index;
		index++;
	}

	_none_continue_index = index;
	_tags[index] = _new MultiTag(Symbol(L"NONE-CO"), Symbol(L"NONE"), false, -1, _none_continue_index);
	index++;

	_tags[index] = _new MultiTag(Symbol(L"NONE-ST"), Symbol(L"NONE"), true, -1, _none_continue_index); 
	_none_start_index = index;
	trueStartTagArray[startTagIndex++] = index;
	index++;

	_tags[index] = _new MultiTag(Symbol(L"START"), true);
	_start_index = index++;
	_tags[index] = _new MultiTag(Symbol(L"END"), false);
	_end_index = index++;
	_n_start_tags = startTagIndex;
}


//This method for standalone idf, expects NCs in format as: PERSON|<ENAMEX TYPE="PER">|</ENAMEX>
//To allow a type to be nestable, add "NEST" as a field at end: PERSON|<ENAMEX TYPE="PER">|</ENAMEX>|NEST
void NameClassTags::initializeComplexNameClassTagArrays(Symbol *tags, int numFileTags) {
	// figure out how many tags we need by making a first pass look for
	// nest fields
	int numNestable = 0;
	int i;
	for (i = 0; i < numFileTags; i++) {
		std::wstring line = tags[i].to_string();
		std::wstring::size_type num = line.find(L"|NEST");
		if (num > 1)
			numNestable++;

	}
	_n_nestable_tags = numNestable;

	// each type gets a solo instance for st and co. If a type is nestable
	// it gets an instance paired with
	// every other type, for both st and co, plus the special -stst for 
	// a nest starting a top-level. And then there's none-st -co.
	// and then there's start and end.
	_n_file_tags = numFileTags;
	_n_tags = 2*numFileTags + (3*numNestable*(numFileTags-1))+2+2;
	_tags = _new MultiTag*[_n_tags];


	bool complexformat=true; //expects a name class|tagopening|tagclosing
	// each type has one top level st. each nest has n-1. and none.
	// and start.
	trueStartTagArray = _new int[numFileTags+(numNestable*(numFileTags-1))+2];
	// each type has a nesting mid start for each nestable type
	// we index by raw type, though
	_startInMiddleTagArray = _new int*[numFileTags];
	for (i = 0; i < numFileTags; i++) {
		_startInMiddleTagArray[i] = _new int[numNestable];
		for (int j = 0; j < numNestable; j++)
			_startInMiddleTagArray[i][j] = -1;
	}
	int startTagIndex = 0;
	Symbol* continueArray = _new Symbol[numFileTags];
	int* continueIndexArray = _new int[numFileTags];
	int continueArrayIndex = 0;


	_tagopenings = _new Symbol[_n_tags]; //symbols like <ENAMEX TYPE="GPE">
	_tagclosings = _new Symbol[_n_tags]; //symbols like </ENAMEX>
	_tagInfo = _new tagInfoType*[_n_tags]; //struct defined in NameClassTags.h
	tagInfoType* tempTagInfo = 0;
	// a place to store the symbols and nest status after the 
	// first iteration/file processing
	Symbol* fileTagList = _new Symbol[numFileTags];
	// so that nests can find the openings/closings later
	Symbol* localOpenings = _new Symbol[numFileTags];
	Symbol* localClosings = _new Symbol[numFileTags];
	bool* isNestList = _new bool[numFileTags];
	Symbol tagopening(L"");
	Symbol tagclosing(L"");
	int index = 0;
	
	wchar_t * buffer=_new wchar_t[100];
	// first pass - all the top-level tags. Also save indices to the appropriate continues
	// also create the fileTagList for the next time through
	for (i = 0; i < numFileTags; i++) {
		_tags[index] = _new MultiTag();

		std::wstring line = tags[i].to_string();
		size_t pipelocation=line.find(L"|");
		if (complexformat && pipelocation<1) {
			pipelocation=line.size();
			complexformat=false;
			std::cout<<"Using simple format in Name Class Tags\n";
		}
		
		swprintf(buffer, 100, L"%ls", line.substr(0,pipelocation).c_str());
		fileTagList[i] = _tags[index]->reduced = _tags[index]->semiReduced = _tags[index]->parent = Symbol(buffer);
		isNestList[i] = false; // set true perhaps below	
		if (complexformat) {
			size_t nextpipelocation=line.find(L"|",pipelocation+1); //find second pipe
			if (nextpipelocation<1) {
				std::cerr << "Unexpected format for Name Class Tags (2)\n";
				std::string err = "Unexpected format for Name Class Tags";
				throw UnexpectedInputException("NameClassTags()", (char*)err.c_str());
			}
			swprintf(buffer,100, L"%ls",line.substr(pipelocation+1,nextpipelocation-pipelocation-1).c_str());
			// load in tag info
			tempTagInfo=getTagInfoFromOpening(buffer);
			tempTagInfo->tagnumber=index;
			_tagInfo[index]=tempTagInfo;
			// load in opening
			tagopening=(buffer);
			localOpenings[i]=_tagopenings[index]=tagopening;
			// set nest info (affects how closing is grabbed
			size_t nestLocation = line.find(L"|NEST", nextpipelocation+1);
			if (nestLocation >= 1 && nestLocation <= line.length()) {
				isNestList[i] = true;
				swprintf(buffer,100, L"%ls",line.substr(nextpipelocation+1,nestLocation-nextpipelocation-1).c_str());
			}
			else {
				swprintf(buffer,100, L"%ls",line.substr(nextpipelocation+1,line.size()-nextpipelocation-1).c_str());
			}
			// load in closing
			tagclosing=(buffer);
			localClosings[i]=_tagclosings[index]=tagclosing;
			
		} //end if in tag|opentag|closetag
		// fill out tag
		swprintf(buffer, 100, L"%ls-CO",  line.substr(0,pipelocation).c_str());
		_tags[index]->tag = Symbol(buffer);
		_tags[index]->isStart = false;
		_tags[index]->isContinue = true;
		int cont_idx = index;
		_tags[index]->continueIndex = _tags[index]->topContinueIndex = cont_idx;
		_tags[index]->parentTypeIndex = i;
		// save off top-level continue index for later
		continueIndexArray[continueArrayIndex] = index;
		continueArray[continueArrayIndex++] = fileTagList[i];

		index++;

		// fill out tag
		_tags[index] = _new MultiTag();
		swprintf(buffer, 100, L"%ls", line.substr(0,pipelocation).c_str());
		_tags[index]->reduced = _tags[index]->semiReduced = _tags[index]->parent = Symbol(buffer);
		swprintf(buffer, 100, L"%ls-ST", line.substr(0,pipelocation).c_str());
		_tags[index]->tag = Symbol(buffer);
		_tags[index]->isStart = true;
		_tags[index]->isContinue = false;
		_tags[index]->continueIndex = _tags[index]->topContinueIndex = cont_idx;
		_tags[index]->parentTypeIndex = i;
		// load in opening
		_tagopenings[index]=tagopening;
		// load in closing
		_tagclosings[index]=tagclosing;
		// load in info
		_tagInfo[index]= _new tagInfoType(*tempTagInfo);
		_tagInfo[index]->tagnumber=index;
		trueStartTagArray[startTagIndex++] = index;
		index++;
	}
	// second pass - all the nest tags
	for (i=0; i < numFileTags; i++) {
		if (!isNestList[i])
			continue;
		Symbol type = fileTagList[i];
		wchar_t * partial_buffer=_new wchar_t[100];
		for (int j = 0; j < numFileTags; j++) {
			if (i == j)
				continue;
			Symbol parentType = fileTagList[j];
			// find the top-level continue
			int topContinue = -1;
			int k;
			for (k = 0; k < continueArrayIndex; k++) {
				if (continueArray[k] == parentType) {
					topContinue = continueIndexArray[k];
					break;
				}
			}
			if (topContinue < 0)
				std::cerr << "Error: Can't find continue for " << parentType.to_debug_string() << "\n";
			// save off this level continue index
			int deepContinue = index;
			// same partial buffer used for all 3 of this particular nest
			swprintf(partial_buffer, 100, L"%ls=%ls", type.to_string(), parentType.to_string());
			swprintf(buffer, 100, L"%ls=%ls-CO", type.to_string(), parentType.to_string());
			// fill out tag
			_tags[index] = _new MultiTag(Symbol(buffer), type, parentType, Symbol(partial_buffer), false, true, j, deepContinue, topContinue);
			// opening and closing are associated with the inner tag
			//load in tag info
			swprintf(buffer, 100, L"%ls",localOpenings[i].to_string());
			tempTagInfo=getTagInfoFromOpening(buffer);
			tempTagInfo->tagnumber = index;
			_tagInfo[index] = tempTagInfo;
			// load in opening
			_tagopenings[index]=localOpenings[i];
			// load in closing
			_tagclosings[index]=localClosings[i];
			index++;
			// start of both inner and outer. A "true start"
			swprintf(buffer, 100, L"%ls=%ls-STST", type.to_string(), parentType.to_string());
			_tags[index] = _new MultiTag(Symbol(buffer), type, parentType, Symbol(partial_buffer), true, false, j, deepContinue, topContinue);
			trueStartTagArray[startTagIndex++] = index;
			// opening and closing are associated with the inner tag
			//load in tag info
			_tagInfo[index] = _new tagInfoType(*tempTagInfo);
			_tagInfo[index]->tagnumber = index;
			// load in opening
			_tagopenings[index]=localOpenings[i];
			// load in closing
			_tagclosings[index]=localClosings[i];
			index++;

			// start of inner only. Not a "true start" for decode purposes
			swprintf(buffer, 100, L"%ls=%ls-ST", type.to_string(), parentType.to_string());
			_tags[index] = _new MultiTag(Symbol(buffer), type, parentType, Symbol(partial_buffer), false, false, j, deepContinue, topContinue);
			// opening and closing are associated with the inner tag
			//load in tag info
			_tagInfo[index] = _new tagInfoType(*tempTagInfo);
			_tagInfo[index]->tagnumber = index;
			// load in opening
			_tagopenings[index]=localOpenings[i];
			// load in closing
			_tagclosings[index]=localClosings[i];
			// position in _startInMiddleTagArray doesn't matter - 
			// just put it in the first available space
			for (k = 0; k < numNestable; k++) {
				if (_startInMiddleTagArray[j][k] < 0) {
					_startInMiddleTagArray[j][k] = index;
					break;
				}
			}
			index++;
		}
		delete [] partial_buffer;
	}
	delete [] continueArray;
	delete [] continueIndexArray;
	delete [] localOpenings;
	delete [] localClosings;
	delete [] fileTagList;
	delete [] isNestList;
	delete [] buffer;
	_none_continue_index = index;
	_tags[index] = _new MultiTag(Symbol(L"NONE-CO"), Symbol(L"NONE"), false, -1, _none_continue_index);
	_tagopenings[index]=Symbol(L"");
	_tagclosings[index]=Symbol(L"");
	_tagInfo[index]=NULL;
	index++;
	
	_tags[index] = _new MultiTag(Symbol(L"NONE-ST"), Symbol(L"NONE"), true, -1, _none_continue_index);
	_none_start_index = index;
	_tagopenings[index]=Symbol(L"");
	_tagclosings[index]=Symbol(L"");
	_tagInfo[index]=NULL;
	trueStartTagArray[startTagIndex++] = index;
	index++;
	_tags[index] = _new MultiTag(Symbol(L"START"), true);
	_start_index = index;
	_tagopenings[index]=Symbol(L"");
	_tagclosings[index]=Symbol(L"");
	_tagInfo[index]=NULL;
	index++;
	_tags[index] = _new MultiTag(Symbol(L"END"), false);
	_end_index = index;
	_tagopenings[index]=Symbol(L"");
	_tagclosings[index]=Symbol(L"");
	_tagInfo[index]=NULL;
	index++;	
	_n_start_tags = startTagIndex;
}


/** 
  * The entitytypes way to do it
  */
/**
 * nested version! 
 * initializes name class arrays to (for n types, m nestable):
 * 
 * tags[0]-CO
 * tags[0]-ST
 * ...
 * tags[n]-ST
 * tags[n]-CO
 * tags[0]=tags[1]-CO
 * tags[0]=tags[1]-STST
 * tags[0]=tags[1]-ST
 * ...
 * tags[0]=tags[n]-CO
 * tags[0]=tags[n]-STST
 * tags[0]=tags[n]-ST
 * ...
 * tags[m]=tags[0]-CO
 * tags[m]=tags[0]-STST
 * tags[m]=tags[0]-ST
 * ...
 * tags[m]=tags[n]-CO
 * tags[m]=tags[n]-STST
 * tags[m]=tags[n]-ST
 * NONE-ST
 * NONE-CO
 * START
 * END
 *
 */
void NameClassTags::initializeNameClassTagArrays() {
	// exploratory: determine how large the arrays need to be
	int numFileTags = 0;
	int numNestable = 0;
	int i = 0;
	for (i = 0; i < EntityType::getNTypes(); i++) {
		EntityType type = EntityType::getType(i);
		if (type.isRecognized()) {
			numFileTags++;
			if (type.isNestable())
				numNestable++;
		}
	}
	_n_nestable_tags = numNestable;

	// each type gets a solo instance for st and co. If a type is nestable
	// it gets an instance paired with
	// every other type, for both st and co, plus the special -stst for 
	// a nest starting a top-level. And then there's none-st -co.
	// and then there's start and end.
	_n_file_tags = numFileTags;
	_n_tags = 2*numFileTags + (3*numNestable*(numFileTags-1))+2+2;
	_tags = _new MultiTag*[_n_tags];
	// each type has one top level st. each nest has n-1. and none.
	// and start.
	trueStartTagArray = _new int[numFileTags+(numNestable*(numFileTags-1))+2];
	// each type has a nesting mid start for each nestable type
	// we index by raw type, though
	_startInMiddleTagArray = _new int*[EntityType::getNTypes()];
	for (i = 0; i < EntityType::getNTypes(); i++) {
		_startInMiddleTagArray[i] = _new int[numNestable];
		for (int j = 0; j < numNestable; j++)
			_startInMiddleTagArray[i][j] = -1;
	}
	int index = 0;
	int startTagIndex = 0;
	EntityType* continueArray = _new EntityType[EntityType::getNTypes()];
	int* continueIndexArray = _new int[EntityType::getNTypes()];
	int continueArrayIndex = 0;
	// first pass - all the top-level tags. Also save indices to the appropriate continues
	for (i = 0; i < EntityType::getNTypes(); i++) {
		EntityType type = EntityType::getType(i);
		if (!type.isRecognized())
			continue;
		wchar_t buffer[100];
		swprintf(buffer, 100, L"%ls-CO", type.getName().to_string());
		// save off shallow-level continue index
		int shallowContinue = index;
		// save off top-level continue index for later
		continueIndexArray[continueArrayIndex] = index;
		continueArray[continueArrayIndex++] = type;
		_tags[index] = _new MultiTag(Symbol(buffer), type.getName(), false, i, shallowContinue);
		index++;
		swprintf(buffer, 100, L"%ls-ST", type.getName().to_string());
		_tags[index] = _new MultiTag(Symbol(buffer), type.getName(), true, i, shallowContinue);
		trueStartTagArray[startTagIndex++] = index;
		index++;
	}
	// second pass - all the nest tags
	for (i = 0; i < EntityType::getNTypes(); i++) {
		wchar_t buffer[100];
		EntityType type = EntityType::getType(i);
		if (!type.isRecognized())
			continue;
		if (!type.isNestable())
			continue;
		wchar_t partial_buffer[100];
		for (int j = 0; j < EntityType::getNTypes(); j++) {
			if (i == j)
				continue;
			EntityType parentType = EntityType::getType(j);
			if (!parentType.isRecognized())
				continue;
			// find the top-level continue
			int topContinue = -1;
			int k;
			for (k = 0; k < continueArrayIndex; k++) {
				if (continueArray[k] == parentType) {
					topContinue = continueIndexArray[k];
					break;
				}
			}
			if (topContinue < 0)
				std::cerr << "Error: Can't find continue for " << parentType.getName().to_debug_string() << "\n";

			// save off this level continue index
			int deepContinue = index;				
			swprintf(partial_buffer, 100, L"%ls=%ls", type.getName().to_string(), parentType.getName().to_string());
			swprintf(buffer, 100, L"%ls=%ls-CO", type.getName().to_string(), parentType.getName().to_string());
			_tags[index] = _new MultiTag(Symbol(buffer), type.getName(), parentType.getName(), Symbol(partial_buffer), false, true, j, deepContinue, topContinue);
			index++;


			// start of both inner and outer. A "true start"
			swprintf(partial_buffer, 100, L"%ls=%ls", type.getName().to_string(), parentType.getName().to_string());
			swprintf(buffer, 100, L"%ls=%ls-STST", type.getName().to_string(), parentType.getName().to_string());
			_tags[index] = _new MultiTag(Symbol(buffer), type.getName(), parentType.getName(), Symbol(partial_buffer), true, false, j, deepContinue, topContinue);
			trueStartTagArray[startTagIndex++] = index;
			index++;

			// start of inner only. Not a "true start" for decode purposes
			swprintf(partial_buffer, 100, L"%ls=%ls", type.getName().to_string(), parentType.getName().to_string());
			swprintf(buffer, 100, L"%ls=%ls-ST", type.getName().to_string(), parentType.getName().to_string());
			_tags[index] = _new MultiTag(Symbol(buffer), type.getName(), parentType.getName(), Symbol(partial_buffer), false, false, j, deepContinue, topContinue);
			// position in _startInMiddleTagArray doesn't matter - 
			// just put it in the first available space
			for (k = 0; k < numNestable; k++) {
				if (_startInMiddleTagArray[j][k] < 0) {
					_startInMiddleTagArray[j][k] = index;
					break;
				}
			}
			index++;


		}
	}
	_tags[index] = _new MultiTag(Symbol(L"NONE-CO"), Symbol(L"NONE"), false, -1, index);
	_none_continue_index = index;
	int topCont = index;
	index++;

	_tags[index] = _new MultiTag(Symbol(L"NONE-ST"), Symbol(L"NONE"), true, -1, topCont); 
	_none_start_index = index;

	trueStartTagArray[startTagIndex++] = index;
	index++;
	_tags[index] = _new MultiTag(Symbol(L"START"), true);
	_start_index = index++;
	_tags[index] = _new MultiTag(Symbol(L"END"), false);
	_end_index = index++;
	_n_start_tags = startTagIndex;

	delete [] continueArray;
	delete [] continueIndexArray;
}


int NameClassTags::getIndexForTag(Symbol tag) const {

	// could change to hashmap later?
	for (int i = 0; i < _n_tags; i++) {
		if (tag == _tags[i]->tag)
			return i;
	}
	
	// SRS: I'm not sure what to do about this in the long run.
	// For now, I'll leave the ACE types hard-coded.
	// SERIF hack:
	std::wstring tagstr = tag.to_string();
	if (tagstr.find(L"PERSON") == 0) {
		tagstr.replace(0,6,L"PER");
		return getIndexForTag(Symbol(tagstr.c_str()));
	} else if (tagstr.find(L"ORGANIZATION") == 0) {
		tagstr.replace(0,12,L"ORG");
		return getIndexForTag(Symbol(tagstr.c_str()));
	} else if (tagstr.find(L"LOCATION") == 0) {
		tagstr.replace(0,8,L"LOC");
		return getIndexForTag(Symbol(tagstr.c_str()));
	} 

	return -1;
}



int* NameClassTags::getStartInMiddleTagArray(int tag) const {
	return _startInMiddleTagArray[_tags[tag]->parentTypeIndex];
}

Symbol NameClassTags::getTagStatus(int index) const { 
	if (isContinue(index)) return CONTINUE;
	else return START;
}

/** return true if tag corresponding to index is a start tag */
/** this refers only to "true starts" */
bool NameClassTags::isStart(int index) const { 
	if (index == _start_index || index == _end_index)
		return true;
	return _tags[index]->isStart;
}

/** return true if tag corresponding to index is a continue tag */
bool NameClassTags::isContinue(int index) const { 
	if (index == _start_index || index == _end_index)
		return false;
	return _tags[index]->isContinue;
}
/** return true if tag corresponding to index is a nested type */
bool NameClassTags::isNested(int index) const {
	if (index == _start_index || index == _end_index)
		return false;
	return _tags[index]->isNested;
}

/** return true if tagStatus is the Symbol that we've defined to mean START */
bool NameClassTags::isStart(Symbol tagStatus) const { 
	return (tagStatus == START); 
}

/** return true if tagStatus is the Symbol that we've defined to mean CONTINUE */
bool NameClassTags::isContinue(Symbol tagStatus) const { 
	return (tagStatus == CONTINUE); 
}

/** given a tag index, return the tag index that is the top-level form with -CO */
/** X=Y-ST, X=Y-CO -> Y-CO; X-ST, X-CO -> X-CO */
int NameClassTags::getTopContinueForm(int tag) const {
	return _tags[tag]->topContinueIndex;
}

/** given a tag index, return the tag index that is the form with -CO */
/** X=Y-ST, X=Y-CO -> X=Y-CO; X-ST, X-CO -> X-CO */
int NameClassTags::getContinueForm(int tag) const {
	return _tags[tag]->continueIndex;
}

/** returns the number of names in the sentence theory */
int NameClassTags::getNumNamesInTheory(IdFSentenceTheory *theory) {
	int name_count = 0;
	for (int i = 0; i < theory->getLength(); i++) {
		if (isStart(theory->getTag(i)) && isMeaningfulNameTag(theory->getTag(i)))
			name_count++;
	}
	return name_count;
}

//
// DEBUG PRINT FUNCTIONS
//

/**
* prints out ((word tag)(word tag)(word tag))
*/
std::wstring NameClassTags::to_string(IdFSentenceTokens *sentence, 
									  IdFSentenceTheory *theory)  const
{
	if (sentence->getLength() != theory->getLength())
		return L"theory and sentence of different lengths";

	std::wstring result = L"(";
	for (int i = 0; i < theory->getLength(); i++) {
		result += L"(";
		result += sentence->getWord(i).to_string();
		result += L" ";
		result += getTagSymbol(theory->getTag(i)).to_string();
		result += L")";
	}
	result += L")";
	return result;
}

/**
* prints out: tag tag tag
*/
std::wstring NameClassTags::to_just_reduced_tags(IdFSentenceTokens *sentence, 
												 IdFSentenceTheory *theory)  const
{
	if (sentence->getLength() != theory->getLength())
		return L"theory and sentence of different lengths";

	std::wstring result = L"";
	for (int i = 0; i < theory->getLength(); i++) {
		result += getReducedTagSymbol(theory->getTag(i)).to_string();
		result += L" ";
	}
	return result;
}

/**
* prints out word <enamex type="tag">word</enamex> word
*
* changed default to ENAMEX, 2/20/2003 EMB
*/
std::wstring NameClassTags::to_enamex_sgml_string(IdFSentenceTokens *sentence, 
												  IdFSentenceTheory *theory)  const
{
	if (sentence->getLength() != theory->getLength())
		return L"theory and sentence of different lengths";

	std::wstring result = L"";
	for (int i = 0; i < theory->getLength(); i++) {
		Symbol tag = getReducedTagSymbol(theory->getTag(i));

		// SRS: as above, not sure what to do here, hardcoding for now
		// SERIF hack
		if (tag == Symbol(L"PER"))
			tag = Symbol(L"PERSON");
		else if (tag == Symbol(L"ORG"))
			tag = Symbol(L"ORGANIZATION");
		else if (tag == Symbol(L"LOC"))
			tag = Symbol(L"LOCATION");

		// add a start if we're not purely continuing.
		// add two starts if we're nested and the last token
		// doesn't have the same parent
		if (!isContinue(theory->getTag(i)) &&
			theory->getTag(i) < getNoneStartTagIndex() &&
			theory->getTag(i) < getNoneContinueTagIndex()) {
				// are we nested and in the special start state?
				if (isNested(theory->getTag(i)) && (isStart(theory->getTag(i)))) {
					Symbol parTag = getParentTagSymbol(theory->getTag(i));
					// SERIF hack
					if (parTag == Symbol(L"PER"))
						parTag = Symbol(L"PERSON");
					else if (parTag == Symbol(L"ORG"))
						parTag = Symbol(L"ORGANIZATION");
					else if (parTag == Symbol(L"LOC"))
						parTag = Symbol(L"LOCATION");

					if (parTag == Symbol(L"PERSON") ||
						parTag == Symbol(L"ORGANIZATION") ||
						parTag == Symbol(L"LOCATION") ||
						parTag == Symbol(L"FAC") ||
						parTag == Symbol(L"GPE") ||
						parTag == Symbol(L"PER_DESC") ||
						parTag == Symbol(L"ORG_DESC") ||
						parTag == Symbol(L"PRODUCT_DESC") ||
						parTag == Symbol(L"FAC_DESC") ||
						parTag == Symbol(L"GPE_DESC") ||
						parTag == Symbol(L"ANIMAL") ||
						parTag == Symbol(L"CONTACT_INFO") ||
						parTag == Symbol(L"DISEASE") ||
						parTag == Symbol(L"EVENT") ||
						parTag == Symbol(L"GAME") ||
						parTag == Symbol(L"LANGUAGE") ||
						parTag == Symbol(L"LAW") ||
						parTag == Symbol(L"NATIONALITY") ||
						parTag == Symbol(L"PLANT") ||
						parTag == Symbol(L"PRODUCT") ||
						parTag == Symbol(L"SUBSTANCE") ||
						parTag == Symbol(L"SUBSTANCE:NUCLEAR") ||
						parTag == Symbol(L"WORK_OF_ART"))
						result += L"<ENAMEX TYPE=\"";
					else if (parTag == Symbol(L"TIME") ||
						parTag == Symbol(L"DATE") ||
						parTag == Symbol(L"DATE:duration") ||
						parTag == Symbol(L"DATE:age") ||
						parTag == Symbol(L"DATE:date") ||
						parTag == Symbol(L"DATE:other"))
						result += L"<TIMEX TYPE=\"";
					else if (parTag == Symbol(L"MONEY") ||
						parTag == Symbol(L"PERCENT") ||
						parTag == Symbol(L"CARDINAL") ||
						parTag == Symbol(L"ORDINAL") ||
						parTag == Symbol(L"QUANTITY"))
						result += L"<NUMEX TYPE=\"";
					else result += L"<ENAMEX TYPE=\"";
					result += parTag.to_string();
					result += L"\">";
				}

				if (tag == Symbol(L"PERSON") ||
					tag == Symbol(L"ORGANIZATION") ||
					tag == Symbol(L"LOCATION") ||
					tag == Symbol(L"FAC") ||
					tag == Symbol(L"GPE") ||
					tag == Symbol(L"PER_DESC") ||
					tag == Symbol(L"ORG_DESC") ||
					tag == Symbol(L"PRODUCT_DESC") ||
					tag == Symbol(L"FAC_DESC") ||
					tag == Symbol(L"GPE_DESC") ||
					tag == Symbol(L"ANIMAL") ||
					tag == Symbol(L"CONTACT_INFO") ||
					tag == Symbol(L"DISEASE") ||
					tag == Symbol(L"EVENT") ||
					tag == Symbol(L"GAME") ||
					tag == Symbol(L"LANGUAGE") ||
					tag == Symbol(L"LAW") ||
					tag == Symbol(L"NATIONALITY") ||
					tag == Symbol(L"PLANT") ||
					tag == Symbol(L"PRODUCT") ||
					tag == Symbol(L"SUBSTANCE") ||
					tag == Symbol(L"SUBSTANCE:NUCLEAR") ||
					tag == Symbol(L"WORK_OF_ART"))
					result += L"<ENAMEX TYPE=\"";
				else if (tag == Symbol(L"TIME") ||
					tag == Symbol(L"DATE") ||
					tag == Symbol(L"DATE:duration") ||
					tag == Symbol(L"DATE:age") ||
					tag == Symbol(L"DATE:date") ||
					tag == Symbol(L"DATE:other"))
					result += L"<TIMEX TYPE=\"";
				else if (tag == Symbol(L"MONEY") ||
					tag == Symbol(L"PERCENT") ||
					tag == Symbol(L"CARDINAL") ||
					tag == Symbol(L"ORDINAL") ||
					tag == Symbol(L"QUANTITY"))
					result += L"<NUMEX TYPE=\"";
				else result += L"<ENAMEX TYPE=\"";
				result += tag.to_string();
				result += L"\">";
			}

			result += sentence->getWord(i).to_string();
			// end the span if we're at the end of the sentence
			// or if the token is a nest and the next one is not
			// or if the token is a nest and the next one is a mid-start
			// or if the next token is a true start
			// add a double end in that last case for the parent
			if (theory->getTag(i) < getNoneStartTagIndex() &&
				theory->getTag(i) < getNoneContinueTagIndex() &&
				(i == theory->getLength() - 1 || 
				(isNested(theory->getTag(i)) && !isNested(theory->getTag(i+1))) ||
				isStart(theory->getTag(i+1)) ||
				(isNested(theory->getTag(i)) && !isContinue(theory->getTag(i+1)))))
			{
				if (tag == Symbol(L"PERSON") ||
					tag == Symbol(L"ORGANIZATION") ||
					tag == Symbol(L"LOCATION") ||
					tag == Symbol(L"FAC") ||
					tag == Symbol(L"GPE") ||
					tag == Symbol(L"PER_DESC") ||
					tag == Symbol(L"ORG_DESC") ||
					tag == Symbol(L"PRODUCT_DESC") ||
					tag == Symbol(L"FAC_DESC") ||
					tag == Symbol(L"GPE_DESC") ||
					tag == Symbol(L"ANIMAL") ||
					tag == Symbol(L"CONTACT_INFO") ||
					tag == Symbol(L"DISEASE") ||
					tag == Symbol(L"EVENT") ||
					tag == Symbol(L"GAME") ||
					tag == Symbol(L"LANGUAGE") ||
					tag == Symbol(L"LAW") ||
					tag == Symbol(L"NATIONALITY") ||
					tag == Symbol(L"PLANT") ||
					tag == Symbol(L"PRODUCT") ||
					tag == Symbol(L"SUBSTANCE") ||
					tag == Symbol(L"SUBSTANCE:NUCLEAR") ||
					tag == Symbol(L"WORK_OF_ART"))
					result += L"</ENAMEX>";
				else if (tag == Symbol(L"TIME") ||
					tag == Symbol(L"DATE") ||
					tag == Symbol(L"DATE:duration") ||
					tag == Symbol(L"DATE:age") ||
					tag == Symbol(L"DATE:date") ||
					tag == Symbol(L"DATE:other"))
					result += L"</TIMEX>";
				else if (tag == Symbol(L"MONEY") ||
					tag == Symbol(L"PERCENT") ||
					tag == Symbol(L"CARDINAL") ||
					tag == Symbol(L"ORDINAL") ||
					tag == Symbol(L"QUANTITY"))
					result += L"</NUMEX>";
				else {
					result += L"</ENAMEX>";
				}
				if (isNested(theory->getTag(i)) && 
					isStart(theory->getTag(i+1))) {
						Symbol parTag = getParentTagSymbol(theory->getTag(i));
						// SERIF hack
						if (parTag == Symbol(L"PER"))
							parTag = Symbol(L"PERSON");
						else if (parTag == Symbol(L"ORG"))
							parTag = Symbol(L"ORGANIZATION");
						else if (parTag == Symbol(L"LOC"))
							parTag = Symbol(L"LOCATION");

						if (parTag == Symbol(L"PERSON") ||
							parTag == Symbol(L"ORGANIZATION") ||
							parTag == Symbol(L"LOCATION") ||
							parTag == Symbol(L"FAC") ||
							parTag == Symbol(L"GPE") ||
							parTag == Symbol(L"PER_DESC") ||
							parTag == Symbol(L"ORG_DESC") ||
							parTag == Symbol(L"PRODUCT_DESC") ||
							parTag == Symbol(L"FAC_DESC") ||
							parTag == Symbol(L"GPE_DESC") ||
							parTag == Symbol(L"ANIMAL") ||
							parTag == Symbol(L"CONTACT_INFO") ||
							parTag == Symbol(L"DISEASE") ||
							parTag == Symbol(L"EVENT") ||
							parTag == Symbol(L"GAME") ||
							parTag == Symbol(L"LANGUAGE") ||
							parTag == Symbol(L"LAW") ||
							parTag == Symbol(L"NATIONALITY") ||
							parTag == Symbol(L"PLANT") ||
							parTag == Symbol(L"PRODUCT") ||
							parTag == Symbol(L"SUBSTANCE") ||
							parTag == Symbol(L"SUBSTANCE:NUCLEAR") ||
							parTag == Symbol(L"WORK_OF_ART"))
							result += L"</ENAMEX>";
						else if (parTag == Symbol(L"TIME") ||
							parTag == Symbol(L"DATE") ||
							parTag == Symbol(L"DATE:duration") ||
							parTag == Symbol(L"DATE:age") ||
							parTag == Symbol(L"DATE:date") ||
							parTag == Symbol(L"DATE:other"))
							result += L"</TIMEX>";
						else if (parTag == Symbol(L"MONEY") ||
							parTag == Symbol(L"PERCENT") ||
							parTag == Symbol(L"CARDINAL") ||
							parTag == Symbol(L"ORDINAL") ||
							parTag == Symbol(L"QUANTITY"))
							result += L"</NUMEX>";
						else {
							result += L"</ENAMEX>";
						}
					}

			}
			result += L" ";

	}
	return result;
}

/**
* prints out: word <tag>word</tag> word
*/
std::wstring NameClassTags::to_sgml_string(IdFSentenceTokens *sentence, 
										   IdFSentenceTheory *theory) const
{
	if (sentence->getLength() != theory->getLength())
		return L"theory and sentence of different lengths";

	std::wstring result = L"";
	for (int i = 0; i < theory->getLength(); i++) {
		Symbol tag = getSemiReducedTagSymbol(theory->getTag(i));
		if (isStart(theory->getTag(i)) && 
			theory->getTag(i) < getNoneStartTagIndex() && 
			theory->getTag(i) < getNoneContinueTagIndex()) {
			result += L"<";
			result += tag.to_string();
			result += L">";
		}
		result += sentence->getWord(i).to_string();
		if ((i == theory->getLength() - 1 || isStart(theory->getTag(i+1))) &&
			theory->getTag(i) < getNoneStartTagIndex() &&
			theory->getTag(i) < getNoneContinueTagIndex()) {
				result += L"</";
				result += tag.to_string();
				result += L">";
			}
			result += L" ";

	}
	return result;

}

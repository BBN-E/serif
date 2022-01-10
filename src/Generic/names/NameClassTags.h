// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAME_CLASS_TAGS_H
#define NAME_CLASS_TAGS_H

#include <vector>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/names/IdFSentenceTheory.h"
#include "Generic/names/IdFSentenceTokens.h"

/**
 *  Provides storage for name class tags and easy access to their many forms/attributes.
 *  Each full tag corresponds to an integer. 
 *  Full tag Symbols (e.g. PERSON-ST, NONE-CO) are stored and indexed in _tags. 
 *  Reduced tag Symbols (e.g. PERSON, NONE) are stored and indexed in _reducedTags.
 *  Every other thing you could want to know about tags is managed here as well.
 */
class NameClassTags {
public:
	// to simplify the reduced thing
	class MultiTag {
	public:
		Symbol tag;
		Symbol reduced;
		Symbol parent;
		Symbol semiReduced;
		// mid-start things are considered neither start nor continue
		bool isStart;
		bool isContinue;
		bool isNested;
		int topContinueIndex;
		int continueIndex;
		// based on the order of the pure tag types
		int parentTypeIndex;
		MultiTag(Symbol t, Symbol r, Symbol p, Symbol sr, bool is_start, bool is_continue, int parIndex, int cont = -1, int topCont = -1) {
			tag = t; reduced = r; parent = p; semiReduced = sr;
			parentTypeIndex = parIndex;
			isStart = is_start;
			isContinue = is_continue;
			isNested = true;
			topContinueIndex = topCont;
			continueIndex = cont;
		}
		MultiTag(Symbol t, Symbol r, bool start=false, int parIndex=-1, int cont = -1) {
			reduced = parent = semiReduced = r;
			tag = t;
			parentTypeIndex = parIndex;
			isStart = start;
			isContinue = !start;
			isNested = false;
			continueIndex = topContinueIndex = cont;

		}
		MultiTag(Symbol t, bool start=false, int parIndex=-1, int cont = -1) {
			reduced = parent = semiReduced = tag = t;
			parentTypeIndex = parIndex;
			isStart = start;
			isContinue = !start;
			isNested = false;
			continueIndex = topContinueIndex = cont;
		}
		MultiTag() {
			isStart = false;
			isContinue = false;
			isNested = false;
			topContinueIndex = continueIndex = parentTypeIndex = -1;
		}
		~MultiTag() {}
	};
	//following two inner classes used exclusively in standalone IdF
	class tagAttributesType {//used to store tag attribute info, as in ATTR=VAL. EX: TYPE="GPE"
	public:
		Symbol *name;
		Symbol * value;
		tagAttributesType() {}
		~tagAttributesType() {
			delete name;
			delete value;
		}
	};

	class tagInfoType {//stores vector of attributes for tag, symbol for tag. Looked up by tag number, used for identifying annotated tags in training and for creating decoder output.
	public:
		Symbol * tag;
		int tagnumber;
		std::vector<tagAttributesType *> * attributes;
		tagInfoType() {
			tag=NULL;
			attributes=NULL;
			tagnumber=-1;
		}
		~tagInfoType() {
			if (tag!=NULL) {
				delete tag;
			}
			if (attributes!=NULL) {
				for (size_t i=0; i < attributes->size() ; i++) {
					delete attributes->at(i);
				}
				delete attributes;
			}
		}
	    tagInfoType& operator=(tagInfoType &t) {
			tag=new Symbol(t.tag->to_string());
			tagnumber=-1;
			attributes=t.attributes;
			return *this;
		}

	};

private:
	MultiTag **_tags;

	Symbol *_tagopenings; //used in standalone idf - array of symbols like <ENAMEX TYPE="GPE">
	Symbol *_tagclosings; //used in standalone idf - array of symbols like </ENAMEX>
	tagInfoType **_tagInfo; //used in standalone idf - array of pointers to type defined above

	// 2 level - first indexed by parent, then by child
	int **_startInMiddleTagArray;

	int _n_tags;
	int _n_file_tags;
	int _n_start_tags;
	int _n_nestable_tags;
	int _none_start_index;
	int _none_continue_index;
	Symbol CONTINUE;	
	Symbol START;
	int _start_index;
	int _end_index;

	void initializeNameClassTagArrays(Symbol *tags, int numFileTags);
	void initializeNameClassTagArrays();
	//This method for standalone idf, expects NCs in format as: PERSON|<ENAMEX TYPE="PER">|</ENAMEX>
	void initializeComplexNameClassTagArrays(Symbol *tags, int numFileTags);

public:
	//default false value in constructor forces using standard serif simple NCs file format. Complex form is for standalone IdF
	NameClassTags();
	NameClassTags(const char *name_class_file,bool complex=false);

	int getNumNamesInTheory(IdFSentenceTheory *theory);
	// start tag array is used by decoder efficiently. a list of all start tag indices;
	// start-in-middle tags don't count.
	int *trueStartTagArray;


	// BASIC ACCESS FUNCTIONS
	/** given an integer, return corresponding full tag (e.g.: PERSON-ST) */
	Symbol getTagSymbol(int index) const { 
		if (index >= 0 && index < _n_tags)
			return _tags[index]->tag; 
		else if (index >= 0) 
			throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getTagSymbol()", _n_tags, index);
		else throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getTagSymbol()", 0, index); 
	}
	/** given an integer, return corresponding reduced tag (e.g.: PERSON)
	    if the tag is nested, the inner type is returned 
		(e.g. GPE=LOC-ST -> GPE)*/
	Symbol getReducedTagSymbol(int index) const { 	
		if (index >= 0 && index < _n_tags)
			return _tags[index]->reduced; 
		else if (index >= 0) 
			throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getReducedTagSymbol()", _n_tags, index);
		else throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getReducedTagSymbol()", 0, index); 
	}

	/** given an integer, return corresponding semi-reduced tag 
		(e.g.: PER=ORG from PER=ORG-ST). If the tag is not nested
		semi-reduced is the same as reduced. */
	Symbol getSemiReducedTagSymbol(int index) const { 	
		if (index >= 0 && index < _n_tags)
			return _tags[index]->semiReduced; 
		else if (index >= 0) 
			throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getSemiReducedTagSymbol()", _n_tags, index);
		else throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getSemiReducedTagSymbol()", 0, index); 
	}
	/** given an integer, return corresponding parent tag 
		(e.g.: ORG from PER=ORG-ST). If the tag is not nested
		parent is the same as reduced. */
	Symbol getParentTagSymbol(int index) const { 	
		if (index >= 0 && index < _n_tags)
			return _tags[index]->parent; 
		else if (index >= 0) 
			throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getParentTagSymbol()", _n_tags, index);
		else throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getParentTagSymbol()", 0, index); 
	}

	//begin standalone IdF methods
	/** given an integer, return pointer to reduced tag (e.g.: PERSON) */
	Symbol * getReducedTagPointer(int index) const { 	
		if (index >= 0 && index < _n_tags)
			return &(_tags[index]->reduced); 
		else if (index >= 0) 
			throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getReducedTagSymbol()", _n_tags, index);
		else throw InternalInconsistencyException::arrayIndexException(
			"NameClassTags::getReducedTagSymbol()", 0, index); 
	}

	Symbol * getTagOpening(int index) const {
		return &_tagopenings[index];
	}
	Symbol * getTagClosing(int index) const {
		return &_tagclosings[index];
	}
	tagInfoType * getTagInfo(int index) const {
		return _tagInfo[index];
	}
	// parent open/close/info methods allow logic of output for commercial idf
	Symbol * getParentTagOpening(int index) const {
		return &_tagopenings[_tags[index]->topContinueIndex];
	}
	Symbol * getParentTagClosing(int index) const {
		return &_tagclosings[_tags[index]->topContinueIndex];
	}
	tagInfoType * getParentTagInfo(int index) const {
		return _tagInfo[_tags[index]->topContinueIndex];
	}

	tagInfoType * getTagInfoFromOpening(wchar_t * buffer);

	int getStartIndex() {
		return _start_index;
	}
	int getEndIndex() {
		return _end_index;
	}

	//end standalone IdF methods

	// SPECIAL ACCESS FUNCTIONS
	/** return total number of tags, including START and END */
	int getNumTags() const { return _n_tags; }

	/** return number of "regular" tags (i.e.: not START or END) */
	int getNumRegularTags() const { return _n_tags - 2; }

	/** return number of -ST tags */
	int getNumStartTags() const { return _n_start_tags; }

	/** return number of general tags (not st/end) that can be inside other tags */
	int getNumNestableTags() const { return _n_nestable_tags; }

	/** return integer corresponding to NONE-ST (our default tag for unknown things) */
	int getNoneStartTagIndex() const { return _none_start_index; }

	/** return integer corresponding to NONE-CO (our default tag for unknown things) */
	int getNoneContinueTagIndex() const { return _none_continue_index; }

	/** return Symbol for NONE-ST (our default tag for unknown things) */
	Symbol getNoneStartTag() const { return _tags[getNoneStartTagIndex()]->tag; }

	/** return Symbol for NONE-CO (our default tag for unknown things) */
	Symbol getNoneContinueTag() const { return _tags[getNoneContinueTagIndex()]->tag; }

	/** given a tag Symbol (e.g.: PERSON-ST), return the integer that corresponds to it. */
	int getIndexForTag(Symbol tag) const;

	/** given a tag index, return the tag index that is the top-level form with -CO */
	int getTopContinueForm(int tag) const;

	/** given a tag index, return the tag index that is the form with -CO */
	int getContinueForm(int tag) const;

	/** given a tag index, return an array of ints that are middle starters with the tag's parent */
	int* getStartInMiddleTagArray(int tag) const;

	/** returns true if tag is an actual NAME tag 
	    (i.e.: not any type of NONE, START, or END) */
	bool isMeaningfulNameTag(int index) const { return (index < getNoneStartTagIndex() && index < getNoneContinueTagIndex()); }
		
	/** return Symbol corresponding to tag "status" (i.e.: START or CONTINUE) */
	Symbol getTagStatus(int index) const;
	bool isStart(int index) const;
	bool isContinue(int index) const;
	bool isNested(int index) const;
	bool isStart(Symbol tagStatus) const;
	bool isContinue(Symbol tagStatus) const;

	/** return Symbol for sentence start tag */
	Symbol getSentenceStartTag() const { return _tags[_start_index]->tag; }
	/** return integer corresponding to sentence start tag */
	Symbol getSentenceEndTag() const { return _tags[_end_index]->tag; }
	/** return Symbol corresponding to sentence start tag */
	int getSentenceStartIndex() const { return _start_index; }
	/** return integer corresponding to sentence end tag */
	int getSentenceEndIndex() const { return _end_index; }

	std::wstring to_string(IdFSentenceTokens *sentence, IdFSentenceTheory *theory) const;
	std::wstring to_sgml_string(IdFSentenceTokens *sentence, IdFSentenceTheory *theory) const;
	std::wstring to_enamex_sgml_string(IdFSentenceTokens *sentence, IdFSentenceTheory *theory) const;
	std::wstring to_just_reduced_tags(IdFSentenceTokens *sentence, IdFSentenceTheory *theory) const;


};






#endif

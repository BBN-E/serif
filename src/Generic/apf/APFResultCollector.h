// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef APF_RESULT_COLLECTOR_H
#define APF_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
//#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/OutputStream.h"

class DocTheory;
class Entity;
class EntitySet;
class RelationSet;
class Relation;
class RelMention;
class SynNode;
class TokenSequence;
class UTF8OutputStream;
class Event;
class EventMention;
class EventEntityRelation;

//class OutputStream;


class APFResultCollector : public ResultCollector {
public:
	APFResultCollector() 
		: _docTheory(0), _entitySet(0), _relationSet(0), _tokenSequence(0), 
		  _outputStream(0), _includeXMLHeaderInfo(true), _includeEvents(0),
		  _useAbbreviatedIDs(false)
		 // ,_includeGenericEERelations(true) 
	{}

	~APFResultCollector()
	{	finalize(); }

	virtual void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t *document_filename);

	virtual void produceOutput(std::wstring *results);

	void produceOutput(OutputStream &str);

	void toggleIncludeXMLHeaderInfo(bool newValue) {
		_includeXMLHeaderInfo = newValue;
	}

	void toggleIncludeEvents(bool newValue) {
		_includeEvents = newValue;
	}

	void toggleUseAbbreviatedIDs(bool newValue) {
		_useAbbreviatedIDs = newValue;
	}



private:

	typedef int CompletedFlag;
	static const CompletedFlag COMPLETED = 1;
	static const CompletedFlag ATTEMPTED = 2;
	static const CompletedFlag FAILED = 3;

	void finalize() {
		delete [] _tokenSequence;
		_tokenSequence = 0;
	}

	/***** utility apf methods *****/

	// some constants that apf uses to describe mentions
	const wchar_t* _convertMentionTypeToAPF(Mention* ment);

	// heads the document (write to stream) representing doc_source and 
	// called doc_name
	void _printAPFDocumentHeader(OutputStream& stream,
								 const wchar_t* doc_name=L"DUMMY_NAME",
								 const wchar_t* doc_source=L"DUMMY_SOURCE");

	// begin info for ent, writing to stream, with generic flag and doc name as indicated
	void _printAPFEntityHeader(OutputStream& stream, 
							   class Entity* ent, 
							   bool isGeneric, 
							   const wchar_t* doc_name=L"DUMMY_NAME");
	// begin info for attributes section. write to stream
	void _printAPFAttributeHeader(OutputStream& stream);

	// info for men, a mention of ent, writing to stream
	void _printAPFMention(OutputStream& stream,
						  Mention* ment,
						  class Entity* ent);
	// info for a name mention, as printed in attributes
	void _printAPFNameAttribute(OutputStream& stream,
								Mention* ment);

	// prints text span and character offset
	void _printAPFMentionExtent(OutputStream& stream,
								const SynNode* node,
								int sentNum);

	// RELATIONS
	void _printAPFRelationHeader(OutputStream& stream, 
							   Relation* rel, 
							   const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFRelMention(OutputStream& stream, 
							   RelMention* ment,
							   Relation* rel);

	void _printAPFNodeText(OutputStream& stream, 
		const SynNode* node,
		int sentNum);

	// EVENTS
	void _printExAPFEventHeader(OutputStream& stream,
							Event* event,
							bool isGeneric,
							CompletedFlag completeFlag,
							const wchar_t* doc_name=L"DUMMY_NAME");
	void _printExAPFEventMention(OutputStream& stream, 
							EventMention* eventMention,
							Event* event,
							const wchar_t* doc_name=L"DUMMY_NAME");
	void _printExAPFEERelationHeader(OutputStream& stream, 
							EventEntityRelation* eeRelation, 
							const wchar_t* doc_name=L"DUMMY_NAME");
	void _printExAPFEERelationMention(OutputStream& stream, 
							EventMention* eventMention,
							const Mention* mention, 
							EventEntityRelation* eeRelation);

	void _printExAPFEERelationFooter(OutputStream& stream);
	void _printExAPFEventFooter(OutputStream& stream);
	

	// cleanup - print footers
	void _printAPFAttributeFooter(OutputStream& stream);
	void _printAPFEntityFooter(OutputStream& stream);
	void _printAPFRelationFooter(OutputStream& stream);
	void _printAPFDocumentFooter(OutputStream& stream);

	/***** utility mention information methods *****/

	// is this the top mention of this entity (i.e. not a child of something structured)
	bool _isTopMention(const Mention* ment, EntityType type);

	// is this mention the child of a list that's not being printed? 
	bool _isItemOfUnprintedList(const Mention* ment);

	// is this mention syntactically inside a name mention that is not a GPE?
	bool _isNestedInsideOfName(const Mention* ment);
	bool _isNestedDriver(const SynNode* node, int sentNum);

	// is this mention the second part of an appositive?
	// that is, the latter mention of two
	bool _isSecondPartOfAppositive(const Mention* ment);

	// is the parent of this mention a GPE with a person role?
	bool _isGPEModifierOfPersonGPE(const Mention* ment);

	// is this a name, and is its parent's head not it, and is its
	// parent a different entity? (wordy, I know)
	bool _isNameNotInHeadOfParent(const Mention* ment, EntityType type);

	bool _isPrintableMention(const Mention *ment, class Entity *ent);

	// the data we need
	// may as well save the whole doc theory
	DocTheory *_docTheory;
	const EntitySet *_entitySet;
	const RelationSet *_relationSet;
	// indexed by sentence
	const TokenSequence **_tokenSequence;

	// an override output stream. This allows us to redirect output that would otherwise go to a file.
	OutputStream *_outputStream;
	bool _includeXMLHeaderInfo;
	bool _includeEvents;
	bool _useAbbreviatedIDs;
//	bool _includeGenericEERelations;

	

};

#endif

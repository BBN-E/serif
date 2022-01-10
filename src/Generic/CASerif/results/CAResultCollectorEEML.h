// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_EEML_RESULT_COLLECTOR_H
#define CA_EEML_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/theories/Mention.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"

class DocTheory;
class Entity;
class EntitySet;
class EventSet;
class Event;
class EventMention;
class EventSet;
class EventEntityRelation;
class RelationSet;
class Relation;
class RelMention;
class SynNode;
class TokenSequence;
class UTF8OutputStream;


class CAResultCollectorEEML : public ResultCollector {
public:
	CAResultCollectorEEML() 
		: _docTheory(0), _entitySet(0), _relationSet(0), _eventSet(0), _tokenSequence(0) {}

	~CAResultCollectorEEML()
	{	finalize(); }

	void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t *document_filename);
	virtual void produceOutput(std::wstring *results) {};

private:


	typedef int CompletedFlag;
	static const CompletedFlag COMPLETED = 1;
	static const CompletedFlag ATTEMPTED = 2;
	static const CompletedFlag FAILED = 3;

	void finalize() {
		delete [] _tokenSequence;
		_tokenSequence = 0;
	}

	/***** utility EEML methods *****/

	// some constants that EEML uses to describe mentions
	const wchar_t* _convertMentionTypeToEEML(Mention* ment);

	// heads the document (write to stream) representing doc_source and 
	// called doc_name
	void _printEEMLDocumentHeader(UTF8OutputStream& stream,
								 const wchar_t* doc_name=L"DUMMY_NAME",
								 const wchar_t* doc_source=L"DUMMY_SOURCE", 
								 const wchar_t* doc_path=L"DUMMY_PATH");

	// begin info for ent, writing to stream, with generic flag and doc name as indicated
	void _printEEMLEntityHeader(UTF8OutputStream& stream, 
							   Entity* ent, 
							   bool isGeneric, 
							   bool isGroupFN,
							   const wchar_t* doc_name=L"DUMMY_NAME");
	// begin info for attributes section. write to stream
	void _printEEMLAttributeHeader(UTF8OutputStream& stream);

	// info for men, a mention of ent, writing to stream
	void _printEEMLMention(UTF8OutputStream& stream,
						  Mention* ment,
						  Entity* ent);

	void _printCorrectMention(UTF8OutputStream& stream,
							 CorrectMention* ment,
							 Entity* ent);

	// info for a name mention, as printed in attributes
	void _printEEMLNameAttribute(UTF8OutputStream& stream,
								Mention* ment);

	// prints text span and character offset
	void _printEEMLMentionExtent(UTF8OutputStream& stream,
								const SynNode* node,
								int sentNum);

	void _printCorrectMentionExtent(UTF8OutputStream& stream,
									CorrectMention* cm,
									int sentNum);

	void _printCorrectMentionHeadExtent(UTF8OutputStream& stream,
									CorrectMention* cm,
									int sentNum);

	void _printCorrectMentionText(UTF8OutputStream& stream, 
		CorrectMention *cm,
		int sentNum);

		void _printCorrectMentionHeadText(UTF8OutputStream& stream, 
		CorrectMention *cm,
		int sentNum) ;

	// RELATIONS
	void _printEEMLRelationHeader(UTF8OutputStream& stream, 
							   Relation* rel, 
							   const wchar_t* doc_name=L"DUMMY_NAME");
	void _printEEMLRelMention(UTF8OutputStream& stream, 
							   RelMention* ment,
							   Relation* rel);

	void _printEEMLNodeText(UTF8OutputStream& stream, 
		const SynNode* node,
		int sentNum);
	

	// EVENTS
	void _printEEMLEventHeader(UTF8OutputStream& stream,
							Event* event,
							bool isGeneric,
							CompletedFlag completeFlag,
							const wchar_t* doc_name=L"DUMMY_NAME");
	void _printEEMLEventMention(UTF8OutputStream& stream, 
							EventMention* eventMention,
							Event* event,
							const wchar_t* doc_name=L"DUMMY_NAME");
	void _printEEMLEERelationHeader(UTF8OutputStream& stream, 
							EventEntityRelation* eeRelation, 
							const wchar_t* doc_name=L"DUMMY_NAME");
	void _printEEMLEERelationMention(UTF8OutputStream& stream, 
							EventMention* eventMention,
							const Mention* mention, 
							EventEntityRelation* eeRelation);


	// cleanup - print footers
	void _printEEMLAttributeFooter(UTF8OutputStream& stream);
	void _printEEMLEntityFooter(UTF8OutputStream& stream);
	void _printEEMLRelationFooter(UTF8OutputStream& stream);
	void _printEEMLDocumentFooter(UTF8OutputStream& stream);
	void _printEEMLEventFooter(UTF8OutputStream& stream);
	void _printEEMLEERelationFooter(UTF8OutputStream& stream);

	/***** utility mention information methods *****/

	// is this the top mention of this entity (i.e. not a child of something structured)
	bool _isTopMention(Mention* ment, EntityType type);

	// is this mention syntactically inside a name mention that is not a GPE?
	bool _isNestedInsideOfName(Mention* ment);
	bool _isNestedDriver(const SynNode* node, int sentNum);

	// is this mention the second part of an appositive?
	// that is, the latter mention of two
	bool _isSecondPartOfAppositive(Mention* ment);

	// is the parent of this mention a GPE with a person role?
	bool _isGPEModifierOfPersonGPE(Mention* ment);

	// is this entity plural?
	bool _isPluralEntity(Entity *ent);

	// is this a name, and is its parent's head not it, and is its
	// parent a different entity? (wordy, I know)
	bool _isNameNotInHeadOfParent(Mention* ment, EntityType type);

	// the edt head of a mention takes into account appositiveness and nameness
	const SynNode* _getEDTHead(Mention* ment);

	// the data we need
	// may as well save the whole doc theory
	DocTheory *_docTheory;
	const EntitySet *_entitySet;
	const RelationSet *_relationSet;
	const EventSet *_eventSet;
	CorrectDocument *_correctDocument;

	// indexed by sentence
	const TokenSequence **_tokenSequence;
	
    serif::hash_map<Mention::UID, bool, Mention::UID::HashOp, Mention::UID::EqualOp> _isValidMention;
};

#endif

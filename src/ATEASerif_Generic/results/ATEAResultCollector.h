// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// Copyright (c) 2006 by BBN Technologies, Inc.
// All Rights Reserved.

#ifndef ATEA_RESULT_COLLECTOR_H
#define ATEA_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/OutputStream.h"
#include "Generic/common/SymbolHash.h"
#include "English/clutter/en_ATEAInternalClutterFilter.h"


#define MAX_MENTIONS_PER_DOCUMENT 10000
#define MAX_TOKENS_PER_MENTION 100
#define MAX_TITLES_PER_ENTITY 25

class DocTheory;
class Entity;
class EntitySet;
class ValueSet;
class Value;
class RelationSet;
class Relation;
class RelMention;
class SynNode;
class TokenSequence;
class UTF8OutputStream;
class EventSet;
class Event;
class EventMention;
class Proposition;
class TitleListNode;
class TitleMap;
class ClutterFilter;
class SynNode;

class ATEAResultCollector : public ResultCollector {
public:
	ATEAResultCollector(int mode_);
	virtual ~ATEAResultCollector();

	const int MODE;
	enum { APF2004, APF2005 };

	virtual void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const char *output_dir,
		const char *document_filename);

	virtual void ATEAResultCollector::produceOutput(const wchar_t *output_dir,
		const wchar_t *doc_filename);

	virtual void produceOutput(std::wstring *results);

	void produceOutput(OutputStream &str);

	void toggleIncludeXMLHeaderInfo(bool newValue) {
		_includeXMLHeaderInfo = newValue;
	}

	void toggleUseAbbreviatedIDs(bool newValue) {
		_useAbbreviatedIDs = newValue;
	}

	void setClutterFilter (ATEAInternalClutterFilter *filter) {
		_clutterFilter = filter;
	}


private:

	typedef int CompletedFlag;
	static const CompletedFlag COMPLETED = 1;
	static const CompletedFlag ATTEMPTED = 2;
	static const CompletedFlag FAILED = 3;

	static const Symbol SUB; 
	static const Symbol OBJ; 
	static const Symbol IOBJ; 

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
						  class Entity* ent, 
						  const wchar_t* doc_name);

	// if the mentions contains a title or honorarium, print the title as a "ROLE" mention
	void _printAPFTitleMentions(OutputStream& stream,
								const wchar_t *doc_name,
        				  	    Mention* ment,
						        Entity* ent);

	// info for a name mention, as printed in attributes
	void _printAPFNameAttribute(OutputStream& stream,
								Mention* ment);

	// prints text span and character offset
	void _printAPFMentionExtent(OutputStream& stream,
								const SynNode* node,
								int sentNum);

	// prints text span and character offset
	void _printAPFMentionExtent(OutputStream& stream,
								int startTok,
								int endTok,
								int sentNum);

	// VALUES
	void _printAPFValueHeader(OutputStream& stream, 
							  Value* val, 
							  const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFValueMention(OutputStream& stream, 
							   Value* val, 
							   const wchar_t* doc_name);

	// TIMEX2
	void _printAPFTimexHeader(OutputStream& stream, 
							  Value* val, 
							  const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFTimexMention(OutputStream& stream, 
							   Value* val, 
							   const wchar_t* doc_name);

	// RELATIONS
	void _printAPFRelationHeader(OutputStream& stream, 
							   Relation* rel, 
							   const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFRelMention(OutputStream& stream, 
							   RelMention* ment,
							   Relation* rel, 
							   const wchar_t* doc_name);

	// EVENTS
	void _printAPFEventHeader(OutputStream& stream, 
							   Event* event, 
							   const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFEventMention(OutputStream& stream, 
							   EventMention* em,
							   Event* event, 
							   const wchar_t* doc_name);
	Symbol *_printedEventEntityRole;
	Symbol *_printedEventValueRole;
	Symbol *_printedRelationTimeRole;

	void _printAPFTokenSpanText(OutputStream& stream, 
		int start_tok, int end_tok,
		int sentNum);

	void _printSafeString(OutputStream& stream, const wchar_t* origStr);

	// standardized way of printing IDs
	void _printEntityID(OutputStream& stream, const wchar_t* doc_name, Entity *ent);
	void _printEntityID(OutputStream& stream, const wchar_t* doc_name, int id);
	void _printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, const Mention *ment);
	void _printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, const Mention *ment, Entity *ent);
	void _printEntityMentionID(OutputStream& stream, const wchar_t* doc_name, MentionUID mid, int eid);
	void _printValueID(OutputStream& stream, const wchar_t* doc_name, const Value *val);
	void _printValueMentionID(OutputStream& stream, const wchar_t* doc_name, const Value *val);
	void _printTimexID(OutputStream& stream, const wchar_t* doc_name, const Value *val);
	void _printTimexMentionID(OutputStream& stream, const wchar_t* doc_name, const Value *val);
	void _printRelationID(OutputStream& stream, const wchar_t* doc_name, Relation *rel);
	void _printRelationMentionID(OutputStream& stream, const wchar_t* doc_name, RelMention *rment, Relation *rel);
	void _printEventID(OutputStream& stream, const wchar_t* doc_name, Event *event);
	void _printEventMentionID(OutputStream& stream, const wchar_t* doc_name, EventMention *evment, Event *event);

	// cleanup - print footers
	void _printAPFAttributeFooter(OutputStream& stream);
	void _printAPFEntityFooter(OutputStream& stream);
	void _printAPFValueFooter(OutputStream& stream);
	void _printAPFTimexFooter(OutputStream& stream);
	void _printAPFRelationFooter(OutputStream& stream);
	void _printAPFEventFooter(OutputStream& stream);
	void _printAPFDocumentFooter(OutputStream& stream);

	void _printSentenceBoundaries(OutputStream& stream, const wchar_t *doc_name);
	void _printOriginalText(OutputStream& stream);
	void _printDumpInfo(OutputStream& stream);
	void _printParse(OutputStream& stream, const SynNode *node, const SynNode *parent, const MentionSet *ms);

	/***** utility mention information methods *****/

	// is this the top mention of this entity (i.e. not a child of something structured)
	bool _isTopMention(const Mention* ment, EntityType type);

	// is this mention the child of a list that's not being printed? 
	bool _isItemOfUnprintedList(const Mention* ment);

	// is this mention part of an appositive?
	bool _isPartOfAppositive(const Mention* ment);

	// is the parent of this mention a GPE with a person role?
	bool _isGPEModifierOfPersonGPE(const Mention* ment);

	// is this a name, and is its parent's head not it, and is its
	// parent a different entity? (wordy, I know)
	bool _isNameNotInHeadOfParent(const Mention* ment, EntityType type);

	bool _filterOverlappingMentions;
	bool _sharesHeadWithAlreadyPrintedMention(const Mention *ment);
	bool _sharesHeadWithEarlierMentionOfSameEntity(const Mention *ment, class Entity *ent);

	// the edt head of a mention takes into account appositiveness and nameness
	const SynNode* _getEDTHead(const Mention* ment);

	bool _isPrintableMention(const Mention *ment, class Entity *ent);
	bool _doesOverlap(const SynNode *node1, const SynNode *node2, int sent_num1, int sent_num2);

	// the data we need
	// may as well save the whole doc theory
	DocTheory *_docTheory;
	const EntitySet *_entitySet;
	const ValueSet *_valueSet;
	const RelationSet *_relationSet;
	const EventSet *_eventSet;
	// indexed by sentence
	const TokenSequence **_tokenSequence;

	// an override output stream. This allows us to redirect output that would otherwise go to a file.
	OutputStream *_outputStream;
	bool _includeXMLHeaderInfo;
	bool _useAbbreviatedIDs;

	bool _isPrintedMention(const Mention *ment);
	MentionUID _printedMentions[MAX_MENTIONS_PER_DOCUMENT];
	int _printed_mentions_count;

	void _printValidVerbRelations(
		OutputStream &stream, 
		const Proposition *proposition, 
		const int max_relation_id, 
		const int sentence_num,
		const wchar_t* doc_name);

	void _printVerbRelation(const Mention *arg1, const Mention *arg2, Symbol role1, Symbol role2,
		   				 OutputStream &stream, const wchar_t* doc_name,
						 Symbol verb, int max_relation_id);

	bool _relationBetween(const Mention *arg1, const Mention *arg2);
	bool _eventBetween(const Mention *arg1, const Mention *arg2);

	bool _matchTitle(const Symbol *terminals, int terminals_pos, int terminals_length, 
		     	     TitleListNode *title);
	void _printTitle(OutputStream &stream, const wchar_t *doc_name,  Mention *ment, 
		            Entity *ent, int start_within_mention, int length);
	bool _containsHead(Mention *ment, int start, int length);

	// removing trailing digits from PER names
	int countTrailingDigits(Mention *ment);
	void _printAPFMentionExtentAfterRemoving(OutputStream& stream, const SynNode* node, int sentNum, int characters_to_remove);
	void _printAPFMentionExtentAfterRemoving(OutputStream& stream, int startTok, int endTok, int sentNum, int characters_to_remove);
	void _printAPFTokenSpanTextAfterRemoving(OutputStream& stream, int startTok, int endTok, int sentNum, int characters_to_remove);

	Symbol _inputType;

	bool _is_downcased_doc;

	bool _print_sentence_boundaries;
	bool _print_original_text;

	SymbolHash *_outputPronounWords;
	SymbolHash *_outputNameWords;
	SymbolHash *_outputNominalWords;

	bool _warning_printed;
	int _unknown_relation_count;
	
	bool _create_title_mentions;

	ATEAInternalClutterFilter *_clutterFilter;

	TitleMap *_titles;
	int _n_printed_titles;
	TitleListNode *_printedTitles[MAX_TITLES_PER_ENTITY];

	bool _print_dump_info;

};

#endif

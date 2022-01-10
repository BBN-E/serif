// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef APF_4_GENERIC_RESULT_COLLECTOR_H
#define APF_4_GENERIC_RESULT_COLLECTOR_H

#include <set>
#include "Generic/results/ResultCollector.h"
#include "Generic/common/Offset.h"
#include "Generic/theories/Mention.h"

class DocTheory;
class Entity;
class EntitySet;
class Value;
class ValueSet;
class RelationSet;
class EventSet;
class Relation;
class RelMention;
class SynNode;
class TokenSequence;
class UTF8OutputStream;
class Event;
class EventMention;
class EventEntityRelation;
class SymbolHash;
class OutputStream;
class TemporalNormalizer;
class OutputStream;

class APF4GenericResultCollector : public ResultCollector {
public:
	APF4GenericResultCollector(int mode_);
	virtual ~APF4GenericResultCollector();

	const int MODE;
	enum { APF2004, APF2005, APF2007, APF2008 };

	virtual void loadDocTheory(DocTheory* docTheory);

	virtual void produceOutput(const wchar_t *output_dir, const wchar_t* document_filename);

	virtual void produceOutput(std::wstring *results);

	void toggleIncludeXMLHeaderInfo(bool newValue) {
		_includeXMLHeaderInfo = newValue;
	}

	void toggleUseAbbreviatedIDs(bool newValue) {
		_useAbbreviatedIDs = newValue;
	}

protected:

	virtual void produceOutput(OutputStream &stream);

	/* Produces output for the XDoc model which includes the original text
	   and sentence boundaries */
	virtual void produceXDocOutput(const wchar_t *output_dir, const wchar_t *document_filename);

	void finalize() {
		delete [] _tokenSequence;
		_tokenSequence = 0;
	}

	/***** utility apf methods *****/

	virtual void _printAPFEntitySet(OutputStream& stream, std::set<int>& printed_entities) = 0;
	virtual void _printAPFValueSet(OutputStream& stream);
	virtual void _printAPFRelationSet(OutputStream& stream, std::set<int>& printed_entities);
	virtual void _printAPFEventSet(OutputStream& stream);

	virtual void _printDocumentDateTimeField(OutputStream& stream);

	virtual bool isValidRelationParticipant(int entity_id) = 0;

	// some constants that apf uses to describe mentions
	virtual const wchar_t* _convertMentionTypeToAPF(Mention* ment);

	// heads the document (write to stream) representing doc_source and 
	// called doc_name
	void _printAPFDocumentHeader(OutputStream& stream,
	                             const wchar_t* doc_name=L"DUMMY_NAME",
	                             const wchar_t* doc_source=L"DUMMY_SOURCE");

	// begin info for ent, writing to stream, with generic flag and doc name as indicated
	void _printAPFEntityHeader(OutputStream& stream, 
	                           class Entity* ent, 
	                           const wchar_t* doc_name=L"DUMMY_NAME");

	void _printAPFAttributeHeader(OutputStream& stream);

	// prints a list mention
	virtual void _printAPFListMention(OutputStream& stream,
	                                  Mention* ment,
	                                  class Entity* ent, 
	                                  const wchar_t* doc_name,
	                                  std::set<MentionUID>& printed_mentions_for_this_entity);

	// info for men, a mention of ent, writing to stream
	virtual void _printAPFMention(OutputStream& stream,
	                              Mention* ment,
	                              class Entity* ent, 
	                              const wchar_t* doc_name,
	                              std::set<MentionUID>& printed_mentions_for_this_entity)=0;

	// info for a name mention, as printed in attributes
	void _printAPFNameAttribute(OutputStream& stream, Mention* ment);

	// begin info for mention, writing to stream
	void _printAPFMentionHeader(OutputStream& stream, 
	                            Mention* ment,
	                            Entity* ent, 
	                            const wchar_t* doc_name=L"DUMMY_NAME");

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
	                          Value *val,
	                          const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFValueMention(OutputStream& stream,
	                           Value *val,
	                           const wchar_t* doc_name);

	// TIMEX2
	void _printAPFTimexHeader(OutputStream& stream,
	                          Value *val,
	                          const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFTimexMention(OutputStream& stream,
	                           Value *val,
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

	// print text spans
	void _printAPFSpanText(OutputStream& stream, 
		int start_tok, int end_tok,
		int sentNum);

	void _printAPFTokenSpanText(OutputStream& stream, 
		int start_tok, int end_tok,
		int sentNum);

	void _printAPFSourceSpanText(OutputStream& stream,
		EDTOffset start_edt, EDTOffset end_edt,
		int sentNum);

	// print entity attributes
	virtual void _printEntityType(OutputStream& stream, Entity* ent);
	virtual void _printEntitySubtype(OutputStream& stream, Entity* ent) = 0;
	virtual void _printEntityClass(OutputStream& stream, Entity* ent) = 0;
	virtual void _printEntityGUID(OutputStream& stream, Entity* ent);
	virtual void _printEntityCanonicalName(OutputStream& stream, Entity* ent);

	// print mention attributes
	virtual void _printEntityMentionType(OutputStream& stream, Mention* ment);
	virtual void _printEntityMentionRole(OutputStream& stream, Mention* ment) = 0;
	virtual void _printEntityMentionMetonymy(OutputStream& stream, Mention* ment);

	void _printSafeString(OutputStream& stream, const wchar_t* origStr);

	// standardized way of printing IDs
	void _printEntityID(OutputStream& stream, const wchar_t* doc_name, Entity *ent);
	void _printEntityID(OutputStream& stream, const wchar_t* doc_name, int id);
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
	std::set<MentionUID> _printedMentions;

	Symbol _inputType;

	bool _is_downcased_doc;

	bool _print_sentence_boundaries;
	bool _print_original_text;
	bool _print_source_spans;

	bool _in_post_xdoc_print_mode;
	bool _dont_print_relation_timex_args;
	bool _dont_print_events;
	bool _dont_print_values;
	bool _use_post_xdoc;
	bool _include_confidence_scores;

	SymbolHash *_outputPronounWords;
	SymbolHash *_outputNameWords;
	SymbolHash *_outputNominalWords;

	TemporalNormalizer *_temporalNormalizer;
};

#endif

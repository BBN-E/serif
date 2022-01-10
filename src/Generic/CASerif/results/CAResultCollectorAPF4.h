// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_RESULT_COLLECTOR_APF_4_H
#define CA_RESULT_COLLECTOR_APF_4_H

#include "Generic/common/OutputStream.h"
#include "Generic/apf/APF4GenericResultCollector.h"

class DocTheory;
class Mention;
class Entity;
class CorrectDocument;
class CorrectMention;

class CAResultCollectorAPF4 : public APF4GenericResultCollector {
public:
	
	CAResultCollectorAPF4(int mode_);

	void loadDocTheory(DocTheory *docTheory);

private:

	/***** utility apf methods *****/

	void _printAPFEntitySet(OutputStream& stream, std::set<int>& printed_entities);

	// some constants that apf uses to describe mentions
	const wchar_t* _convertMentionTypeToAPF(Mention* ment);

	// info for men, a mention of ent, writing to stream
	void _printAPFMention(OutputStream& stream,
						  Mention* ment,
						  class Entity* ent, 
						  const wchar_t* doc_name,
						  std::set<Mention::UID>& printed_mentions_for_this_entity);

	void _printCorrectMentionExtent(OutputStream& stream,
									CorrectMention* cm,
									int sentNum);

	void _printCorrectMentionHeadExtent(OutputStream& stream,
									CorrectMention* cm,
									int sentNum);

	void _printCorrectMentionText(OutputStream& stream, 
		CorrectMention *cm,
		int sentNum);

	void _printCorrectMentionHeadText(OutputStream& stream, 
		CorrectMention *cm,
		int sentNum) ;

	bool USE_CORRECT_COREF;
	bool USE_CORRECT_SUBTYPES;

	CorrectDocument *_correctDocument;

	void _printEntitySubtype(OutputStream& stream, Entity* ent);
	void _printEntityClass(OutputStream& stream, Entity* ent);

	void _printEntityMentionRole(OutputStream& stream, Mention* ment);

	bool isValidRelationParticipant(int entity_id);
};

#endif

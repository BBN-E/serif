// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef APF_4_RESULT_COLLECTOR_H
#define APF_4_RESULT_COLLECTOR_H

#include "Generic/common/OutputStream.h"
#include "Generic/apf/APF4GenericResultCollector.h"

class DocTheory;
class Mention;
class Entity;

class APF4ResultCollector :  public APF4GenericResultCollector  {
public:

	APF4ResultCollector(int mode_);

private:

	bool _print_pronoun_level_entities;
	bool _print_1p_pronoun_level_entities;
	bool _print_partitives_in_relations;
	bool _print_partitives_in_events;
	bool _print_all_partitives;

	/***** utility apf methods *****/

	void _printAPFEntitySet(OutputStream& stream, std::set<int>& printed_entities);

	// info for men, a mention of ent, writing to stream
	void _printAPFMention(OutputStream& stream,
	                      Mention* ment,
	                      class Entity* ent, 
	                      const wchar_t* doc_name,
	                      std::set<MentionUID>& printed_mentions_for_this_entity);

	void _printEntitySubtype(OutputStream& stream, Entity* ent);
	void _printEntityClass(OutputStream& stream, Entity* ent);

	void _printEntityMentionRole(OutputStream& stream, Mention* ment);

	bool isValidRelationParticipant(int entity_id);
};

#endif

// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PG_FACT_H
#define PG_FACT_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "Generic/theories/MentionConfidence.h" 
#include "ProfileGenerator/PGFactArgument.h"
#include "ProfileGenerator/ProfileSlot.h"

#include <vector>
#include <string>
#include <math.h>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(PGFact);
BSP_DECLARE(PGFactDate);

class PGFact
{
public:	
	
	typedef enum { NOT_APPLICABLE, TEXT, STT, OCR } source_type_t;
	typedef enum { VALID, USER_DELETED, TRIMMED, UNTRUSTED, USER_ADDED } status_type_t;

	// Annoyingly, there are too many of these to pass in as values using make_shared,
	//  so we do it this way instead! If you add a field here, make SURE it is initialized
	//  everywhere you create one of these. I suggest searching the codebase for 'pg_fact_info_t'.
	typedef struct {
		int fact_id; // primary key
		int fact_type_id;
		int document_id;
		int passage_id;
		int mention_id;
		int entity_id;
		int source_id;
		bool is_serif; // a hack for knowing if this is one of "our" facts
		LanguageAttribute language;
		boost::gregorian::date document_date; // if not initialized will be null
		double score;
		int score_group;
		ProfileSlot::db_fact_type_t db_fact_type;
		source_type_t source_type;
		std::wstring support_chunk_value;
		status_type_t status;
		int epoch_id; // for KBFacts only
	} pg_fact_info_t;


	friend PGFact_ptr boost::make_shared<PGFact>(pg_fact_info_t const&);
	friend PGFact_ptr boost::make_shared<PGFact>(PGFact_ptr const&, ProfileSlot::DatabaseFactInfo const&);
	friend PGFact_ptr boost::make_shared<PGFact>(void);

	// For sorting by date (breaking ties by factid)
	static bool compareByDate(const PGFact_ptr& a, const PGFact_ptr& b) { 
		if (a->getDocumentDate() == b->getDocumentDate())
			return (a->getFactId() < b->getFactId());
		else return a->getDocumentDate() < b->getDocumentDate();
	}

	// Setters
	void addArgument(PGFactArgument_ptr arg, bool is_answer = false);
	void addDate(PGFactDate_ptr date);
	
	// Getters
	PGFactArgument_ptr getFirstArgumentForRole(std::string role);
	std::vector<PGFactArgument_ptr> getArgumentsForRole(std::string role);
	std::vector<PGFactArgument_ptr>& getOtherArguments() { return _otherArguments; }
	std::vector<PGFactArgument_ptr>& getAllArguments() { return _allArguments; }
	bool matchesDFI(ProfileSlot::DatabaseFactInfo dfi);
	PGFactArgument_ptr getAgentArgument() { return _agentArgument; }
	PGFactArgument_ptr getAnswerArgument() { return _answerArgument; }
	int getFactId() { return _fact_info.fact_id; } // CAUTION: this can be either a Fact id or a KBFact id
	int getFactType() { return _fact_info.fact_type_id; }
	int getMentionId() { return _fact_info.mention_id; }
	int getEntityId() { return _fact_info.entity_id; }
	int getPassageId() { return _fact_info.passage_id; }
	int getDocumentId() { return _fact_info.document_id; }
	int getSourceId() { return _fact_info.source_id; }
	bool isSerif() { return _fact_info.is_serif; }
	int getEpochId() { return _fact_info.epoch_id; }
	double getScore() { return _fact_info.score; }
	int getScoreGroup() { return _fact_info.score_group; }
	ProfileSlot::db_fact_type_t getDBFactType() { return _fact_info.db_fact_type; }
	source_type_t getSourceType() { return _fact_info.source_type; }
	pg_fact_info_t getFactInfo() { return _fact_info; }
	boost::gregorian::date getDocumentDate() { return _fact_info.document_date; }
	bool isMT() { return _fact_info.language != Language::UNSPECIFIED && _fact_info.language != Language::ENGLISH; }
	LanguageAttribute getLanguage() { return _fact_info.language; }
	std::wstring getSupportChunkValue() { return _fact_info.support_chunk_value; }
	std::vector<PGFactDate_ptr> getFactDates();
	bool hasNoDates();
	status_type_t getStatus() { return _fact_info.status; }
	
	// Confidence measures
	bool hasReliableAgentMentionConfidence();
	bool hasReliableAnswerMentionConfidence();
	
	// Exact equivalency (nothing fancy)
	bool isEquivalent(PGFact_ptr other);	
	
private:
	PGFact(pg_fact_info_t info) : _fact_info(info)
	{		
		// HACK for SERIF-style confidences
		if (_fact_info.score > 1) {
			_fact_info.score_group = int(floor(_fact_info.score));
			_fact_info.score = _fact_info.score - _fact_info.score_group;
		}
	}
	
	PGFact(PGFact_ptr origFact, ProfileSlot::DatabaseFactInfo dfi);

	PGFact() {
		_fact_info.fact_id = -1;
		_fact_info.fact_type_id = -1;
		_fact_info.document_id = -1;
		_fact_info.passage_id = -1;
		_fact_info.mention_id = -1;
		_fact_info.entity_id = -1;
		_fact_info.source_id = -1;
		_fact_info.is_serif = false;
		_fact_info.language = Language::UNSPECIFIED;
		_fact_info.score = 0;
		_fact_info.score_group = -1;
		_fact_info.support_chunk_value = L"";
		_fact_info.db_fact_type = ProfileSlot::FAKE;
		_fact_info.source_type = PGFact::NOT_APPLICABLE;
		_fact_info.status = PGFact::VALID;			
		_fact_info.epoch_id = -1;
	}

	pg_fact_info_t _fact_info;

	std::vector<PGFactDate_ptr> _factDates;
	std::vector<PGFactArgument_ptr> _otherArguments;
	std::vector<PGFactArgument_ptr> _allArguments;
	PGFactArgument_ptr _agentArgument;
	PGFactArgument_ptr _answerArgument;

};

#endif

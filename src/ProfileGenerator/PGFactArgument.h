// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

// PGFactArgument.h
// This file describes the PGFact Argument, which is used to store data about PGFact arguments.

#ifndef PG_FACT_ARGUMENT_H
#define PG_FACT_ARGUMENT_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "Generic/theories/MentionConfidence.h" 
#include "boost/date_time/gregorian/gregorian.hpp"
#include <map>
#include <vector>
#include <string>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(PGFactArgument);

class PGFactArgument
{
public:
	friend PGFactArgument_ptr boost::make_shared<PGFactArgument>(int const&, int const&, std::string const&, 
		std::wstring const&, std::wstring const&, std::string const&, double const&, bool const&);

	// Getters
	int getPrimaryId() { return _primary_id; }
	int getActorId() { return _actor_id; }
	std::string getEntityType() { return _entity_type; }
	bool isInFocus() { return _is_focus; }
	std::string getRole() { return _role; }
	void setRole(std::string role) { _role = role; }
	MentionConfidenceAttribute getSERIFMentionConfidence() { return _serif_mention_confidence; }
	void setSERIFMentionConfidence(MentionConfidenceAttribute conf) { _serif_mention_confidence = conf; }
	std::wstring getResolvedStringValue() { return _resolved_string_value; }
	std::wstring getLiteralStringValue() { return _literal_string_value; }
	double getConfidence() { return _confidence; }

	// Reliability
	bool isReliable();

	// Exact equivalency (nothing fancy)
	bool isEquivalent(PGFactArgument_ptr other);
	
private:
	PGFactArgument(int primary_id, int actor_id, std::string entity_type, std::wstring literal_string_value, std::wstring resolved_string_value, 
		std::string role, double confidence, bool is_focus) 
		: _primary_id(primary_id), _actor_id(actor_id), _entity_type(entity_type), _literal_string_value(literal_string_value), _resolved_string_value(resolved_string_value), 
		_role(role), _confidence(confidence), _is_focus(is_focus), _serif_mention_confidence(MentionConfidenceStatus::UNKNOWN_CONFIDENCE)
	{
		// NOTE: For KBFactArguments, entity_type will be "" and confidence will be 0.0 and serif_mention_confidence will never be set	
	}

	int _primary_id;	
	std::string _role;
	double _confidence;
	MentionConfidenceAttribute _serif_mention_confidence;
	bool _is_focus;

	// Optional-- can be null (-1 for integers, or "" for strings)
	int _actor_id; // for mention arguments
	std::string _entity_type; // for mention arguments
	std::wstring _resolved_string_value; // for time or string arguments
	std::wstring _literal_string_value; // for time or string arguments

};

#endif

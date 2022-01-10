// Copyright (c) 2012 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PG_ACTOR_INFO_H
#define PG_ACTOR_INFO_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <string>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(PGActorInfo);

class PGActorInfo
{
public:
	friend PGActorInfo_ptr boost::make_shared<PGActorInfo>(int const&, std::wstring const&, std::string const&, double const&);

	int getActorId() { return _actor_id; }
	std::wstring getCanonicalActorName() { return _canonical_actor_name; }
	std::string getEntityType() { return _entity_type; }
	double getConfidence() { return _confidence; }

	std::wstring toPrintableString() { 
		std::wstringstream str;
		str << getCanonicalActorName() << " (id: " << getActorId() << ")";
		return str.str();
	}

		
private:
	PGActorInfo(int actor_id, std::wstring canonical_actor_name, std::string entity_type, double confidence) 
		: _actor_id(actor_id), _canonical_actor_name(canonical_actor_name), _entity_type(entity_type), _confidence(confidence)
	{}

	int _actor_id;
	std::wstring _canonical_actor_name;
	std::string _entity_type;
	double _confidence;

};

#endif

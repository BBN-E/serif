// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_EDIT_DISTANCE_H
#define ACTOR_EDIT_DISTANCE_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/xdoc/EditDistance.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorInfo.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <set>

class Mention;
class SentenceTheory;
class DocTheory;
class EntityType;

class ActorEditDistance {
public:
	typedef std::map<ActorId, double> ActorEditDistanceMap;

	ActorEditDistance(ActorInfo_ptr actorInfo);
	~ActorEditDistance();

	ActorEditDistanceMap findCloseActors(const Mention *mention, double threshold, const SentenceTheory *st, const DocTheory *dt);

private:
	EditDistance _editDistance;
	UTF8OutputStream _debugStream;

	std::vector<ActorPattern *> _gpeActors;
	std::vector<ActorPattern *> _perActors;
	std::vector<ActorPattern *> _orgActors;
	std::vector<ActorPattern *> _locActors;
	std::vector<ActorPattern *> _facActors;

	std::map<std::wstring, ActorEditDistanceMap> _cache;
	static const size_t AED_MAX_ENTRIES = 1000;

	std::map<std::wstring, std::set<std::wstring> > _nicknames;
	
	void expandNameToEquivalentSet(std::wstring name, EntityType entityType, std::set<std::wstring> & eqNames);
};

typedef boost::shared_ptr<ActorEditDistance> ActorEditDistance_ptr;

#endif

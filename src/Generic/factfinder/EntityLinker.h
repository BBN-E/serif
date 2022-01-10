// Copyright (c) 2014 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef ENTITY_LINKER_H
#define ENTITY_LINKER_H

#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include "Generic/common/Symbol.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/DocTheory.h"


class DocTheory;

// This is a wrapper class for using any Entity Linker that we may have
// Designed to manage Entity linking resources
class EntityLinker {
public:
	EntityLinker(bool _use_actor_id);
	~EntityLinker();

	// currently only general unique ID at each time it is called
	Symbol getEntityID(int id);

	Symbol getEntityID(Symbol entity_type, std::wstring canonicalNameStr, const Entity *e, const DocTheory* docTheory, double min_actor_match_conf);

	Symbol getMentionActorID(Symbol type, const Mention *m, const DocTheory* docTheory, double minActorPatternConf, double minEditDistance, double minPatternMatchScorePlusAssociationScore, std::map<int, double>& actorId2actorEntityConf);
private:
	int gid;
	bool _use_actor_id_for_entity_linker;
};

#endif


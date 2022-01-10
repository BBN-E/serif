// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_TOKEN_SUBSET_TREES_H
#define ACTOR_TOKEN_SUBSET_TREES_H

#include "Generic/xdoc/TokenSubsetTrees.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorEntityScorer.h"
#include "Generic/actors/ActorInfo.h"

#include <boost/shared_ptr.hpp>

class Mention;

class ActorTokenSubsetTrees {
public:
	typedef std::map<ActorId, double> ActorScoreMap;

	ActorTokenSubsetTrees(ActorInfo_ptr actorInfo, ActorEntityScorer_ptr aes);
	~ActorTokenSubsetTrees();

	ActorScoreMap getTSTEqNames(const Mention *mention);

private:

	// Mention name -> Token Subset Tree results cache
	std::map<std::wstring, ActorScoreMap> _mentionNameCache;
	static const size_t ATST_MAX_ENTRIES = 1000;

	ActorEntityScorer_ptr _actorEntityScorer;

	// Mention word -> possible actor pattern matches cache
	std::map<Symbol, std::vector<ActorPattern *> > _actorNameCache;

	bool closePersonTSTMatch(std::wstring name1, std::wstring name2);

	size_t _too_frequent_token_threshold;

};

typedef boost::shared_ptr<ActorTokenSubsetTrees> ActorTokenSubsetTrees_ptr;

#endif

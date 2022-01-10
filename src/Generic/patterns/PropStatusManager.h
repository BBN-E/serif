// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PROP_STATUS_MANAGER_H
#define PROP_STATUS_MANAGER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/Attribute.h"
#include "Generic/patterns/PatternMatcher.h"

class PropStatusManager{
	
public:
	BOOST_MAKE_SHARED_0ARG_CONSTRUCTOR(PropStatusManager);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PropStatusManager, bool);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PropStatusManager, Sexp*);
	
private:
	PropStatusManager(bool require_negative); // for backwards compatibility
	PropStatusManager(Sexp *sexp);
	PropStatusManager() {}

public:
	static bool isPropStatusManagerSym(Symbol constraintType);
	bool propositionIsValid(PatternMatcher_ptr patternMatcher, const Proposition* prop);
	
private:

	void throwError(Sexp *sexp);
	
	bool explicitlyAllows(PropositionStatusAttribute att);
	bool blocks(PropositionStatusAttribute att);

	std::set<PropositionStatusAttribute> _blocked;
	std::set<PropositionStatusAttribute> _required;
	std::set<PropositionStatusAttribute> _allowed;

	bool addAllowedStatus(PropositionStatusAttribute att);
	bool addRequiredStatus(PropositionStatusAttribute att);
	bool addBlockedStatus(PropositionStatusAttribute att);
	
};

typedef boost::shared_ptr<PropStatusManager> PropStatusManager_ptr;

#endif

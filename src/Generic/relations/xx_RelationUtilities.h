#ifndef XX_RELATION_UTILITIES_H
#define XX_RELATION_UTILITIES_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/PropositionSet.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/relations/RelationUtilities.h"

class DefaultRelationUtilities : public RelationUtilities {
public:

	std::vector<bool> identifyFalseOrHypotheticalProps(const PropositionSet *propSet, const MentionSet *mentionSet) {
		SessionLogger::warn("unimplemented_class") << L"DefaultRelationUtilities::identifyFalseOrHypotheticalProps should not be called for a default language\n";
		std::vector<bool> dummy(propSet->getNPropositions(), false);
		return dummy;
	}
	
	bool coercibleToType(const Mention *mention, Symbol type) { return false; }

	Symbol stemPredicate(Symbol word, Proposition::PredType predType) {
		return word;
	}


};

// RelationModel factory
class DefaultRelationUtilitiesFactory: public RelationUtilities::Factory {

	DefaultRelationUtilities* utils;
	virtual RelationUtilities *get() { 
		if (utils == 0) {
			utils = _new DefaultRelationUtilities(); 
		} 
		return utils;
	}

public:
	DefaultRelationUtilitiesFactory() { utils = 0;}

};


#endif

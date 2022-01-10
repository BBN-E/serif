// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MODIFYEE_NAME_TYPE_FT_H
#define MODIFYEE_NAME_TYPE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"

#include <string>
#include <iostream>

using namespace std;


class ModifyeeNameTypeFT: public P1DescFeatureType {
public:
	ModifyeeNameTypeFT() : P1DescFeatureType(Symbol(L"modifyee-name-type")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		if (o->getNode()->getParent() == 0)
			return 0;

		const SynNode *modifyee = o->getNode()->getParent()->getHead();

		// if the parent's head is the same node, then we're not a premod
		if (modifyee == o->getNode())
			return 0;

		Mention *mention = o->getMentionSet()->getMentionByNode(modifyee);
		if (mention == 0 || mention->getMentionType() != Mention::NAME)
			return 0;

		resultArray[0] = _new DTBigramFeature(this, state.getTag(),
			mention->getEntityType().getName());
		return 1;
	}

};

#endif

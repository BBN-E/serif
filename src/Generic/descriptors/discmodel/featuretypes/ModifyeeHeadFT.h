// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MODIFYEE_HEAD_FT_H
#define MODIFYEE_HEAD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"

#include <string>
#include <iostream>

using namespace std;


class ModifyeeHeadFT: public P1DescFeatureType {
public:
	ModifyeeHeadFT() : P1DescFeatureType(Symbol(L"modifyee-head")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		if (o->getNode()->getParent() == 0)
			return 0;

		const SynNode *modifyee = o->getNode()->getParent()->getHead();

		// if the parent's head is the same node, then we're not a premod,
		// so generate a warning
		if (modifyee == o->getNode()) {
			//string nodeDump = o->getNode()->getParent()->toDebugString(2);
			return 0;
		}

		resultArray[0] = _new DTBigramFeature(this, state.getTag(),
			modifyee->getHeadWord());
		return 1;
	}

};

#endif

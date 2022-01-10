// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FUNC_PARENT_HEAD_FT_H
#define FUNC_PARENT_HEAD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class FuncParentHeadFT : public P1DescFeatureType {
public:
	FuncParentHeadFT() : P1DescFeatureType(Symbol(L"func-parent-head")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		const SynNode *parent = o->getNode();
		while (parent != 0 && 
			parent->getParent() != 0 &&
			parent->getParent()->getHead() == parent)
		{
			parent = parent->getParent();
		}

		if (parent != 0 && parent->getParent() != 0 )
			parent = parent->getParent();
		else return 0;
		Symbol hw =  o->getNode()->getHeadWord();

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), parent->getHeadWord(), hw);
		return 1;
	}

};

#endif

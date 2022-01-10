// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NAMEMODTYPE_HEAD_FT_H
#define NAMEMODTYPE_HEAD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class NameModTypeHeadFT : public P1DescFeatureType {
public:
	NameModTypeHeadFT() : P1DescFeatureType(Symbol(L"namemod-type-head")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		const SynNode *node = o->getNode();
		int modindex = node->getHeadIndex() - 1;
		if((modindex > 0) && node->getChild(modindex)->hasMention()){
			const Mention* ment = o->getMentionSet()->getMentionByNode(node->getChild(modindex));
			if(ment->getMentionType() == Mention::NAME){
				node->getChild(modindex)->getMentionIndex();
				resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
					ment->getEntityType().getName(), node->getHeadWord());
				return 1;
			}
		}
		return 0;

	}

};

#endif

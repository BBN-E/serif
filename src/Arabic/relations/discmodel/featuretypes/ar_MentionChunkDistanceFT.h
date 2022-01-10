// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_MENTION_CHUNK_DISTANCE_FT_H
#define AR_MENTION_CHUNK_DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"

class ArabicMentionChunkDistanceFT : public P1RelationFeatureType {
public:
	ArabicMentionChunkDistanceFT() : P1RelationFeatureType(Symbol(L"mention-chunk-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1000);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{

		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		const SynNode *chunk1 = RelationUtilities::get()->findNPChunk(o->getMention1()->getNode());
		const SynNode *chunk2 = RelationUtilities::get()->findNPChunk(o->getMention2()->getNode());

		if (chunk1 == 0 || chunk2 == 0)
			throw InternalInconsistencyException("ArabicMentionChunkDistanceFT::extractFeatures()", 
												 "Null mention chunk found.");

		if (chunk1 == chunk2) 
			resultArray[0] = _new DTIntFeature(this, state.getTag(), 0);	
		else if (chunk1->getEndToken() < chunk2->getStartToken()) {
			int tok1 = chunk1->getEndToken();
			int tok2 = chunk2->getStartToken();
			resultArray[0] = _new DTIntFeature(this, state.getTag(), tok2 - tok1 - 1);	
		}
		else {
			int tok1 = chunk2->getEndToken();
			int tok2 = chunk1->getStartToken();
			resultArray[0] = _new DTIntFeature(this, state.getTag(), tok2 - tok1 - 1);	
		}

		return 1;

	}

};

#endif

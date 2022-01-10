// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_PREP_CHAIN_FT_H
#define AR_PREP_CHAIN_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"

class ArabicPrepChainFT : public P1RelationFeatureType {
public:
	ArabicPrepChainFT() : P1RelationFeatureType(Symbol(L"prep-chain")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));


		Symbol enttype1 = o->getMention1()->getEntityType().getName();
		Symbol enttype2 = o->getMention2()->getEntityType().getName();

		const SynNode *chunk1 = RelationUtilities::get()->findNPChunk(o->getMention1()->getNode());
		const SynNode *chunk2 = RelationUtilities::get()->findNPChunk(o->getMention2()->getNode());

		if (chunk1 == chunk2)
			return 0;
		if (chunk1 == 0 || chunk2 == 0)
			throw InternalInconsistencyException("ArabicPrepChainFT::extractFeatures()",
												 "Could not find NP chunk for one of mentions.");

		std::wstring prep_str = L"";
		const SynNode *parent = chunk1->getParent();
		if (parent == 0 )
			throw InternalInconsistencyException("ArabicPrepChainFT::extractFeatures()",
												 "Chunk1 has no parent.");
		int i;
		for (i = 0; i < parent->getNChildren(); i++) {
			if (parent->getChild(i) == chunk1 ||
				parent->getChild(i) == chunk2)
			{
				i++;
				break;
			}
		}
		for (int j = i; j < parent->getNChildren(); j++) {
			if (parent->getChild(j)->getTag() == ArabicSTags::IN) {			
				if (wcscmp(prep_str.c_str(), L""))
					prep_str += L"_";
				prep_str += parent->getChild(j)->getHeadWord().to_string();
			}
			if (parent->getChild(j) == chunk1 || parent->getChild(j) == chunk2)
				break;
		}

		if (wcscmp(prep_str.c_str(), L"")) {
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
				enttype1, enttype2, Symbol(prep_str.c_str()));
			if(RelationUtilities::get()->distIsLessThanCutoff(o->getMention1(), o->getMention2())){
				wchar_t buffer[300];
				wcscpy(buffer, enttype1.to_string());
				wcscat(buffer, L"-CLOSE");
				Symbol s1 =Symbol(buffer);
				wcscpy(buffer, enttype2.to_string());
				wcscat(buffer, L"-CLOSE");
				Symbol s2 =Symbol(buffer);
				resultArray[1] = _new DTQuadgramFeature(this, state.getTag(), s1, s2, Symbol(prep_str.c_str()));
				return 2;
			}
			else{
				wchar_t buffer[300];
				wcscpy(buffer, enttype1.to_string());
				wcscat(buffer, L"-FAR");
				Symbol s1 =Symbol(buffer);
				wcscpy(buffer, enttype2.to_string());
				wcscat(buffer, L"-FAR");
				Symbol s2 =Symbol(buffer);
				resultArray[1] = _new DTQuadgramFeature(this, state.getTag(), s1, s2, Symbol(prep_str.c_str()));
				return 2;
			}

			return 1;
		}

		return 0;

	}

};

#endif

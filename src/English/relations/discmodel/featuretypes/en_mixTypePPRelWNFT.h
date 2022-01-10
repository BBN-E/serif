// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_MIXTYPE_PP_REL_WN_FT_H
#define EN_MIXTYPE_PP_REL_WN_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuadgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishMixTypePPRelWNFT : public P1RelationFeatureType {
public:
	EnglishMixTypePPRelWNFT() : P1RelationFeatureType(Symbol(L"mixtype-pp-relation-wordnet")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->hasPPRel()) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();
			int nfeatures = 0;

			int level = o->getNOffsetsOfMent1() - startLevel();
				
			while (level >= 0) {
				if (nfeatures >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
					SessionLogger::warn("DT_feature_limit") <<"en_mixTypePPRelWNFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
					break;
				}
				resultArray[nfeatures++] = _new DTQuadgramIntFeature(this, state.getTag(),
				enttype1, enttype2, o->getPrepinPPRel(), o->getNthOffsetOfMent1(level));
				level -= interval();
			}
			return nfeatures;
		}else {
			return 0;
		}
	}

	static int startLevel() {
		static bool init = false;
		static int _start_offset;
		if (!init) {
			init = true;
			_start_offset = ParamReader::getOptionalIntParamWithDefaultValue("wordnet_level_start", 1);
			if (_start_offset <= 0) {
				throw UnexpectedInputException("EnglishMixTypePPRelWNFT::startLevel()",
					"Parameter 'wordnet_level_start' must be greater than 0");
			}
		}
		return _start_offset;
	}

	static int interval() {
		static bool init = false;
		static int _interval;
		if (!init) {
			_interval = ParamReader::getOptionalIntParamWithDefaultValue("wordnet_level_interval", 1);
			init = true;
			if (_interval <= 0) {
				throw UnexpectedInputException("EnglishMixTypePPRelWNFT::interval()",
					"Parameter 'wordnet_level_interval' must be greater than 0");
			}
		}
		return _interval;
	}

};

#endif

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SIMPLE_PROP_WN_NO_TYPES_FT_H
#define EN_SIMPLE_PROP_WN_NO_TYPES_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishSimplePropWNNoTypesFT : public P1RelationFeatureType {
public:
	EnglishSimplePropWNNoTypesFT() : P1RelationFeatureType(Symbol(L"simple-prop-wordnet-no-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		// later, put in a reversal for in-<sub>, etc.

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {
			Symbol role1 = link->getArg1Role();
			Symbol role2 = link->getArg2Role();

			int nfeatures = 0;
			int level = link->getNOffsets() - startLevel();
			while (level >= 0) {
				if (nfeatures >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION-){
						SessionLogger::warn("DT_feature_limit") <<"en_SimplePropWNNoTypesFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
						break;
				}
				resultArray[nfeatures++] = _new DTTrigramIntFeature(this, state.getTag(),
					 role1, role2, link->getNthOffset(level));
				level -= interval();
			}

			return nfeatures;
		} else return 0;
	}


	static int EnglishSimplePropWNNoTypesFT::startLevel() {
		static bool init = false;
		static int _start_offset;
		if (!init) {
			init = true;
			_start_offset = ParamReader::getOptionalIntParamWithDefaultValue("wordnet_level_start", 1);
			if (_start_offset <= 0) {
				throw UnexpectedInputException("EnglishSimplePropWNNoTypesFT::startLevel()",
					"Parameter 'wordnet_level_start' must be greater than 0");
			}
		}
		return _start_offset;
	}

	static int EnglishSimplePropWNNoTypesFT::interval() {
		static bool init = false;
		static int _interval;
		if (!init) {
			_interval = ParamReader::getOptionalIntParamWithDefaultValue("wordnet_level_interval", 1);
			init = true;
			if (_interval <= 0) {
				throw UnexpectedInputException("EnglishSimplePropWNNoTypesFT::interval()",
					"Parameter 'wordnet_level_interval' must be greater than 0");
			}
		}
		return _interval;
	}


};

#endif

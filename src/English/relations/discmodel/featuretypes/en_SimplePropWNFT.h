// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SIMPLE_PROP_WN_FT_H
#define EN_SIMPLE_PROP_WN_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishSimplePropWNFT : public P1RelationFeatureType {
public:
	EnglishSimplePropWNFT() : P1RelationFeatureType(Symbol(L"simple-prop-wordnet")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
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
			//Symbol enttype1 = o->getMention1()->getEntityType().getName();
			//Symbol enttype2 = o->getMention2()->getEntityType().getName();

			//When one of the argument role is <sub>, it will be switched to the first argument.
			//Entity types should be switched accordingly.
			Symbol enttype1 = link->getArg1Ment(o->getMentionSet())->getEntityType().getName();
			Symbol enttype2 = link->getArg2Ment(o->getMentionSet())->getEntityType().getName();

			int nfeatures = 0;
			int level = link->getNOffsets() - startLevel();
			while (level >= 0) {
				resultArray[nfeatures++] = _new DTQuintgramIntFeature(this, state.getTag(),
					enttype1, enttype2, role1, role2, link->getNthOffset(level));
				level -= interval();
			}

			return nfeatures;
		} else return 0;
	}


	static int startLevel() {
		static bool init = false;
		static int _start_offset;
		if (!init) {
			init = true;
			_start_offset = ParamReader::getOptionalIntParamWithDefaultValue("wordnet_level_start", 1);
			if (_start_offset <= 0) {
				throw UnexpectedInputException("EnglishSimplePropWNFT::startLevel()",
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
				throw UnexpectedInputException("EnglishSimplePropWNFT::interval()",
					"Parameter 'wordnet_level_interval' must be greater than 0");
			}
		}
		return _interval;
	}


};

#endif

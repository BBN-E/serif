// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_HEADWORD_WN_FT_H
#define EN_HEADWORD_WN_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DT2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/wordnet/xx_WordNet.h"


class EnglishHeadwordWNFT : public P1DescFeatureType {
public:
	EnglishHeadwordWNFT() : P1DescFeatureType(Symbol(L"headword-wordnet")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT2IntFeature(this, SymbolConstants::nullSymbol, 0, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		int level = o->getNOffsets() - startLevel();
		int nfeatures = 0;
		while (level >= 0) {
			if (nfeatures>= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
					SessionLogger::warn("DT_feature_limit") <<"EnglishHeadwordWNFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
					break;
			}else{
				resultArray[nfeatures++] = _new DT2IntFeature(this, state.getTag(),
					o->getNthOffset(level), o->getNOffsets() - level);
				level -= interval();
			}
		}

		return nfeatures;
	}

	static int startLevel() {
		static bool init = false;
		static int _start_offset;
		if (!init) {
			init = true;
			_start_offset = ParamReader::getOptionalIntParamWithDefaultValue("wordnet_level_start", 1);
			if (_start_offset <= 0) {
				throw UnexpectedInputException("EnglishHeadwordWNFT::startLevel()",
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
				throw UnexpectedInputException("EnglishHeadwordWNFT::startLevel()",
					"Parameter 'wordnet_level_interval' must be greater than 0");
			}
		}
		return _interval;
	}

};

#endif

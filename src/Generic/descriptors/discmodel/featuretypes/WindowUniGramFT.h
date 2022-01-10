// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WINDOW_UNIGRAM_FT_H
#define WINDOW_UNIGRAM_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolHash.h"
#include <boost/scoped_ptr.hpp>


class WindowUniGramFT : public P1DescFeatureType {
private:
	static bool _initialized; 
	static const int _window_size = 10;
	static SymbolHash* _stopWords;

	static void initializeLists(){
		std::string result = ParamReader::getRequiredParam("pdesc_window_gram_stopwords");
		_stopWords = _new SymbolHash(5000);
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		uis.open(result.c_str());
		UTF8Token word;
		while(!uis.eof()){
			uis >> word;
			_stopWords->add(word.symValue());
		}
		_initialized = true;
	}
public:
	
	WindowUniGramFT() : P1DescFeatureType(Symbol(L"window-unigram")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		if(!_initialized){
			initializeLists();
		}

		int start = o->getNode()->getHeadPreterm()->getStartToken();
		int first = start - _window_size;
		int last = start + _window_size;
		if(first < 0){
			first = 0;
		}
		if(last > o->getNSentWords()){
			last = o->getNSentWords();
		}
		int  nresults = 0;
		for(int i = first; i< last; i++){
			if(i == start){ //skip the word itself
				continue;
			}
			if(i == start +1){ //skip the bigram, there's a separate feature for that word itself
				continue;
			}
			if(i == start -1){ //skip the bigram, there's a separate feature for that word itself
				continue;
			}
			Symbol oth = o->getSentWord(i);
			if(!_stopWords->lookup(oth)){
				if( nresults >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
					SessionLogger::warn("DT_feature_limit") 
						<<"WindowUniGramFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
					break;
				}else{
					resultArray[nresults++] = _new DTBigramFeature(this, 
						state.getTag(), oth);
				}
			}
		}
		return nresults;
	}


};


#endif

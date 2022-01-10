// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/PDecoder.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/names/discmodel/PIdFFeatureTypes.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/names/discmodel/PIdFSecondaryDecoders.h"
#include <boost/scoped_ptr.hpp>

PIdFSecondaryDecoders::PIdFSecondaryDecoders(){
	std::string buffer = ParamReader::getParam("pidf_secondary_decoders");
	if (!buffer.empty()) {
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		uis.open(buffer.c_str());
		if(uis.fail()){
			throw(UnexpectedInputException("PIdFSecondaryDecoders::PIdFSecondaryDecoders()",
				"couldnt open input file"));
		}
		UTF8Token tok;

		uis >> _n_decoders;
		if(_n_decoders >= MAX_DECODERS){
			_n_decoders = MAX_DECODERS;
		}
		wchar_t filenamebuff[500];
		char buff[500];
		for(int i = 0; i<_n_decoders; i++){
			//decoder name
			//model file
			//tag file
			//feature file

			uis >> tok;
			_decoderNames[i] = tok.symValue();

			uis.getLine(filenamebuff, 499);
			wcstombs(buff, filenamebuff, 499);
			_secondaryWeights[i] = _new DTFeature::FeatureWeightMap(500009);
			DTFeature::readWeights(*_secondaryWeights[i], buff, PIdFFeatureType::modeltype);

			uis.getLine(filenamebuff, 499);
			wcstombs(buff, filenamebuff, 499);
			_secondaryTagSets[i] = _new DTTagSet(buff, true, true);

			uis.getLine(filenamebuff, 499);
			wcstombs(buff, filenamebuff, 499);
			_secondaryFeatureTypes[i] = _new DTFeatureTypeSet(buff, PIdFFeatureType::modeltype);

			//make a decoder
			_secondaryDecoders[i] = _new PDecoder(_secondaryTagSets[i], _secondaryFeatureTypes[i], _secondaryWeights[i]);
		}
	}
	else{
		_n_decoders = 0;
	}
}
PIdFSecondaryDecoders::~PIdFSecondaryDecoders(){
	for(int i = 0; i<_n_decoders; i++){
		delete _secondaryWeights[i];
		delete _secondaryFeatureTypes[i];
		delete _secondaryTagSets[i];
		delete _secondaryDecoders[i];
	}
}

void PIdFSecondaryDecoders::AddDecoderResultsToObservation(std::vector<DTObservation *> & observations)
{
	int tags[MAX_SENTENCE_TOKENS+2];
	for(int i=0; i< _n_decoders; i++){
		_secondaryDecoders[i]->decode(observations, tags);
		for(int j=1; j<(int)(observations.size()) - 1; j++){
			Symbol tagSym = _secondaryTagSets[i]->getTagSymbol(tags[j]);
			if((_secondaryTagSets[i]->getNoneSTTag() != tagSym) &&
				(_secondaryTagSets[i]->getNoneCOTag() != tagSym))
			{
				//std::cout<<"add "<<
				//	_decoderNames[i].to_debug_string()<<" "<<
				//	_secondaryTagSets[i]->getTagSymbol(tags[j]).to_debug_string()
				//	<<std::endl;



				static_cast<TokenObservation*>(observations[j])->addDecoderFeatures(_decoderNames[i],
				_secondaryTagSets[i]->getTagSymbol(tags[j]));
			}
		}
	}
}


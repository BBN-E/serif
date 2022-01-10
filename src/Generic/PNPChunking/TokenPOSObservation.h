// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKENPOS_OBSERVATION_H
#define TOKENPOS_OBSERVATION_H

#include "Generic/common/Symbol.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/Token.h"


/** A TokenObservation represents all the information about a token that the
  * Discriminative-IdF extracts features from.
  */

class TokenPOSObservation : public DTObservation {
public:
	TokenPOSObservation()
		: DTObservation(_className),
		  _token(Symbol()), _symbol(Symbol()),
		  _lcSymbol(Symbol()),
		  _pos(Symbol()),
		  _idfWordFeature(Symbol()),
		  _wordClass(WordClusterClass::nullCluster())
		  
	{}

	virtual DTObservation *makeCopy();

	/// Recycle the instance by entering new information into it.
	void populate(const Token &token, Symbol pos, Symbol _lcSymbol,
				  Symbol idfWordFeature, const WordClusterClass &wordClass);
    
	/// May return null pointer
	const Token &getToken() const { return _token; }
	Symbol getSymbol() const { return _symbol; }
	Symbol getLCSymbol() const {return _lcSymbol;}
	Symbol getPOS() const { return _pos; }
	Symbol getIdFWordFeature() const { return _idfWordFeature; }
	const WordClusterClass &getWordClass() const { return _wordClass; }

private:
	Token _token;
	Symbol _pos;
	Symbol _symbol;
	Symbol _lcSymbol;
	Symbol _idfWordFeature;
	WordClusterClass _wordClass;

	static const Symbol _className;
};

#endif

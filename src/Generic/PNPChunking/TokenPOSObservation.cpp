// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/PNPChunking/TokenPOSObservation.h"
#include "Generic/theories/Token.h"


const Symbol TokenPOSObservation::_className(L"token");

DTObservation *TokenPOSObservation::makeCopy() {
	TokenPOSObservation *copy = _new TokenPOSObservation();

	copy->populate(_token, _pos, _lcSymbol, _idfWordFeature, _wordClass);
	return copy;
}

void TokenPOSObservation::populate(const Token &token, Symbol pos, Symbol lcSymbol,
				Symbol idfWordFeature, const WordClusterClass &wordClass)
{
	_token = token;
	_symbol = _token.getSymbol();
	_lcSymbol = lcSymbol;
	_pos = pos;
	_idfWordFeature = idfWordFeature;
	_wordClass = wordClass;
}


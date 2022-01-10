// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"

class Attributes {
public:
	Attributes() {
		type = SymbolConstants::nullSymbol;
		role = SymbolConstants::nullSymbol;
	}

	Attributes(Symbol _type, int id) {
		type = _type;
		role = SymbolConstants::nullSymbol;
		coref_id = id;
	}

	Attributes(Symbol _type, Symbol _role, int id) {
		type = _type;
	    role = _role;
		coref_id = id;
	}

	// some documents has their id inside a tag
	// currenty only used in StandAloneSentenceBreaker
	// TODO: support it in Serif too
	void setDocumentID(Symbol docID) { this->docID = docID; }

	Symbol type;
	Symbol role;
	Symbol docID;
	int coref_id;
};

#endif

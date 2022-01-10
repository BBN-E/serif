// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ModularizedSerifTypes.hpp"

namespace serif {
	
	SerifCAS * ModularizedSerifTypes::NoDocumentFound = _new SerifCAS();
	SerifCAS * ModularizedSerifTypes::DocumentSourceClosed  = _new SerifCAS();

};



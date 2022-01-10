// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"

#include "Generic/common/Sexp.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/SPropTrees/SNodeContent.h"
#include "boost/pool/object_pool.hpp"

//WordNet* SNodeContent::wn=0; //can not initialize here, because initialization need opened parameter file

std::wstring SNodeContent::dummy=L"";
SNodeContent::Language language=SNodeContent::UNKNOWN;

boost::object_pool<SNodeContent>* SNodeContent::_nctPool = NULL;

void SNodeContent::initializeMemoryPool() { 
	if ( _nctPool != NULL ) delete _nctPool;
	_nctPool = _new boost::object_pool<SNodeContent>;
	if ( _nctPool == NULL ) {
		SessionLogger::info("SERIF") << "failure in memory allocation: _nctPool; bailing out\n";
		exit(-2);
	}
	return;
}

void SNodeContent::clearMemoryPool() { 
	if ( _nctPool != NULL ) delete _nctPool;
	_nctPool = NULL;
}


// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/PronounLinker.h"
#include "Generic/edt/xx_PronounLinker.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"

DebugStream PronounLinker::_debugStream;

PronounLinker::PronounLinker() {
	if(ParamReader::hasParam("pronlink_debug_file")){
		_debugStream.init(Symbol(L"pronlink_debug_file"), false);
	}
}


boost::shared_ptr<PronounLinker::Factory> &PronounLinker::_factory() {
	static boost::shared_ptr<PronounLinker::Factory> factory(new GenericPronounLinkerFactory());
	return factory;
}


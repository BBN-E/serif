// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TEMPORAL_NORMALIZER_H
#define TEMPORAL_NORMALIZER_H

#include "Generic/common/LocatedString.h"
#include <boost/shared_ptr.hpp>

using namespace std;

class DocTheory;

class TemporalNormalizer {
public:
	/** Create and return a new TemporalNormalizer. */
	static TemporalNormalizer *build() { return _factory()->build(); }
	/** Hook for registering new TemporalNormalizer factories */
	struct Factory { virtual TemporalNormalizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~TemporalNormalizer() {}
	
	virtual void normalizeTimexValues(DocTheory* docTheory) = 0;
	virtual void normalizeDocumentDate(const DocTheory *docTheory, std::wstring &docDateTime, std::wstring &additionalDocTime) = 0;

	virtual wstring parseDocumentDateTime(const LocatedString *ls) { return L""; }

protected:
	TemporalNormalizer() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/timex/en_TemporalNormalizer.h"
//#else 
//	#include "Generic/values/xx_TemporalNormalizer.h"
//#endif

#endif

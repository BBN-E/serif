#ifndef EN_NMEOUTPUTTER_H
#define EN_NMEOUTPUTTER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapStatus.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/TokenSequence.h" 
#include "Generic/theories/Token.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/EntityConstants.h"
#include "English/reader/en_DocumentReader.h"
#include "English/sentences/en_SentenceBreaker.h"
#include "Generic/results/ResultCollector.h"
#include "fb/results/FBResultCollector.h"
#include "Generic/adept/results/AdeptResultCollector.h"
#include "Generic/common/OutputUtil.h"
#include "English/generics/en_GenericsFilter.h"
#include "Generic/relationpromotion/RelationPromoter.h"
#include "en_NMEResults.h"

class NMEOutputter {
private:
	NameClassTags _nct;
public:
	NMEOutputter() {}
	///Caller owns and should delete the NMEResultVector
	NMEResultVector * to_enamex_sgml_string(SentenceTheory *theory,int offset) const;
	void outputfile(const char * source, const char * destdir, const char * filename, NMEResultVector * results) const;

};



#endif

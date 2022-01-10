// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
/*
#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Arabic/BuckWalter/ar_BWRuleDictionaryReader.h"
#include "Arabic/BuckWalter/ar_BWRuleDictionary.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"

#include <string>
#include <string.h>
#include <iostream>
#include <boost/scoped_ptr.hpp>

BWRuleDictionary* BWRuleDictionaryReader::readBWRuleDictionaryFile(const char *rule_dict_file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(rule_dict_file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	size_t numEntries;
	UTF8Token position1;
	UTF8Token position2;	
	UTF8Token category1;
	UTF8Token category2;

	stream >> numEntries;

	BWRuleDictionary* bwrd = BWRuleDictionary::getInstance();

	for(size_t line = 0; line < numEntries; line++){

		stream >> position1;
		stream >> position2;
		stream >> category1;
		stream >> category2;
		bwrd->addRule(position1.symValue(), category1.symValue(), 
					  position2.symValue(), category2.symValue());

	}	
	stream.close();
	return bwrd;
}
*/

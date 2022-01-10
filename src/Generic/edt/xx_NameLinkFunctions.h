// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_NAMELINKFUNCTIONS_H
#define xx_NAMELINKFUNCTIONS_H

#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/common/SessionLogger.h"

class GenericNameLinkFunctions {
	// Note: this class is intentionally not a subclass of
	// NameLinkFunctions.  See NameLinkFunctions.h for
	// an explanation.
public:
	// See NameLinkFunctions.h for method documentation.
	static bool populateAcronyms(const Mention *mention, EntityType type) { 
		defaultMsg();
		return false; 
	}
	static void destroyDataStructures() {
		defaultMsg();
	}
	static void recomputeCounts(CountsTable &inTable, 
								CountsTable &outTable, 
								int &outTotalCount)
	{
		defaultMsg();
		outTotalCount = -1;
		
    }

	static int getLexicalItems(Symbol words[], int nWords, Symbol results[], int max_results) {
		if (nWords > max_results) {
			SessionLogger::dbg("trunc_words_10") << "GenericNameLinkFunctions::getLexicalItems(): "
								  << "nWords > max_results, truncating words\n";
		}
		int i;
		for (i = 0; i < nWords && i < max_results; i++) {
			results[i] = words[i];
		}
		return i;
	}

private:
	static void defaultMsg(){
		SessionLogger::warn("unimplemented_class")<<"<<<<<<<<<<<<WARNING: Using an uninstantiated NameLinkFunctions class >>>>>>>>>>\n";
	}
};

#endif

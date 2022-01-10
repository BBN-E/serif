// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EXTERNAL_DICTIONARY_EXPANDER_H
#define EXTERNAL_DICTIONARY_EXPANDER_H

#include "PropTreeExpander.h"


class ExternalDictionaryExpander : public PropTreeExpander {
public:
	ExternalDictionaryExpander();		
	void expand(const PropNodes& pnodes) const;

private:

	typedef struct {
		std::wstring word;
		float score;
		Symbol pred_type;
	} dictionary_entry_t;

	typedef std::map< std::wstring, std::vector<dictionary_entry_t> > external_dictionary_t;
	external_dictionary_t _dictionary;

	void initializeTables();
};



#endif


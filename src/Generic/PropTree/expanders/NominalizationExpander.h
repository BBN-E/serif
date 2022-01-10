// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef NOMINALIZATION_EXPANDER_H
#define NOMINALIZATION_EXPANDER_H

#include "PropTreeExpander.h"


class NominalizationExpander : public PropTreeExpander {
public:
	NominalizationExpander(float nom_prob);		
	void expand(const PropNodes& pnodes) const;

private:
	float NOMINALIZATION_PROB;

	std::wstring nominalizeWord(const std::wstring& word) const;
	std::wstring verbifyWord(const std::wstring& word) const;

	std::map<std::wstring, std::wstring> _nominalizations;
	std::map<std::wstring, std::wstring> _verbifications;

	void initializeTables();
};



#endif


// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_PATTERN_H
#define ACTOR_PATTERN_H

#include "Generic/common/Symbol.h"
#include "Generic/actors/Identifiers.h"

#include <string>
#include <vector>

class ActorPattern {

public:

	ActorPatternId pattern_id;
	ActorId actor_id;
	Symbol entityTypeSymbol;
	std::vector<Symbol> pattern;
	std::vector<Symbol> lcPattern;
	std::wstring lcString;
	bool acronym;
	bool requires_context;
	float confidence;

	static std::wstring getNameFromSymbolList(std::vector<Symbol> &syms) {
		std::wstring name;

		for (size_t i = 0; i < syms.size(); i++) {
			std::wstring word = syms[i].to_string();
			if (name.size() > 0)
				name += L" ";
			name += word;
		}

		return name;
	}
};

#endif

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/RelationTypeSet.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include <boost/scoped_ptr.hpp>

bool RelationTypeSet::is_initialized = false;
int RelationTypeSet::INVALID_TYPE = -10000;
int RelationTypeSet::N_RELATION_TYPES = 0;
Symbol RelationTypeSet::relationConstants[];
Symbol RelationTypeSet::reversedRelationConstants[];

void RelationTypeSet::initialize() {

	if (!is_initialized) {
		relationConstants[0] = RelationConstants::NONE;
		reversedRelationConstants[0] = Symbol();
		N_RELATION_TYPES = 1;

		std::string type_file = ParamReader::getRequiredParam("relation_type_list");
		read_in_relation_types(type_file.c_str());

		is_initialized = true;
	}

}

void RelationTypeSet::read_in_relation_types(const char *filename) {

	boost::scoped_ptr<UTF8InputStream> typeStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& typeStream(*typeStream_scoped_ptr);
	UTF8Token token;
	typeStream >> N_RELATION_TYPES;

	// for NONE
	N_RELATION_TYPES += 1;

	if (N_RELATION_TYPES > MAX_RELATION_TYPES)
		throw UnexpectedInputException("RelationTypeSet::read_in_relation_types()",
		"too many relation types (hard-coded max of 1000)");

	typeStream >> token;
	Symbol symmetric = Symbol(L"S");
	for (int i = 1; i <= N_RELATION_TYPES; i++) {
		relationConstants[i] = token.symValue();
		UTF8Token next_token;
		typeStream >> next_token;
		if (next_token.symValue() != symmetric) {
			wchar_t rev[100];
			swprintf(rev, 99, L"%ls:reversed", token.chars());
			reversedRelationConstants[i] = Symbol(rev);
		} else {
			reversedRelationConstants[i] = Symbol();
			typeStream >> next_token;
		}
		token = next_token;
	}

}

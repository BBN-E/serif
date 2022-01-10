// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/PartOfSpeechTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"

const float PartOfSpeechTable::targetLoadingFactor = static_cast<float>(0.7);
PartOfSpeechTable::PartOfSpeechTable(UTF8InputStream& in)
{
    int numEntries;
    int numBuckets;
    UTF8Token token;
    Symbol word;
    TableEntry entry;
    int numTags;

	int max_pos_tags = ParamReader::getOptionalIntParamWithDefaultValue("maximum_pos_tags", 99999);
	bool prune_pos_tags = ParamReader::isParamTrue("prune_pos_tags");

    in >> numEntries;
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
    table = _new Table(numBuckets);
    for (int i = 0; i < numEntries; i++) {
        in >> token;
        if (in.eof()) return;
        if (token.symValue() != ParserTags::leftParen)
			throw UnexpectedInputException("PartOfSpeechTable::()",
                "ERROR: problem reading part-of-speech table");
        in >> token;
        word = token.symValue();
        in >> numTags;
     
        entry.tags = _new Symbol[numTags];

		int tag_count = 0;

		bool found_noun = false;
		bool found_verb = false;
		bool found_adj = false;

        for (int j = 0; j < numTags; j++) {
            in >> token;
            
			if (LanguageSpecificFunctions::isNoun(token.symValue()) && prune_pos_tags) {
				if (found_noun) 
					continue;
				found_noun = true;
			}

			if (LanguageSpecificFunctions::isVerbPOSLabel(token.symValue()) && prune_pos_tags) {
				if (found_verb) 
					continue;
				found_verb = true;
			}

			if (LanguageSpecificFunctions::isAdjective(token.symValue()) && prune_pos_tags) {
				if (found_adj)
					continue;
				found_adj = true;
			}

			if (tag_count < max_pos_tags)
				entry.tags[tag_count++] = token.symValue();
        }

		entry.numTags = tag_count;
        in >> token;
        if (token.symValue() != ParserTags::rightParen)
            throw UnexpectedInputException("PartOfSpeechTable::()",
                "ERROR: problem reading part-of-speech table");
        (*table)[word] = entry;
    }

}

PartOfSpeechTable::~PartOfSpeechTable() {
	if (table) {
		Table::iterator iter;
		for (iter = table->begin(); iter != table->end(); ++iter) {
			TableEntry tableEntry = (*iter).second;
			delete[] tableEntry.tags;
		}
		delete table;
	}
}

const Symbol* PartOfSpeechTable::lookup(Symbol word, int& numTags) const
{
    Table::iterator iter;

    iter = table->find(word);
    if (iter == table->end()) {
        numTags = 0;
        return static_cast<Symbol*>(0);
    }
    numTags = (*iter).second.numTags;
    return (*iter).second.tags;
}


// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/parse/VocabularyTable.h"

using namespace std;


const float VocabularyTable::targetLoadingFactor = static_cast<float>(0.7);

VocabularyTable::VocabularyTable(UTF8InputStream& in)
{
    int numEntries;
    int numBuckets;
    UTF8Token token;
    Symbol word;

    in >> numEntries;
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
    table = _new Symbol::HashSet(numBuckets);
    for (int i = 0; i < numEntries; i++) {
        in >> token;
        Symbol word = token.symValue();
        table->insert(word);
	}

}
VocabularyTable::VocabularyTable(UTF8InputStream& in, int threshold)
{
    int numEntries;
    int numBuckets;
    UTF8Token word_token;
	UTF8Token token;
	int count;
    Symbol word;

    in >> numEntries;

	size = 0;
	// adjust for high thresholds?
	numBuckets = static_cast<int>(numEntries / targetLoadingFactor);

    table = _new Symbol::HashSet(numBuckets);
    for (int i = 0; i < numEntries; i++) {
        in >> word_token;
		in >> count;
		if (count > threshold) {
			Symbol word = word_token.symValue();	//was token mrf
			table->insert(word);
			size++;
		}
    }
}

bool VocabularyTable::find(Symbol word) const
{
    Symbol::HashSet::iterator iter;

    iter = table->find(word);
    if (iter == table->end()) {
         return false;
    }
    return true;
}

void VocabularyTable::print(char* filename)
{
	Symbol::HashSet::iterator iter;

	UTF8OutputStream out;
	out.open(filename);


	out << size;
	out << '\n';

	for (iter = table->begin() ; iter != table->end() ; ++iter) {
		out << (*iter).to_string();
		out << '\n';
	}

	out.close();
}


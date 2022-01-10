// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTrainer/PrunerPOS.h"
#include "Generic/parse/ParserTrainer/VocabPruner.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"
#include <string>
const float PrunerPOS::targetLoadingFactor = static_cast<float>(0.7);

PrunerPOS::PrunerPOS(UTF8InputStream& in, VocabPruner* vp)
{
    int numEntries;
    int numBuckets;
    UTF8Token token;
	UTF8Token countToken;
    Symbol word;
	Symbol features;

	LinkedTag* old_tags;
	LinkedTag* old_tags_iter;
	LinkedTag* new_tag;
	LinkedTag* placeholder_tag;

	int read_numtags;
	Symbol read_tags[100];
	int read_counts[100];
	
	ParserTrainerLanguageSpecificFunctions::initialize();

	Symbol first_word_yes = Symbol(L"H1");

	size = 0;

    in >> numEntries;
	// what is the appropriate size for this hash table?
	// do we need more because we will be adding feature vectors? 
	// do we need fewer because we will be combining 
	//   ngrams that currently contain now irrelevant firstword info?
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);
    table = _new Table(numBuckets);


	// NB: ALL words get put in the POS table.
	// Words that don't meet the threshold ALSO
	// get put in as feature vectors.
	
	for (int i = 0; i < numEntries; i++) {

		// begin by reading all the tags for a given word
        in >> token;
        if (in.eof()) return;
        if (wcscmp(token.chars(), L"(") != 0)
			throw UnexpectedInputException("PrunerPOS::PrunerPOS",
                "ERROR: problem parsing part-of-speech input file");
        in >> token;
        word = Symbol(token.chars());
        in >> token;
	    features = Symbol(token.chars());
        in >> read_numtags;
        for (int j = 0; j < read_numtags; j++) {
            in >> token;
			read_tags[j] = Symbol(token.chars());
			in >> countToken;
			read_counts[j] = _wtoi(countToken.chars());
        }
        in >> token;
        if (wcscmp(token.chars(), L")") != 0)
            UnexpectedInputException("PrunerPOS::PrunerPOS",
               "ERROR: problem parsing part-of-speech input file");

		// get pointer to previous tags stored for this word, if any
		old_tags = lookup(word);

		bool this_tag_already_recorded;

		// if this word is already in the POS table
		if (old_tags) {

			placeholder_tag = old_tags;

			for (int i = 0; i < read_numtags; i++) {
				old_tags_iter = old_tags;
				this_tag_already_recorded = false;
				while (old_tags_iter != 0) {
					if (read_tags[i] == old_tags_iter->tag) {
						old_tags_iter->count = old_tags_iter->count + read_counts[i];
						this_tag_already_recorded = true;
						break;
					}
					old_tags_iter = old_tags_iter->next;
				}
				if (!this_tag_already_recorded) {
					new_tag = _new LinkedTag(read_tags[i], read_counts[i]);
					new_tag->next = placeholder_tag;
					placeholder_tag = new_tag;
				}
			}

			(*table)[word] = placeholder_tag;

		} else {
			placeholder_tag = _new LinkedTag(read_tags[0], read_counts[0]);
			new_tag = placeholder_tag;
			for (int j = 1; j < read_numtags; j++) {
				new_tag->next = _new LinkedTag(read_tags[j], read_counts[j]);
				new_tag = new_tag->next;
			}

			(*table)[word] = placeholder_tag;
			size++;
		}

		// now do it all again for feature vectors,
		//  when the word is not in the pruned vocabulary
		// the only difference is that only openClassTags
		//  can be added as tags for feature vectors
		if (!(vp->vocabularyTable->find(word))) {

			if (features == first_word_yes) 
				word = vp->wordFeatures->features(word, true);
			else 
				word = vp->wordFeatures->features(word, false);

			old_tags = lookup(word);

			placeholder_tag = old_tags;

			for (int j = 0; j < read_numtags; j++) {
				old_tags_iter = old_tags;
				this_tag_already_recorded = false;
				while (old_tags_iter != 0) {
					if (read_tags[j] == old_tags_iter->tag) {
						old_tags_iter->count = old_tags_iter->count + read_counts[j];
						this_tag_already_recorded = true;
						break;
					}
					old_tags_iter = old_tags_iter->next;
				}
				if (this_tag_already_recorded == false) {
					const std::vector<Symbol>& openClassTags = 
						ParserTrainerLanguageSpecificFunctions::openClassTags();
					for (size_t k = 0; k < openClassTags.size(); k++) {
						if (read_tags[j] == openClassTags[k]) 
						{
							new_tag = _new LinkedTag(read_tags[j], read_counts[j]);
							new_tag->next = placeholder_tag;
							placeholder_tag = new_tag;
						}
					}
				}

			}
			
			if (placeholder_tag != 0) {
				(*table)[word] = placeholder_tag;
				if (!(old_tags)) 
					size++;
			}
		}
	}
}


LinkedTag* PrunerPOS::lookup(Symbol word) {
	
	Table::iterator iter;

    iter = table->find(word);
    if (iter == table->end()) {
        return 0;
    }
    return (*iter).second;

}

//prints (word features #tags tag tag tag ... )
void PrunerPOS::print(char *filename)
{
	UTF8OutputStream out;
	out.open(filename);

	out << size;
	out << '\n';

	Table::iterator iter;

	for (iter = table->begin() ; iter != table->end() ; ++iter) {
		out << "(";
		out << (*iter).first.to_string();
		LinkedTag* value = (*iter).second;
		int count = 1;

		while (value->next != 0)
		{
			value = value->next;
			count++;
		}
		out << ' ';
		out << count;
		value = (*iter).second;

		while (value != 0) {
			int max = 0;
			LinkedTag *mostCommonTag = 0;
			LinkedTag *prev = 0;
			LinkedTag *beforeMostCommon = 0;

			LinkedTag *lt = value;

			// find most common tag
			while (lt != 0) {
				if (lt->count > max) {
					max = lt->count;
					mostCommonTag = lt;
					beforeMostCommon = prev;
				}
				prev = lt;
				lt = lt->next;
			}

			// print it out
			out << ' ';
			out << mostCommonTag->tag.to_string();

			// remove most common from list
			if (beforeMostCommon == 0) {
				value = mostCommonTag->next;
			} else {
				beforeMostCommon->next = mostCommonTag->next;
			}
		}
		out << ")\n";
	}

	out.close();

	return;
}


// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/parse/ParserTrainer/TrainerPOS.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/VocabularyTable.h"
#include "Generic/common/UTF8OutputStream.h"

const float TrainerPOS::targetLoadingFactor = static_cast<float>(0.7);

TrainerPOS::TrainerPOS(int init_size) 
{
	int numBuckets = static_cast<int>(init_size / targetLoadingFactor);
    table = _new Table(numBuckets);
	size = 0;
}



void TrainerPOS::add(Symbol* word, Symbol tag)
{
	Table::iterator iter; 

    iter = table->find(word);
    if (iter == table->end()) {
		Symbol* new_word = _new Symbol[2];
		new_word[0] = word[0];
		new_word[1] = word[1];
		LinkedTag* lt = _new LinkedTag(tag);
		(*table)[new_word] = lt;
		size++;
    }
    else {
		LinkedTag* list = (*iter).second;
		while (true) {
			if (tag == list->tag) {
				list->count = list->count + 1;
				break;
			}
			if (list->next == 0) {
				list->next = _new LinkedTag(tag);
				break;
			}
			list = list->next;
		}

	}

	return;
}

//prints (word features #tags tag count tag count tag count ... )
void TrainerPOS::print(char *filename)
{

	UTF8OutputStream out;
	out.open(filename);

	out << size;
	out << '\n';

	Table::iterator iter;

	for (iter = table->begin() ; iter != table->end() ; ++iter) {
		out << "(";
		out << (*iter).first[0].to_string();
		out << ' ';
		out << (*iter).first[1].to_string();
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
			
			// print it out
			out << ' ';
			out << value->tag.to_string();
			out << ' ';
			out << value->count;

			value = value->next;
		}
		
		out << ")\n";
	}

	out.close();

	return;
}

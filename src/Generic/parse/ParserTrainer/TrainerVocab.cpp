// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/parse/ParserTrainer/TrainerVocab.h"
#include "Generic/common/Symbol.h"
//#include "Generic/common/UTF8Token.h"

TrainerVocab::TrainerVocab(int NB)
{
	size = 0;
    table = _new Table(NB);

}

void TrainerVocab::add(Symbol s)
{
    Table::iterator iter;

    iter = table->find(s);
    if (iter == table->end()) {
		(*table)[s] = 1;
		size += 1;
    }
    else {
		(*iter).second += 1;
	}

	return;
}

// prints: word count
void TrainerVocab::print(char *filename)
{

	UTF8OutputStream out;
	out.open(filename);

	Table::iterator iter;

	out << size;
	out << '\n';

	for (iter = table->begin() ; iter != table->end() ; ++iter) {
		out << (*iter).first.to_string();
		out << ' ';
		out << (*iter).second;
		out << '\n';
	}

	out.close();

	return;
}

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/maxent/OldMaxEntFeatureTable.h"

const float OldMaxEntFeatureTable::targetLoadingFactor = static_cast<float>(0.7);

OldMaxEntFeatureTable::OldMaxEntFeatureTable(UTF8InputStream& stream) : _table(0) {
    int numEntries;
    int numBuckets;
    UTF8Token token;
	
	stream >> numEntries;
    numBuckets = static_cast<int>(numEntries / targetLoadingFactor);

	// numBuckets must be >= 1,
	//  but we set it to a minimum of 5, just to be safe
	// only relevant for very very small training sets, where
	//  there are no events of a certain type 
	if (numBuckets < 5)
		numBuckets = 5;


	_table = _new Table(numBuckets);
	_size = numEntries;

    for (int i = 0; i < numEntries; i++) {

        stream >> token;

        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed feature record at entry: %d %s", i, token.symValue().to_debug_string());
		  throw UnexpectedInputException("OldMaxEntFeatureTable::()", c);
        }

		stream >> token;

        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed feature record at entry: %d %s", i, token.symValue().to_debug_string());
		  throw UnexpectedInputException("OldMaxEntFeatureTable::()", c);
        }

		stream >> token;

        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
		  sprintf( c, "ERROR: ill-formed feature record at entry: %d  %s", i, token.symValue().to_debug_string());
		  throw UnexpectedInputException("OldMaxEntFeatureTable::()", c);
        }

		stream >> token; // first predicate member

		GrowableArray<Symbol> symArray;
		while (token.symValue() != SymbolConstants::rightParen) {
            symArray.add(token.symValue());
			stream >> token;
        }

		stream >> token; // outcome

		Feature *feature = _new Feature(token.symValue(), SymbolSet(symArray));

		stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed feature record at entry: %d %s", i, token.symValue().to_debug_string());
          throw UnexpectedInputException("OldMaxEntFeatureTable::()",c);
        }

        double score;
        stream >> score;

        stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR: ill-formed feature record at entry: %d  %s", i, token.symValue().to_debug_string());
          throw UnexpectedInputException("OldMaxEntFeatureTable::()",c);
        }

        (*_table)[feature] = score;

    }
}


OldMaxEntFeatureTable::OldMaxEntFeatureTable(int init_size)
    : _table(0)
{
	int numBuckets = static_cast<int>(init_size / targetLoadingFactor);

	// numBuckets must be >= 1,
	//  but we set it to a minimum of 5, just to be safe
	// only relevant for very very small training sets, where
	//  init_size is created as a fraction of some (small) integer
	
	if (numBuckets < 5)
		numBuckets = 5;
	_table = _new Table(numBuckets);
	_size = 0;
}


OldMaxEntFeatureTable::~OldMaxEntFeatureTable() {
	Table::iterator iter;

	for (iter = _table->begin(); iter != _table->end(); ++iter)
		delete (*iter).first;
	delete _table;
}


double OldMaxEntFeatureTable::lookup(Symbol outcome, SymbolSet predicate) const
{
	Feature feature(outcome, predicate);
    Table::iterator iter;

    iter = _table->find(&feature);
    if (iter == _table->end()) {
        return 0;
    }
    return (*iter).second;
}

void OldMaxEntFeatureTable::add(Symbol outcome, SymbolSet predicate)
{
	add(outcome, predicate, 1);
}

// creates new Feature iff it's a new key
void OldMaxEntFeatureTable::add(Symbol outcome, SymbolSet predicate, double value)
{
	Feature feature(outcome, predicate);
    Table::iterator iter;

    iter = _table->find(&feature);
    if (iter == _table->end()) {
		Feature *new_feature = _new Feature(feature);
		new_feature->setID(_size);
        (*_table)[new_feature] = value;
		_size += 1;
    }
    else {
		(*iter).second += value;
	}

	return;
}

void OldMaxEntFeatureTable::print(const char *filename)
{

	//ofstream out;
	UTF8OutputStream out;
	out.open(filename);
	print_to_open_stream (out);

	out.close();

	return;
}

// prints (((predicate) outcome) score)
void OldMaxEntFeatureTable::print_to_open_stream(UTF8OutputStream& out) 
{
	Table::iterator iter;

	out << _size;
	out << "\n";

	SymbolSet predicate;
	Symbol outcome;
	for (iter = _table->begin() ; iter != _table->end() ; ++iter) {
		out << "((";
		predicate = ((*iter).first)->getPredicate();
		out << predicate;
		out << " ";
		out << ((*iter).first)->getOutcome().to_string();
		out << ") ";
		out << (*iter).second;
		out << ")";
		out << "\n";
	}

}

// prints (((predicate) outcome) score)
void OldMaxEntFeatureTable::print_to_open_stream(DebugStream& out) 
{
	Table::iterator iter;

	out << _size;
	out << "\n";

	SymbolSet predicate;
	Symbol outcome;
	for (iter = _table->begin() ; iter != _table->end() ; ++iter) {
		out << "(((";
		predicate = ((*iter).first)->getPredicate();
		out << predicate.to_string();
		/*for (int i = 0; i < predicate.getNSymbols() - 1; i++) {
			out << predicate[i].to_string();
			out << " ";
		}
		out << predicate[predicate.getNSymbols() - 1].to_string();*/
		out << ") ";
		out << ((*iter).first)->getOutcome().to_string();
		out << ") ";
		out << (*iter).second;
		out << ")";
		out << "\n";
	}

}


// for use with min_history_count threshold
void OldMaxEntFeatureTable::prune(int threshold)
{
	int numBuckets = static_cast<int>(_size / targetLoadingFactor);
	if (numBuckets < 5)
		numBuckets = 5;
	Table* new_table = _new Table(numBuckets);

	Table::iterator iter;
	_size = 0;

	for (iter = _table->begin() ; iter != _table->end() ; ++iter) {
		// Add to new table if feature occurred more than threshold times
		// OR if it is a prior feature (empty predicate)
		if ((*iter).second >= threshold || (*iter).first->isPriorFeature()) {
			(*iter).first->setID(_size);
			(*new_table)[(*iter).first] = (*iter).second;
			_size++;
		}
		else {
			delete (*iter).first;   // delete feature since it's not getting added to new table
		}
	}

	delete _table;
	_table = new_table;
}

OldMaxEntFeatureTable::Table::iterator OldMaxEntFeatureTable::get_element(Symbol outcome, SymbolSet predicate) {
	Feature feature(outcome,predicate);
	return _table->find(&feature);
}

Feature::Feature(Symbol outcome, SymbolSet predicate) {
	_predicate = predicate;
	_outcome = outcome;
}

size_t Feature::hash_value() const { 
	return (_outcome.hash_code() >> 2) + _predicate.hash_value(); 
}

bool Feature::operator==(const Feature& f) const { 
	return (this->_predicate == f._predicate && this->_outcome == f._outcome); 
}

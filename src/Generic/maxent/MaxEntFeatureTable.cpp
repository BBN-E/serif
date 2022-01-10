// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/maxent/MaxEntFeatureTable.h"

const float MaxEntFeatureTable::targetLoadingFactor = static_cast<float>(0.7);


MaxEntFeatureTable::MaxEntFeatureTable(int init_size)
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


MaxEntFeatureTable::~MaxEntFeatureTable() {
	Table::iterator iter;

	for (iter = _table->begin(); iter != _table->end(); ++iter)
		delete (*iter).first;
	delete _table;
}


double MaxEntFeatureTable::lookup(DTFeature *feature) const
{
    Table::iterator iter;
	FeatureTableEntry entry(feature);
    iter = _table->find(&entry);
    if (iter == _table->end()) {
        return 0;
    }
    return (*iter).second;
}

void MaxEntFeatureTable::add(DTFeature *feature)
{
	add(feature, 1);
}

// creates new Feature iff it's a new key
void MaxEntFeatureTable::add(DTFeature *feature, double value)
{
    FeatureTableEntry entry(feature);
    Table::iterator iter;

    iter = _table->find(&entry);
    if (iter == _table->end()) {
		FeatureTableEntry *new_entry = _new FeatureTableEntry(feature);
		new_entry->setID(_size);
        (*_table)[new_entry] = value;
		_size += 1;
    }
    else {
		(*iter).second += value;
		// we already have a copy in the table, so get rid of this one
		feature->deallocate();
	}

	return;
}

// for use with min_history_count threshold
void MaxEntFeatureTable::prune(int threshold)
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
		if ((*iter).second >= threshold || 
			(*iter).first->getFeature()->getFeatureType()->getName() == Symbol(L"prior")) {
			(*iter).first->setID(_size);
			(*new_table)[(*iter).first] = (*iter).second;
			_size++;
		}
		else {
			// delete feature and table entry since it's not getting added to new table
			DTFeature *feature = (*iter).first->getFeature();
			feature->deallocate();
			delete (*iter).first;   
		}
	}

	delete _table;
	_table = new_table;
}

MaxEntFeatureTable::Table::iterator MaxEntFeatureTable::get_element(DTFeature *feature) {
	FeatureTableEntry entry(feature);
	return _table->find(&entry);
}

// prints (feature score)
void MaxEntFeatureTable::print_to_open_stream(UTF8OutputStream& out) 
{
	Table::iterator iter;

	out << _size;
	out << "\n";

	DTFeature *feature;
	for (iter = _table->begin() ; iter != _table->end() ; ++iter) {
		feature = (*iter).first->getFeature();
		out << L"((" << feature->getFeatureType()->getName().to_string()
			<< L" ";
		feature->write(out);
		out << L") " << (*iter).second<< L")\n";
	}

}


size_t FeatureTableEntry::hash_value() const { 
	return _feature->getHashCode(); 
}

bool FeatureTableEntry::operator==(const FeatureTableEntry& f) const { 
	return (*(this->_feature) == *(f._feature)); 
}

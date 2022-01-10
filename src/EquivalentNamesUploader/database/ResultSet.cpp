#include "common/leak_detection.h" // This must be the first #include

#include "Generic/common/InternalInconsistencyException.h"
#include "ResultSet.h"
#include "ValueVector.h"
#include <sstream>

ResultSet::ResultSet()
{
	_n_columns = 0;
}

ResultSet::~ResultSet()
{
	vector<ValueVector *>::iterator values_iter;
	for(values_iter = _column_values.begin(); values_iter != _column_values.end(); values_iter++)
	{
		delete (*values_iter);
	}
	_column_names.clear();
	_column_values.clear();
}

const char * ResultSet::getColumnName(size_t i) {
	if (i >= _column_names.size()) {
		stringstream stream;
		stream << "i: " << i << " length: " << _column_names.size();
		throw InternalInconsistencyException("ResultSet::getColumnName", stream.str().c_str());
	}
	return _column_names.at(i).c_str();
}

ValueVector * ResultSet::getColumnValues(size_t i) {
	if (i >= _column_values.size()) {
		stringstream stream;
		stream << "i: " << i << " length: " << _column_values.size();
		throw InternalInconsistencyException("ResultSet::getColumnValues", stream.str().c_str());
	}
	return _column_values.at(i);
}

void ResultSet::addColumn(const char * column_name, ValueVector * values) {
	_column_names.push_back(column_name);
	_column_values.push_back(values);
	_n_columns++;
}

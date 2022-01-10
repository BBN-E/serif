#pragma once

#include <iostream>
#include <vector>
class ValueVector;

using namespace std;

class ResultSet
{
public:
	ResultSet();
	~ResultSet();

	const char * getColumnName(size_t i);
	size_t getNColumns() {return _n_columns;}
	ValueVector * getColumnValues(size_t i);
	void addColumn(const char * column_name, ValueVector * values);

private:
	vector <string> _column_names;
	vector <ValueVector *> _column_values;
	size_t _n_columns;
};

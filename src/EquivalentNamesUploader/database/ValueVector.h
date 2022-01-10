#pragma once

#include <iostream>
#include <vector>

using namespace std;

// The database truncates most values longer than this. Ensure your use 
// of valuevector doesn't assume individual values longer than this define
#define VALUEVECTOR_MAX_SQL_VALUE_LENGTH 128

class ValueVector
{
public:
	ValueVector();
	~ValueVector();

	void addValue(const wchar_t * value);
    void addValue(const wstring& value);
    void addValueIfNotAlreadyInVector(const wstring& value);
    void eraseValue(size_t i);
	const wchar_t * getValue(size_t i) const;
	wstring getValueForSQL(size_t i) const;
	size_t getNValues() const { return _n_values; }
    size_t size() const { return _n_values; }
	bool contains(const wchar_t * value) const; 
    bool contains(const wstring value) const { return contains(value.c_str()); }
private:
	vector <wstring> _values;
	size_t _n_values;

};

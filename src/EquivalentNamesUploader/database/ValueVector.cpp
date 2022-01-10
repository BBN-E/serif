#include "common/leak_detection.h" // This must be the first #include

#include "ValueVector.h"

ValueVector::ValueVector()
{
	_n_values = 0;
}

ValueVector::~ValueVector()
{
	_values.clear();
}

void ValueVector::addValue(const wchar_t * value) {
	_values.push_back(value);
	_n_values++;
}

void ValueVector::addValue(const wstring& value) {
	_values.push_back(value);
	_n_values++;
}

void ValueVector::addValueIfNotAlreadyInVector(const wstring& value) {
    if (!contains(value)) {
        addValue(value);
    }
}

void ValueVector::eraseValue(size_t i) {
    if (i < _values.size()) {
        _values.erase(_values.begin() + i);
        _n_values--;
    }
}

const wchar_t * ValueVector::getValue(size_t i) const {
	return _values.at(i).c_str();
}

bool ValueVector::contains(const wchar_t * value) const {
	for (size_t i = 0; i < _n_values; i++) {
        if (wcscmp(value, _values.at(i).c_str()) == 0)
            return true;
    }
    return false;
}

wstring ValueVector::getValueForSQL(size_t i) const {

    wstring sqlval = _values.at(i).substr( 0, VALUEVECTOR_MAX_SQL_VALUE_LENGTH );
    for( wstring::size_type it = sqlval.find( L"'" ); it != wstring::npos; it = sqlval.find( L"'", it+2 ) ) {
		sqlval.replace( it, 1, L"''" ); // Replace ' with ''
    }
    for( wstring::size_type it = sqlval.find( L'"' ); it != wstring::npos; it = sqlval.find( L'"')) {
		sqlval.replace( it, 1, L"" );   // Remove " (Quotation marks break the sql query, and are ignored by the db in full text searches)
    }
	return sqlval;
}

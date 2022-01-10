#include "Generic/common/leak_detection.h"

#include <string>
#include <iostream>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "StringStore.h"
#include "Generic/common/UTF8InputStream.h"

using std::string; using std::wstring; using std::getline;

using boost::lexical_cast; using boost::trim; using boost::make_shared;

StringStore::StringStore(unsigned int size) :
	_n_strings(size) 
{
}

unsigned int StringStore::size() const {
	return _n_strings;
}

SimpleInMemoryStringStore::SimpleInMemoryStringStore(unsigned int size)
	: StringStore(size), _strings(size)
{
}

SimpleInMemoryStringStore_ptr SimpleInMemoryStringStore::create(
	const std::string& strings_file, unsigned int size) 
{
	SimpleInMemoryStringStore_ptr store = make_shared<SimpleInMemoryStringStore>(size);
	boost::scoped_ptr<UTF8InputStream> inp(UTF8InputStream::build(strings_file.c_str()));
	wstring line;

	getline(*inp, line);
	trim(line);

	size_t idx = 0;
        try {
            // we might have a line listing the number of entries
            // at the top of the file. If so, ignore it
            int dummy = lexical_cast<int>(line);
        } catch (boost::bad_lexical_cast&) {
            store->storeString(static_cast<int>(idx), line);
			++idx;
        }

	while (getline(*inp, line)) {
		trim(line);
		store->storeString(static_cast<int>(idx), line);
		++idx;
	}

	if (idx != store->size()) {
		std::stringstream err;
		err << "Preview strings file claims to have " << store->size() << 
			" lines but actually has " << idx;
		throw UnexpectedInputException("SimpleInMemoryStringStore::create",
			err.str().c_str());
	}

	return store;
}

void SimpleInMemoryStringStore::storeString(unsigned int idx, const std::wstring& line) {
	_strings[idx] = line;
}

const std::wstring SimpleInMemoryStringStore::getString(unsigned int idx) const {
	return _strings[idx];
}

DummyStringStore::DummyStringStore(unsigned int size)
	: StringStore(size)
{}


const std::wstring DummyStringStore::getString(unsigned int idx) const {
	return L"";
}

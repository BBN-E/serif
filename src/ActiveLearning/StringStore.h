#ifndef _LEARNIT_STRINGSTORE_H_
#define _LEARNIT_STRINGSTORE_H_
// This file is here because we anticipate our memory needs will grow to the
// point that storing the strings all in memory is infeasible.
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/bsp_declare.h"

class StringStore {
public:
	virtual const std::wstring getString(unsigned int idx) const = 0;
	unsigned int size() const;
protected:
	StringStore(unsigned int size);
private:
	unsigned int _n_strings;
};
typedef boost::shared_ptr<StringStore> StringStore_ptr;

BSP_DECLARE(SimpleInMemoryStringStore);

class SimpleInMemoryStringStore : public StringStore {
public:
	static SimpleInMemoryStringStore_ptr create(const std::string& strings_file, unsigned int sz);
	virtual const std::wstring getString(unsigned int idx) const;
protected:
	SimpleInMemoryStringStore(unsigned int size);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(SimpleInMemoryStringStore, unsigned int);
private:
	void storeString(unsigned int idx, const std::wstring& str);
	std::vector<std::wstring> _strings;
};

BSP_DECLARE(DummyStringStore);

class DummyStringStore : public StringStore {
public:
	DummyStringStore(unsigned int size);
	virtual const std::wstring getString(unsigned int idx) const;
};

#endif

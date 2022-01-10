#ifndef _ALPHABET_PR_H_
#define _ALPHABET_PR_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

namespace GraphicalModel {
class Alphabet {
public:
	Alphabet();
	unsigned int feature(const std::wstring& featureString);
	unsigned int feature(const std::wstring& featureString) const;
	unsigned int featureAlreadyPresent(const std::wstring& featureString) const;
	const std::wstring& name(unsigned int featureIdx) const; 
	void dump() const;
	size_t size() const;
private:
	typedef std::map<std::wstring, unsigned int> StringToInt;
	StringToInt stringToInt;
	typedef std::map<unsigned int, std::wstring> IntToString;
	IntToString intToString;
};

typedef boost::shared_ptr<Alphabet> Alphabet_ptr;

class BadLookupException {};
};
#endif


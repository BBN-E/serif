#pragma once

#include <set>
#include <string>
#include <vector>
#include "boost/shared_ptr.hpp"
#include "ObjectWithSlots.h"

/**
 * A pair of entities that might satisfy a given target relation.
 */
class Seed;
typedef boost::shared_ptr<Seed> Seed_ptr;
class Seed : public ObjectWithSlots {
public:
	Seed(Target_ptr target, std::vector<std::wstring> const &slots, bool active, std::wstring const &source, 
			std::wstring const &language = L"");
	~Seed();

	std::wstring toString() const;
	bool active() const { return _active; }
	std::wstring source() const { return _source; }
	std::wstring language() const { return _language; }

private:
	bool _active;
	std::wstring _source;
	std::wstring _language;
};

inline std::wostream& operator<<(std::wostream& out, const Seed& seed) 
{ out << seed.toString(); return out;}
inline std::wostream& operator<<(std::wostream& out, const Seed_ptr& seed) 
{ out << seed->toString(); return out;}

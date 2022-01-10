#include <set>

#include "Generic/theories/EntitySubtype.h"
class Entity;
class EntitySet;
#pragma once

class PlaceInfo{

public:
	PlaceInfo();
	PlaceInfo(const Entity* entity, const EntitySet* entity_set, int index);
	const Entity* getEntity() {return _entity;}
	int getIndex() {return _index;}
	bool isInLookupTable() {return _is_in_lookup_table;}
	void setIsInLookupTable(bool bVal) {_is_in_lookup_table = bVal;}
	EntitySubtype getSubtype() {return _subtype;}
	std::set<int> & getWKSubLocations() {return _wk_sub_locations;}
	std::set<int> & getWKSuperLocations() {return _wk_super_locations;}
	bool isCityLike();
	bool isStateLike();
	bool isNationLike();

private:
	const Entity* _entity;
	int _index;
	bool _is_in_lookup_table;
	EntitySubtype _subtype;
	std::set<int> _wk_sub_locations;
	std::set<int> _wk_super_locations;

};

#include "Generic/common/leak_detection.h"

#include "PredFinder/inference/PlaceInfo.h"

#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/EntitySet.h"

PlaceInfo::PlaceInfo()
: _entity(0), _index(-1), _is_in_lookup_table(false) 
{
	_subtype = EntitySubtype::getUndetType();
}

PlaceInfo::PlaceInfo(const Entity* entity, const EntitySet* entity_set, int index)
: _entity(entity), _index(index), _is_in_lookup_table(false) 
{
	_subtype = entity_set->guessEntitySubtype(entity);
}

bool PlaceInfo::isCityLike(){
	//city level
	static const Symbol local = Symbol(L"Region-Local");
	static const Symbol county = Symbol(L"County-or-District");
	static const Symbol city = Symbol(L"Population-Center");
	return _subtype.getName() == local || _subtype.getName() == county || _subtype.getName() == city;

}
bool PlaceInfo::isNationLike(){
	static const Symbol nation = Symbol(L"Nation");
	return _subtype.getName() == nation;
}
bool PlaceInfo::isStateLike(){
	static const Symbol subnational = Symbol(L"Region-Subnational");
	static const Symbol state = Symbol(L"State-or-Province");
	return _subtype.getName() == subnational || _subtype.getName() == state;
	
}


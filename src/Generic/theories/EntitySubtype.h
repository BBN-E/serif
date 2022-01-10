// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_SUBTYPE_H
#define ENTITY_SUBTYPE_H


#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/EntityType.h"
#include <vector>

#define ENTITY_SUBTYPE_UNDET_INDEX 0

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED EntitySubtype {
public:

	/** default constructor makes UNDET type */
	EntitySubtype() : _info(getUndetType()._info) {}

	/** This constructor throws an UnexpectedInputException if it doesn't
	  * recognize the name Symbol and allow_dynamic_entity_types is false
	  * When you construct an entity subtype object, and there is the
	  * the possibility that the Symbol doesn't correspond to an actual
	  * type, then you should probably trap that exception so that it
	  * it doesn't halt Serif. */
	EntitySubtype(Symbol fullTypeName);	
	EntitySubtype(Symbol entityTypeName, Symbol entitySubtypeName);

	static bool subtypesDefined() { 
		initEntitySubtypeInfo();
		return (_subtypes.subtypes.size() > 1);
	}

	/// Gets subtype's name (e.g., Country, Commercial, etc.)
	Symbol getName() const { return _info->name; }

	EntityType getParentEntityType() const { return _info->parentEntityType; }

	/// Convenience string operations that split on '.'
	static Symbol splitEntityTypeName(Symbol fullTypeName);
	static Symbol splitEntitySubtypeName(Symbol fullTypeName);
	static Symbol joinEntityTypeAndSubtypeNames(Symbol entityTypeName, Symbol entitySubtypeName);

	/// true iff the type is anything but UNDET
	bool isDetermined() const {
		return _info->index != ENTITY_SUBTYPE_UNDET_INDEX;
	}

	bool operator==(const EntitySubtype &other) const {
		return _info == other._info;
	}
	bool operator!=(const EntitySubtype &other) const {
		return _info != other._info;
	}
	EntitySubtype &operator=(const EntitySubtype &other) {
		_info = other._info;
		return *this;
	}

	static EntitySubtype getUndetType() {
		return EntitySubtype(ENTITY_TYPE_UNDET_INDEX);
	}

	static EntitySubtype getDefaultSubtype(EntityType entityType);
	static int getNSubtypes() {
		initEntitySubtypeInfo();
		return static_cast<int>(_subtypes.subtypes.size());
	}
	static EntitySubtype getSubtypeByIndex(int index);
	static int getSubtypeIndex(Symbol entityTypeName, Symbol entitySubtypeName);

	static bool isValidEntitySubtype(Symbol possibleTypeName);	
	static bool isValidEntitySubtype(EntityType entityType, EntitySubtype entitySubtype);	

private:
	struct EntitySubtypeInfo {
		int index;
		Symbol name;
		EntityType parentEntityType;
	};

	typedef boost::shared_ptr<EntitySubtypeInfo> EntitySubtypeInfo_ptr;

	EntitySubtypeInfo_ptr _info;

	EntitySubtype(int index);

	typedef Symbol::HashMap<int> EntitySubtypeMap;

	struct EntitySubtypes {
		EntitySubtypes() {}

		std::vector<EntitySubtypeInfo_ptr> subtypes;

		EntitySubtypeMap subtypesByFullName;

		std::vector<int> defaultSubtypes;
	};

	static EntitySubtypes _subtypes;

	static const EntitySubtypeInfo_ptr getEntitySubtypeInfo(int index);

	static const EntitySubtypeInfo_ptr getEntitySubtypeInfo(Symbol name);

	static const EntitySubtypeInfo_ptr addEntitySubtypeInfo(Symbol name);

	static void initEntitySubtypeInfo();

	static EntityType getParentEntityType(Symbol sym);
};

#endif

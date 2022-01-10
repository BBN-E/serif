// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H


#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include <vector>
#include <boost/shared_ptr.hpp>

#define ENTITY_TYPE_UNDET_INDEX 0
#define ENTITY_TYPE_OTH_INDEX 1

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED EntityType {
public:

	/** default constructor makes UNDET type */
	EntityType() : _info(getUndetType()._info) {}

	/** This constructor throws an UnexpectedInputException if it doesn't
	  * recognize the name Symbol and allow_dynamic_entity_types is false.
	  * When you construct an entity type object, and there is the
	  * the possibility that the Symbol doesn't correspond to an actual
	  * type, then you should probably trap that exception so that it
	  * it doesn't halt Serif. */
	EntityType(Symbol name);

	/// Gets type's unique number, which corresponds to getType(int i)
	int getNumber() const { return _info->index - 1; }

	/// Gets type's name (e.g., PER, MonetaryValue, etc.)
	Symbol getName() const { return _info->name; }

	/// true iff the type is anything but UNDET
	bool isDetermined() const {
		return _info->index != ENTITY_TYPE_UNDET_INDEX;
	}

	/// true iff the type is anything but UNDET or OTH
	bool isRecognized() const {
		return _info->index != ENTITY_TYPE_UNDET_INDEX &&
			   _info->index != ENTITY_TYPE_OTH_INDEX;
	}

	/// true iff IdF can find this type as a descriptor
	bool isIdfDesc() const { return _info->idf_desc; }

	Symbol getPrimaryTag() const { return _info->primary_tag; }

	Symbol getSecondaryTag() const { return _info->secondary_tag; }

	Symbol getDefaultTag() const { return _info->default_tag; }

	Symbol getDefaultWord() const { return _info->default_word; }

	/// true iff mentions of this type may be relation arguments
	bool canBeRelArg() const { return _info->rel_arg; }

	/// true iff mentions of this type should be linked into entities
	bool isLinkable() const { return _info->linkable; }

	/// true iff mentions of this type are temporal expressions
	bool isTemp() const { return _info->is_temp; }

	/// true for "Russians", but also "Catholics", "Democrats", etc.
	bool isNationality() const { return _info->is_nationality; }

	/// true iff this type can nest inside other types
	bool isNestable() const { return _info->is_nestable; }

	/// true iff this type should not used trained coref
	bool useSimpleCoref() const { return _info->use_simple_coref;}

	bool matchesPER() const { return _info->matches_PER; }
	bool matchesORG() const { return _info->matches_ORG; }
	bool matchesGPE() const { return _info->matches_GPE; }
	bool matchesFAC() const { return _info->matches_FAC; }
	bool matchesLOC() const { return _info->matches_LOC; }

	bool operator==(const EntityType &other) const {
		return _info == other._info;
	}
	bool operator!=(const EntityType &other) const {
		return _info != other._info;
	}
	EntityType &operator=(const EntityType &other) {
		_info = other._info;
		return *this;
	}

	static EntityType getUndetType() {
		return EntityType(ENTITY_TYPE_UNDET_INDEX);
	}
	static EntityType getOtherType() {
		return EntityType(ENTITY_TYPE_OTH_INDEX);
	}

	// Note: the following functions, used for getting entity types
	// with particular properties, must only be called if you *know* that
	// the type is available, or you catch the
	// InternalInconsistencyException that might get thrown.
	// In fact, these should probably never be used in the Serif core
	// library.
	static EntityType getNationalityType(); /// crashes if type not defined!
	static EntityType getTempType(); /// crashes if type not defined!

	static EntityType getPERType(); /// crashes if type not defined!
	static EntityType getORGType(); /// crashes if type not defined!
	static EntityType getGPEType(); /// crashes if type not defined!
	static EntityType getLOCType(); /// crashes if type not defined!
	static EntityType getFACType(); /// crashes if type not defined!
	static EntityType getABSType(); /// crashes if type not defined! (currently in Spanish for ideas, statements, etc)

	/** Get the number of types (not counting UNDET) */
	static int getNTypes() {
		initEntityTypeInfo();
		return static_cast<int>(_types.types.size()) - 1; // subtract 1 to omit UNDET
	}
	/** Get type #i */
	static EntityType getType(int i) {
		return EntityType(i + 1); // shift index to skip UNDET
	}

	static bool isValidEntityType(Symbol possibleType);

private:
	struct EntityTypeInfo {
		int index;
		Symbol name;
		bool idf_desc;
		bool rel_arg;
		bool linkable;
		bool is_temp;
		bool is_nationality;
		bool is_nestable;
		bool use_simple_coref;
		bool matches_PER;
		bool matches_ORG;
		bool matches_GPE;
		bool matches_FAC;
		bool matches_LOC;
		Symbol primary_tag;
		Symbol secondary_tag;
		Symbol default_tag;
		Symbol default_word;
	};

	typedef boost::shared_ptr<EntityTypeInfo> EntityTypeInfo_ptr;

	EntityTypeInfo_ptr _info;

	EntityType(int index);

	typedef Symbol::HashMap<int> EntityTypeMap;

	struct EntityTypes {
		EntityTypes()
			: PER_index(ENTITY_TYPE_OTH_INDEX),
			  ORG_index(ENTITY_TYPE_OTH_INDEX),
			  GPE_index(ENTITY_TYPE_OTH_INDEX),
			  LOC_index(ENTITY_TYPE_OTH_INDEX),
			  FAC_index(ENTITY_TYPE_OTH_INDEX),
			  nationality_index(ENTITY_TYPE_OTH_INDEX),
			  temp_index(ENTITY_TYPE_OTH_INDEX),
			  ABS_index(ENTITY_TYPE_OTH_INDEX)
		{}

		std::vector<EntityTypeInfo_ptr> types;

		EntityTypeMap typesByName;

		int PER_index;
		int ORG_index;
		int GPE_index;
		int LOC_index;
		int FAC_index;
		int nationality_index;
		int temp_index;
		int ABS_index;
	};

	static EntityTypes _types;

	static const EntityTypeInfo_ptr getEntityTypeInfo(int index);

	static const EntityTypeInfo_ptr getEntityTypeInfo(Symbol name);

	static const EntityTypeInfo_ptr addEntityTypeInfo(Symbol name);

	static void initEntityTypeInfo();
};


#endif


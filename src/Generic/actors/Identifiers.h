// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDENTIFIERS_H
#define IDENTIFIERS_H
#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include <boost/cstdint.hpp>

extern const Symbol default_db_name;

/** Identifiers that are used by ICEWS theories to point into an external
* database containing agents, actors, and other objects.  In particular,
* these identifiers correspond with primary keys in ICEWS database
* tables.  A separate identifier class is defined (by pairing a template
* with a unique tag class) for each unique primary key column.
*
* The identify of each identifier is determined by its "id" field,
* which is a 32-bit integer SQL primary key, and a "db_name" defined
* in a configuration file while uniquely refers to the containing SQL
* database.
* Default-constructed identifiers are given a special "NULL" value.  This
* value only compares equal with itself.
*/
template<typename Tag, typename IdType>
class ICEWSIdentifier {
public:
	typedef IdType id_type;

	ICEWSIdentifier(): _id(0), _is_null(true) {}
	explicit ICEWSIdentifier(id_type id, Symbol db_name = default_db_name)
		: _db_name(db_name), _id(id), _is_null(false) {}
	ICEWSIdentifier(const ICEWSIdentifier<Tag, IdType> &other)
		: _db_name(other._db_name), _id(other._id), _is_null(other._is_null) {}

	bool operator==(const ICEWSIdentifier<Tag, IdType> &other) const 
	{ return (_is_null==other._is_null) && (_db_name==other._db_name) && (_id==other._id); }
	bool operator!=(const ICEWSIdentifier<Tag, IdType> &other) const
	{ return !((*this)==other); }
	bool operator< (const ICEWSIdentifier<Tag, IdType> &other) const {
		if (_db_name < other._db_name)
			return true;
		if (_db_name == other._db_name)
			return _id < other._id; 
		return false;
	}
	ICEWSIdentifier<Tag, IdType>& operator=(const ICEWSIdentifier<Tag, IdType> &other) 
	{ _db_name = other._db_name; _id=other._id; _is_null=other._is_null; return *this; }

	Symbol getDbName() const { return _db_name; }
	id_type getId() const { return _id; }
	bool isNull() const { return _is_null; }

	struct HashOp {size_t operator()(const ICEWSIdentifier &v) const {return v._db_name.hash_code() ^ static_cast<size_t>(v._id);}};
	struct EqualOp {bool operator()(const ICEWSIdentifier &v1, const ICEWSIdentifier &v2) const {return v1 == v2;}};

	template<typename ValueType>
	class HashMap: public serif::hash_map<ICEWSIdentifier, ValueType, HashOp, EqualOp> {};
	typedef hash_set<ICEWSIdentifier, HashOp, EqualOp> HashSet;
private:
	Symbol _db_name;
	id_type _id;
	bool _is_null;
};

// Allows ICEWSIdentifiers to be used as keys in std::map etc.
#if !defined(_WIN32)
#if defined(__APPLE_CC__)
namespace std {
#else
#include <ext/hash_map>
namespace __gnu_cxx {
#endif
	template<typename Tag, typename IdType> struct hash<ICEWSIdentifier<Tag, IdType> > {
	size_t operator()( const ICEWSIdentifier<Tag, IdType> & v ) const {return v._db_name.hash_code() ^ static_cast<size_t>(v._id);};
	};
}
#endif

// IdType will typically be either SQLInt or SQLBigInt.
typedef boost::int32_t SQLInt;
typedef boost::int64_t SQLBigInt;

struct ActorPatternIdTag {};
/** Identifier type for actor patterns (primary key in the dict_actorpatterns table) */
typedef ICEWSIdentifier<ActorPatternIdTag, SQLInt> ActorPatternId;

struct AgentPatternIdTag {};
/** Identifier type for agent patterns (primary key in the dict_agentpatterns table) */
typedef ICEWSIdentifier<AgentPatternIdTag, SQLInt> AgentPatternId;

struct ActorIdTag {};
/** Identifier type for actors (primary key in the dict_actors table) */
typedef ICEWSIdentifier<ActorIdTag, SQLInt> ActorId;

struct AgentIdTag {};
/** Identifier type for agents (primary key in the dict_agents table) */
typedef ICEWSIdentifier<AgentIdTag, SQLInt> AgentId;

struct ICEWSEventTypeIdTag {};
/** Identifier type for event types (primary key in the eventtypes table) */
typedef ICEWSIdentifier<ICEWSEventTypeIdTag, SQLInt> ICEWSEventTypeId;

struct CountryIdTag {};
/** Identifier type for countries (primary key in the countries table) */
typedef ICEWSIdentifier<CountryIdTag, SQLInt> CountryId;

struct SectorIdTag {};
/** Identifier type for sectors (primary key in the dict_sectors table) */
typedef ICEWSIdentifier<SectorIdTag, SQLInt> SectorId;

struct StoryIdTag {};
/** Identifier type for stories (primary key in the stories table) */
typedef ICEWSIdentifier<StoryIdTag, SQLBigInt> StoryId;

/** A CompositeActorId is just a pair of AgentId & ActorId.  This is used 
* when matching "precomposited composite actors", i.e., actors specified
* in the "composite_actor_patterns.txt" file. */
struct CompositeActorId: public std::pair<AgentId, ActorId> {
	CompositeActorId(): std::pair<AgentId, ActorId>() {}
	CompositeActorId(AgentId pairedAgentId, ActorId pairedActorId)
		: std::pair<AgentId, ActorId>(pairedAgentId, pairedActorId) {}
	struct HashOp {size_t operator()(const CompositeActorId &v) const {return static_cast<size_t>(v.first.getId()+v.second.getId());}};
	struct EqualOp {bool operator()(const CompositeActorId &v1, const CompositeActorId &v2) const { return v1==v2; }};
	template<typename ValueType>
	class HashMap: public serif::hash_map<CompositeActorId, ValueType, HashOp, EqualOp> {};
	AgentId getPairedAgentId() const { return first; }
	ActorId getPairedActorId() const { return second; }
	bool isNull() const { return first.isNull(); }
};



template<typename Tag, typename IdType>
std::ostream & operator << ( std::ostream &stream, ICEWSIdentifier<Tag,IdType> id ) {
	if (id.isNull()) stream << "NULL";
	else stream << id.getId();
	return stream;
}

template<typename Tag, typename IdType>
std::wostream & operator << ( std::wostream &stream, ICEWSIdentifier<Tag,IdType> id ) {
	if (id.isNull()) stream << L"NULL";
	else stream << id.getId();
	return stream;
}

#endif

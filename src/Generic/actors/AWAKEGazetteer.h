// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KNOWLEDGE_BASE_GAZETTEER_H
#define KNOWLEDGE_BASE_GAZETTEER_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/Gazetteer.h"
#include <vector>
#include <boost/regex.hpp>
#include <boost/make_shared.hpp>
#include <boost/cstdint.hpp>

// Forward declaration
class DatabaseConnection;


class AWAKEGazetteer : public Gazetteer {
public:

	AWAKEGazetteer();
	~AWAKEGazetteer();

	/* Look up a location by name in the icews database country table
	and return a GeoResolution pointer for each potential match */
	virtual std::vector<GeoResolution_ptr> countryLookup(std::wstring locationName);

	virtual DatabaseConnection_ptr getSingletonGeonamesDb();

	virtual Gazetteer::GeoResolution_ptr getCountryResolution(std::wstring country_iso_code);

private:

	Symbol _actorDBName;

	std::map<std::wstring, Gazetteer::GeoResolution_ptr> _countryResolutionCache;

};


#endif

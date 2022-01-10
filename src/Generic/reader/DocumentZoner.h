// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_ZONER_H
#define DOCUMENT_ZONER_H

#include <boost/shared_ptr.hpp>
#include <vector>

#include "Generic/driver/DocumentDriver.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

/**
 * The DocumentZoner class defines a separate factory hook for each 
 * zoning_method. Entry point is the overridden process() method since
 * zoners are stage handlers.
 **/
class SERIF_EXPORTED DocumentZoner : public DocumentDriver::DocTheoryStageHandler {
public:
	// Create and return a new DocumentZoner; there is no default zoner yet
	static DocumentZoner* build(const char* zoning_method);

	// Hook for registering new factories
	//   This is a subclass so that we can use the factory when adding a stage
	struct Factory : public DocumentDriver::DocTheoryStageHandlerFactory { 
		virtual ~Factory() {}
	};		
	static void setFactory(const char* zoning_method, boost::shared_ptr<Factory> factory);
	static bool hasFactory(const char* zoning_method);

	// Expose factories so that the DocumentDriver can associate them with Stages
	static boost::shared_ptr<Factory> getFactory(const char* zoning_method) { return _factory(zoning_method); }

	virtual ~DocumentZoner() {}

protected:
	DocumentZoner() {}

private:
	typedef std::map<std::string, boost::shared_ptr<Factory> > FactoryMap; 
	static boost::shared_ptr<Factory>& _factory(const char* zoning_method);
	static FactoryMap& _factoryMap();
};

#endif

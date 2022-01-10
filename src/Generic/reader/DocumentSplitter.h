// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_SPLITTER_H
#define DOCUMENT_SPLITTER_H

#include <boost/shared_ptr.hpp>
#include <vector>

#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

/**
 * The DocumentSplitter class defines a separate factory hook for each 
 * split_strategy. Currently we only define two, "none" and "regions".
 **/
class SERIF_EXPORTED DocumentSplitter {
public:
	// Create and return a new DocumentSplitter
	static DocumentSplitter* build();
	static DocumentSplitter* build(const char* split_strategy);

	// Hook for registering new factories
	struct Factory { 
		virtual ~Factory() {}
		virtual DocumentSplitter* build() = 0; 
	};		
	static void setFactory(const char* split_strategy, boost::shared_ptr<Factory> factory);
	static bool hasFactory(const char* split_strategy);

	virtual ~DocumentSplitter() {}

	// Entry point, expected to be called by a DocumentDriver. May return only a copy of the original Document.
	virtual std::vector<Document*> splitDocument(Document* document) = 0;
	virtual DocTheory* mergeDocTheories(std::vector<DocTheory*> splitDocTheories) = 0;

protected:
	DocumentSplitter() {}

private:
	typedef std::map<std::string, boost::shared_ptr<Factory> > FactoryMap; 
	static boost::shared_ptr<Factory>& _factory(const char* split_strategy);
	static FactoryMap& _factoryMap();
};

#endif

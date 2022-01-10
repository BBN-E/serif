// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ATTRIBUTE_VALUE_PAIR_EXTRACTOR_H
#define ATTRIBUTE_VALUE_PAIR_EXTRACTOR_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/Symbol.h"

#include <set>
#include <vector>
#include <map>

class DocTheory;
class LinkInfoCache;

class AttributeValuePairBase;
typedef boost::shared_ptr<AttributeValuePairBase> AttributeValuePair_ptr;

/**
  *  Base class for objects that extract AttributeValuePairs from data structures.
  */
class AttributeValuePairExtractorBase : boost::noncopyable {
public:
	virtual ~AttributeValuePairExtractorBase() {};
		
	Symbol getName() const { return _name; }
	Symbol getFullName() const;

	/** Store a pointer to extractor in registry, indexed by full extractor name. */
	static void registerExtractor(Symbol sourceType, AttributeValuePairExtractorBase* extractor);

	/** Return a pointer to the AttributeValuePairExtractor with this name and sourceType. */
	static const AttributeValuePairExtractorBase* getExtractor(Symbol sourceType, Symbol name);

	/** Return a pointer to the AttributeValuePairExtractor with this full name. */
	static const AttributeValuePairExtractorBase* getExtractor(Symbol fullName);

protected:
	AttributeValuePairExtractorBase(Symbol sourceType, Symbol name);
	virtual void validateRequiredParameters() {};

	Symbol _name;
	Symbol _sourceType;	

	// extractor registry
	typedef Symbol::HashMap<AttributeValuePairExtractorBase*> AttributeValuePairExtractorMap;
	static AttributeValuePairExtractorMap _featureExtractors;
	
	/** Return the full name Symbol for an extractor with name and sourceType. */ 
	static Symbol getFullName(Symbol sourceType, Symbol name);

private:
	AttributeValuePairExtractorBase() {}
};

/**
  *  Base class for objects that extract AttributeValuePairs from SourceType data 
  *  structures.
  *
  *  @tparam SourceType The data structure type from which to extract info.
  */
template <class SourceType>
class AttributeValuePairExtractor : public AttributeValuePairExtractorBase {
public:
	typedef boost::shared_ptr< AttributeValuePairExtractor<SourceType> > ptr_type;

	virtual ~AttributeValuePairExtractor() {};

	/** Return a vector of AttributeValuePairs extracted from context. */
	virtual std::vector< AttributeValuePair_ptr > extractFeatures(const SourceType& context, LinkInfoCache& cache, const DocTheory *docTheory) = 0;	

	virtual void resetForNewDocument(const DocTheory *docTheory) {};

protected:
	// Protected constructor: use AttributeValuePairExtractorBase::getExtractor() to access extractor instances.  
	AttributeValuePairExtractor(Symbol sourceType, Symbol name) : AttributeValuePairExtractorBase(sourceType, name) {};

private:
	AttributeValuePairExtractor() {}
};

#endif


// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_FEATURE_H
#define D_T_FEATURE_H

#include <string>

#include "Generic/common/hash_map.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/discTagger/PWeight.h"
#include "Generic/discTagger/DTFeatureType.h"

class UTF8OutputStream;
class BlockFeatureTable;

#define ALLOCATION_POOLING // turn on pooling (for subclasses)


/** DTFeature is an abstract class. Instances of its subclasses represent
  * features extracted from a DTState.
  */

class DTFeature {
public:
	DTFeature(const DTFeatureType *featureType) : _featureType(featureType) {}
	virtual ~DTFeature() {}

	/** This is the right way to remove a feature because subclasses may
	  * override operator delete() */
	// It seems like this was needed because the destructor wasn't declared
	// virtual.  Since now its destructor is virtual, we can get rid of this
	// method, and call delete directly because, in all of its derived classes,
	// deallocate() simply just does "delete this;".
	virtual void deallocate() { delete this; }

	/** This function returns a Symbol that represents the "future" part
	  * of this feature, necessary for use with the maxent model. If a 
	  * feature is being designed for use with another type of model,
	  * it's OK if it returns something meaningless. */	  
	virtual Symbol getTag() const = 0;

	/* The main use of DTFeature is as a hashtable key, so we define
	   equality and a hash function: */
	virtual bool operator==(const DTFeature &other) const = 0;
	virtual size_t getHashCode() const = 0;

	virtual bool equalsWithoutTag(const DTFeature &other) const = 0;
	virtual size_t getHashCodeWithoutTag() const = 0;

	virtual void toString(std::wstring &str) const = 0;

	virtual void toStringWithoutTag(std::wstring &str) const = 0;

	/// Write the contents to a file in a form that will be used by
	/// read() to re-instantiate the feature.
	virtual void write(UTF8OutputStream &out) const = 0;

	/// Re-instantiate the feature based on the output of write()
	virtual void read(UTF8InputStream &in) = 0;

	const DTFeatureType *getFeatureType() const { return _featureType; }


	// define hash_map mapping DTFeatures to floats
	struct FeatureHash {
        size_t operator()(const DTFeature *feature) const {
            return feature->getHashCode();
        }
    };
    struct FeatureEquality {
        bool operator()(const DTFeature *f1, const DTFeature *f2) const {
            return *f1 == *f2;
        }
    };
	typedef serif::hash_map<DTFeature*, PWeight, FeatureHash, FeatureEquality>
		FeatureWeightMap;

	static void readWeights(DTFeature::FeatureWeightMap &weightMap,
							const char *model_file,Symbol modelprefix);
	static void readWeights(BlockFeatureTable &weightTable,
							const char *model_file,Symbol modelprefix);
	static void writeWeights(DTFeature::FeatureWeightMap &weightMap,
							UTF8OutputStream& out,
							bool write_zero_weights = false);
	static void writeSumWeights(DTFeature::FeatureWeightMap &weightMap,
							UTF8OutputStream& out,
							bool write_zero_weights = false);
	static void writeLazySumWeights(DTFeature::FeatureWeightMap &weightMap,
							UTF8OutputStream& out, long n_hypotheses,
							bool write_zero_weights = false);

	static void checkParam(const char *model_file, Symbol paramName, Symbol paramValue);
	static void recordParamForReference(Symbol key, UTF8OutputStream& out);
	static void recordParamForConsistency(Symbol key, UTF8OutputStream& out);
	static void recordFileListForReference(Symbol key, UTF8OutputStream& out);


	// these two function are safer since there won't be any conversion from wchars to chars
	// and back again
	static void recordParamForReference(const char *key, UTF8OutputStream& out);
	static void recordParamForConsistency(const char *key, UTF8OutputStream& out);

	// write the time and date into the stream
	static void recordDate(UTF8OutputStream& out);

	static void addWeightsToSum(FeatureWeightMap *weights);

protected:
	/** Every DTFeature must have a DTFeatureType. This is a pointer
	* to a DTFeatureType instance in memory, of which there is exactly
	* one per feature type. */
	// This is protected (as opposed to private) only so that subclasses
	// can use the pointer for the totally unrelated purpose of allocation
	// pooling.
	const DTFeatureType *_featureType;
};

#endif

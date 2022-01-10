// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_FEATURE_TYPE_SET
#define D_T_FEATURE_TYPE_SET

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Symbol.h"

class DTFeatureType;


class DTFeatureTypeSet {
public:
	DTFeatureTypeSet(int n_feature_types);
	DTFeatureTypeSet(const char *features_file, Symbol modeltype);
	DTFeatureTypeSet(const char *features_file, Symbol modeltype, DTFeatureTypeSet *superset);
	~DTFeatureTypeSet();

	int getNFeaturesTypes() const { return _n_feature_types; }
        inline const DTFeatureType *getFeatureType(int i) const {
          if (_n_feature_types != _array_size) 
            throw InternalInconsistencyException("DTFeatureTypeSet::getFeatureType()",
                                                 "Instantiator of this DTFeatureTypeSet object added fewer feature\n"
                                                 "types than expected.");
          if ((unsigned) i < (unsigned) _n_feature_types) 
            return _featureTypes[i];
          else 
            throw InternalInconsistencyException::arrayIndexException("DTFeatureTypeSet::getFeatureType()", _n_feature_types, i);
        }
	bool addFeatureType(Symbol model, Symbol name);
private:
	bool addDictionaryFeaturetype(Symbol model, Symbol dict_name, Symbol dict_filename, bool is_lowercase_list, bool use_context, bool do_hybrid, int offset);
	
	// List feature that is used in a maximum-matching fasion
	bool addMaxMatchingListFeaturetype(Symbol model, Symbol dict_name, Symbol dict_filename);

	bool addListContextFeatureType(Symbol model, Symbol dict_name, Symbol list, bool is_lowercase);

	void readFeatures(const char *features_file, Symbol modeltype, DTFeatureTypeSet *superset);
	int _n_feature_types;
	int _array_size;
	DTFeatureType **_featureTypes;
};

#endif

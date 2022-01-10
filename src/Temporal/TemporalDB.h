#ifndef _TEMPORAL_DB_H_
#define _TEMPORAL_DB_H_

#include <vector>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"

BSP_DECLARE(TemporalDB)
BSP_DECLARE(FeatureMap)
BSP_DECLARE(DatabaseConnection)
BSP_DECLARE(TemporalTypeTable)
BSP_DECLARE(InstanceAnnotationDBView)
BSP_DECLARE(MultiAlphabet)
BSP_DECLARE(TemporalFeature)
class Symbol;

class TemporalDB {
public:
	FeatureMap_ptr createFeatureMap();
	std::vector<TemporalFeature_ptr> features(double threshold = 0.0) const;
	TemporalTypeTable_ptr makeTypeTable();
	void syncFeatures(const FeatureMap& featureMap);
	InstanceAnnotationDBView_ptr instanceAnnotationView();
	MultiAlphabet_ptr featureAlphabet();
private:
	TemporalDB(const std::string& filename);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(TemporalDB, const std::string&);

	int _nAlphabets;

	void initialize_db();
	void createOrCheckFeatureAlphabets(int numAlphabets);
	void createOrCheckAttributesTable(const std::vector<Symbol>& attributes);
	
	void checkFeatureMapIsConsistentWithDBContents(const FeatureMap& featureMap);
	int maxIdxForAlphabet(int alphabet) const;
	int sizeOfAlphabet(int alphabet);
	void assertDense(int alphabet);

	DatabaseConnection_ptr _db;
	bool _cached_size;
	int _size;
};

#endif

#ifndef _FEATURE_ANNOTATION_DB_H_
#define _FEATURE_ANNOTATION_DB_H_

#include <vector>
#include "Generic/common/bsp_declare.h"
#include "database/SqliteDBConnection.h"

BSP_DECLARE(FeatureAnnotationDBView)
class AnnotatedFeatureRecord;

class FeatureAnnotationDBView {
public:
	FeatureAnnotationDBView(DatabaseConnection_ptr db);
	virtual std::vector<AnnotatedFeatureRecord> getFeatureAnnotations() const;
	virtual void recordAnnotations(
		const std::vector<AnnotatedFeatureRecord>& annotations);
private:
	DatabaseConnection_ptr _db;
};

#endif

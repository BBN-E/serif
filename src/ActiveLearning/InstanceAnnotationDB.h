#ifndef _INSTANCE_ANNOTATION_DB_H_
#define _INSTANCE_ANNOTATION_DB_H_

#include <vector>
#include "Generic/common/bsp_declare.h"
#include "database/SqliteDBConnection.h"

BSP_DECLARE(InstanceAnnotationDBView)
class AnnotatedInstanceRecord;

class InstanceAnnotationDBView {
public:
	InstanceAnnotationDBView(DatabaseConnection_ptr db);
	virtual std::vector<AnnotatedInstanceRecord> getInstanceAnnotations() const;
	virtual void recordAnnotations(
		const std::vector<AnnotatedInstanceRecord>& annotations);
private:
	mutable DatabaseConnection_ptr _db;

	static size_t makeSizeT(unsigned int highBits, unsigned int lowBits);
	static std::pair<size_t, size_t> split64Bit(size_t big_hash);

	void translateOldStyleInstanceAnnotations() const;
	void createSplitAnnotationTable() const;

};

#endif

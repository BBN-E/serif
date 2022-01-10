#include "Generic/common/leak_detection.h"
#include "ActiveLearning/InstanceAnnotationDB.h"
#include "LearnIt2DB.h"

LearnIt2DB::LearnIt2DB(const std::string& db_location, bool readonly) :
LearnItDB(db_location, readonly) {}

InstanceAnnotationDBView_ptr LearnIt2DB::instanceAnnotationView() {
	InstanceAnnotationDBView_ptr ret(_new InstanceAnnotationDBView(_db));
	return ret;
}

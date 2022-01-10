#ifndef _LEARNIT2_DB_H_
#define _LEARNIT2_DB_H_

#include <string>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "LearnIt/db/LearnItDB.h"

BSP_DECLARE(LearnIt2DB)
BSP_DECLARE(InstanceAnnotationDBView)

class LearnIt2DB : public LearnItDB {
public:
	InstanceAnnotationDBView_ptr instanceAnnotationView();
protected:
	LearnIt2DB(const std::string& db_location, bool readonly=true);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(LearnIt2DB, const std::string&);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(LearnIt2DB, const std::string&, bool);
};

#endif


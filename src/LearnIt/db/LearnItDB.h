// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef LEARNIT_DB_H
#define LEARNIT_DB_H

#include <vector>
#include <string>
#include "boost/noncopyable.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
//#include "LearnIt/Eigen/Core"
#include "Generic/database/DatabaseConnection.h"
BSP_DECLARE(LearnItPattern)  // Defined in "learnit/Pattern.h"

BSP_DECLARE(Target);   // Defined in "learnit/Target.h"
BSP_DECLARE(Seed);     // Defined in "learnit/Seed.h"
BSP_DECLARE(Instance); // Defined in "learnit/Instance.h"
BSP_DECLARE(Feature);  // Defined in "learnit/Feature.h"
BSP_DECLARE(SentenceMatchableFeature)
BSP_DECLARE(MatchMatchableFeature)
BSP_DECLARE(SlotConstraints);
BSP_DECLARE(SlotPairConstraints);
BSP_DECLARE(AnnotatedFeatureRecord);
BSP_DECLARE(LearnItDB)
BSP_DECLARE(SqliteDBConnection)
BSP_DECLARE(FromDBFeatureAlphabet)

/** A simple data access layer for the LearnIt database.
  *
  * This class intentionally provides fairly minimal functionality.  In 
  * particular, it can only be used to read the database; and it does not 
  * read all fields from all tables.
  */
class LearnItDB: boost::noncopyable, public boost::enable_shared_from_this<LearnItDB> {
public:
	/** Return a vector of all targets in the database. */
	std::vector<Target_ptr > getTargets(const std::wstring& constraints=L"") const;

	/** Return a vector of all patterns in the database. */
	std::vector<LearnItPattern_ptr >getPatterns(bool throw_out_rejected) const;

	/** Return a vector of all seeds in the database. */
	std::vector<boost::shared_ptr<Seed> > getSeeds(bool active_only = false) const;

	/** Old databases don't have features tables. */
	bool hasFeatures() const;

	/* old databases don't have instances tables. */
	bool hasInstances() const;
	/** Return a vector of all instance in the database. */
	std::vector<Instance_ptr> getInstances() const;

	bool containsEquivalentNameRow(std::wstring name, std::wstring equivalent_name) const;
	bool containsEquivalentNamesForName(std::wstring name) const;
	void insertEquivalentNameRow(std::wstring name, std::wstring equivalent_name, std::wstring score);

	/** Return the target with the given name from the database.
	  * If no such target is found, throw a std::runtime_error
	  * exception. */
	Target_ptr getTarget(const std::wstring& targetName) const;

	double getPrior() const;
	double probabilityThreshold() const;

	bool readOnly() const;

	boost::shared_ptr<FromDBFeatureAlphabet> getAlphabet(unsigned int alphabetIndex);
protected:
	/** Open the database in the file named "db_location". */
	LearnItDB(const std::string& db_location, bool readonly=true);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(LearnItDB, const std::string&);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(LearnItDB, const std::string&, bool);

	mutable DatabaseConnection_ptr _db;
private:
	
	static const int MAX_NUM_SLOTS = 6;

        std::vector<SlotConstraints_ptr> _getSlotConstraints(
            const std::wstring& target_name, int nslots) const;
        std::vector<SlotPairConstraints_ptr> _getSlotPairConstraints(
            const std::wstring& target_name) const;

	// things to support FeatureAlphabets
	friend class FromDBFeatureAlphabet;
	DatabaseConnection::Table_ptr execute(const std::wstring& sql);
	void beginTransaction();
	void endTransaction();
};


#endif

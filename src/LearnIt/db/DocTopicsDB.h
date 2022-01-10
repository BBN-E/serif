#include <string>
#include "Generic/database/SqliteDBConnection.h"
#include "boost/noncopyable.hpp"
#include "Generic/common/bsp_declare.h"
BSP_DECLARE(DocTopicsDB)

/** A really simple data access layer for the topics database. */
class DocTopicsDB: boost::noncopyable {
public:
	/** Open the database in the file named "db_location". */
	DocTopicsDB(const std::string& db_location, bool readonly=true): _db(db_location, readonly) {}

	/** Return the topics given docid. */
	std::vector<std::wstring> getAllTopics(const std::string& full_docid) const;
	std::vector<std::wstring> getAllWeights(const std::string& full_docid) const;
	std::vector<std::pair<int, std::wstring> > getTopicInfo(const std::string& full_docid) const;
	std::wstring getTopicInfoString(const std::string& full_docid) const;
private:
	mutable SqliteDBConnection _db;
};

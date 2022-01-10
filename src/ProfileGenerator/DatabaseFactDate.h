// DatabaseFactDate.h
// A derived class of FactDate, for storing dates that came out of the database.
// (Dates may also be created by ProfileGenerator)

#ifndef DATABASE_FACT_DATE_H
#define DATABASE_FACT_DATE_H

#include "ProfileGenerator/PGFactDate.h"

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(DatabaseFactDate);

class DatabaseFactDate : public PGFactDate
{
public:
	friend DatabaseFactDate_ptr boost::make_shared<DatabaseFactDate>(int const&, PGFact_ptr const&, std::string const&, std::wstring const&, std::wstring const&);

private:
	DatabaseFactDate(int date_id, PGFact_ptr fact, std::string date_type_str,
		std::wstring literal_string_value, std::wstring resolved_string_value);

	int _date_id; // primary key	
	std::wstring _literal_string_val;
	std::wstring _resolved_string_val;
};

#endif

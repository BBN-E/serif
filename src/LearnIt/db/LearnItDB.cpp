#include "Generic/common/leak_detection.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <sstream>
#include "Generic/common/ParamReader.h"
#include "Generic/sqlite/sqlite3.h"
#include "Generic/database/SqliteDBConnection.h"
#include "boost/foreach.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UnexpectedInputException.h"
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "ActiveLearning/alphabet/FromDBFeatureAlphabet.h"
#include "LearnIt/LearnItPattern.h"
#include "LearnIt/Seed.h"
#include "LearnIt/Target.h"
#include "LearnIt/Instance.h"
#include "LearnIt/features/Feature.h"
#include "LearnIt/SlotConstraints.h"
#include "LearnIt/SlotPairConstraints.h"
//#include "LearnIt/Eigen/Core"
#include "LearnItDB.h"

using boost::make_shared;
using boost::dynamic_pointer_cast;
using boost::lexical_cast;
using std::remove_if;
using std::cout; using std::endl;
//using namespace Eigen;

typedef std::vector<std::wstring>::const_iterator strings_iter;

LearnItDB::LearnItDB(const std::string& db_location, bool readonly)
: _db(make_shared<SqliteDBConnection>(db_location, readonly))
{}

std::vector<Target_ptr> LearnItDB::getTargets(const std::wstring& constraints) const {
	Targets targets;

	DatabaseConnection::Table_ptr targetTable = _db->exec(L"select name, nslots, elf_ontology_type from targetinfo"+constraints);
	BOOST_FOREACH(std::vector<std::wstring> row, *targetTable) {
		std::wstring target_name = row[0];
		int nslots = boost::lexical_cast<int>(row[1]);
		std::wstring elf_ontology_type = row[2];

		std::vector<std::vector<int> > sufficientSlots;
		DatabaseConnection::Table_ptr targetSufficientSlotTable = _db->exec(L"select bitmask from targetsufficientslotinfo "
			L"where target='" + boost::lexical_cast<std::wstring>(target_name) + L"'");
		BOOST_FOREACH(std::vector<std::wstring> slotRow, *targetSufficientSlotTable) {
			int bitmask = boost::lexical_cast<int>(slotRow[0]);
			std::vector<int> slotSet;
			for (int slot_num=0; slot_num < nslots; ++slot_num) {
				if (bitmask & (1<<slot_num)) {
					slotSet.push_back(slot_num);
				}
			}
			sufficientSlots.push_back(slotSet);
		}

		std::vector<SlotConstraints_ptr> slot_constraints = 
			_getSlotConstraints(target_name, nslots);
		if (slot_constraints.size() != (size_t)nslots) {
                        throw UnexpectedInputException("LearnItDB::getTargets",
                            ("Expected " + boost::lexical_cast<std::string>(nslots) + 
                             " constraints, found " + boost::lexical_cast<std::string>(slot_constraints.size())).c_str());
		}

		std::vector<SlotPairConstraints_ptr> slot_pair_constraints =
			_getSlotPairConstraints(target_name);

		targets.push_back(boost::make_shared<Target>(target_name,
                                    slot_constraints, slot_pair_constraints,
                                    elf_ontology_type, sufficientSlots));
	}
	return targets;
}

std::vector<SlotConstraints_ptr>
LearnItDB::_getSlotConstraints(const std::wstring& target_name, int nslots) const {
    std::vector<SlotConstraints_ptr> slot_constraints;
    for (int slot_num=0; slot_num < nslots; ++slot_num) {
        DatabaseConnection::Table_ptr targetSlotTable =_db->exec(
            L"select type, mention_constraints, seed_type, "
            L"elf_ontology_type, elf_role, use_best_name, allow_desc_training "
            L"from targetslotinfo "
            L"where target='" + boost::lexical_cast<std::wstring>(target_name) + 
            L"' AND slotnum=" + boost::lexical_cast<std::wstring>(slot_num));

        if (targetSlotTable->size() == 0) {
            throw UnexpectedInputException("LearnItDB::_getSlotConstraints", 
                ("No slot " + boost::lexical_cast<std::string>(slot_num) +
                 " for target " + UnicodeUtil::toUTF8StdString(target_name)).c_str());
        } else if (targetSlotTable->size() > 1) {
            throw UnexpectedInputException("LearnItDB_getSlotConstraints", ("Multiple slot "
                + boost::lexical_cast<std::string>(slot_num) + "s for target "
                + UnicodeUtil::toUTF8StdString(target_name)).c_str());
        }

        BOOST_FOREACH(std::vector<std::wstring> slotRow, *targetSlotTable) {
            std::wstring slot_type = slotRow[0];
            std::wstring slot_mention_constraints = slotRow[1];
            std::wstring slot_seed_type = slotRow[2];
            std::wstring slot_elf_ontology_type = slotRow[3];
            std::wstring slot_elf_role = slotRow[4];
            bool use_best_name = boost::lexical_cast<bool>(slotRow[5]);
			bool allow_desc_training = boost::lexical_cast<bool>(slotRow[6]);
            slot_constraints.push_back(boost::make_shared<SlotConstraints>(
                slot_type, slot_mention_constraints, slot_seed_type, 
                slot_elf_ontology_type, slot_elf_role, use_best_name,
				allow_desc_training));
        }
    }
    return slot_constraints;
}

std::vector<SlotPairConstraints_ptr> 
LearnItDB::_getSlotPairConstraints(const std::wstring& target_name) const
{
    std::vector<SlotPairConstraints_ptr> slot_pair_constraints;
    DatabaseConnection::Table_ptr targetSlotPairTable =_db->exec(L"select A, B, "
            L"must_corefer, must_not_corefer from targetslotpairinfo where target='"
			+ boost::lexical_cast<std::wstring>(target_name) + L"'");

    typedef std::vector<std::wstring> StringVector;
    BOOST_FOREACH(StringVector slotRow, *targetSlotPairTable)
    {
        int slot_a = boost::lexical_cast<int>(slotRow[0]);
        int slot_b = boost::lexical_cast<int>(slotRow[1]);
        bool must_corefer = boost::lexical_cast<bool>(slotRow[2]);
		bool must_not_corefer = boost::lexical_cast<bool>(slotRow[3]);

        slot_pair_constraints.push_back(
            boost::make_shared<SlotPairConstraints>(slot_a, slot_b,
                must_corefer, must_not_corefer));
    }

    return slot_pair_constraints;
}

std::vector<LearnItPattern_ptr> LearnItDB::getPatterns(bool throw_out_rejected) const {
	std::vector<LearnItPattern_ptr> patterns;
	bool unswitched = ParamReader::isParamTrue("unswitched_pattern_and_name");
	bool prefilter = ParamReader::isParamTrue("keyword_prefiltering");
	DatabaseConnection::Table_ptr table;
	if (prefilter) {
		table =_db->exec(L"select target, name, pattern, language, active, rejected, precision, recall, keywords from patterns");
	} else {
		table =_db->exec(L"select target, name, pattern, language, active, rejected, precision, recall from patterns");
	}
	BOOST_FOREACH(std::vector<std::wstring> row, *table) {
		std::wstring target = row[0];
		std::wstring name;
		std::wstring pattern;
		if (unswitched) {
			name = row[1];
			pattern = row[2];
		} else {
			pattern = row[1]; //switch!
			name = row[2];
		}
		LanguageVariant_ptr language = LanguageVariant::getLanguageVariant(Symbol(row[3]),Symbol(L"source"));
		std::wstring active = row[4];
		std::wstring rejected = row[5];
		std::wstring precision = row[6];
		std::wstring recall = row[7];
		std::wstring keywords = L"";
		if (prefilter) {
			keywords = row[8];
		}
		if (precision == L"") precision = L"-1"; //unknown
		if (recall == L"") recall = L"-1";
		bool b_rejected = boost::lexical_cast<bool>(rejected);
		if (b_rejected && throw_out_rejected) {
			continue;
		}
		patterns.push_back(LearnItPattern::create(getTarget(target), name, pattern,
			                               boost::lexical_cast<bool>(active),
										   b_rejected,
										   boost::lexical_cast<float>(precision),
										   boost::lexical_cast<float>(recall),
										   true,
										   language,
										   keywords));
	}
	return patterns;
}

std::vector<Seed_ptr> LearnItDB::getSeeds(bool active_only) const {

	std::vector<Seed_ptr> seeds;
	for (int num_slots=1; num_slots < MAX_NUM_SLOTS; ++num_slots) {
		std::wstringstream table_name;
			table_name << "seeds" << num_slots;
		
		if (_db->tableExists(table_name.str())) {
		  std::wstringstream sql_command;
		  sql_command << L"select target, active, source, language";
		  for (int slot_num=0; slot_num < num_slots; slot_num++) {
			sql_command << L", slot" << slot_num;
		  }
		  sql_command << L" from seeds" << num_slots;

		  DatabaseConnection::Table_ptr table =_db->exec(sql_command.str());
		  BOOST_FOREACH(std::vector<std::wstring> row, *table) {
			std::vector<std::wstring>::iterator iter = row.begin();
			std::wstring target = *iter++;
			bool active = boost::lexical_cast<bool>(*iter++);
			std::wstring source = *iter++;
			std::wstring language = *iter++;
			// What happens here for empty optional slots??  Will we be dereferencing NULL?? [XX]
			std::vector<std::wstring> slots(iter, row.end());
			if (active || !active_only) {
			  seeds.push_back(
				make_shared<Seed>(getTarget(target), slots, active, source, language));
			}
		  }
		}
	}

	return seeds;
}

bool LearnItDB::hasInstances() const {
	for (int num_slots = 1; num_slots < MAX_NUM_SLOTS; ++num_slots) {
		std::wstringstream table_name;
		table_name << "instance" << num_slots;
		if (_db->tableExists(table_name.str())) {
			return true;
		}
	}
	return false;
}

std::vector<Instance_ptr> LearnItDB::getInstances() const {
	std::vector<Instance_ptr> instances;

	if (hasInstances()) {
		for (int num_slots = 1; num_slots < MAX_NUM_SLOTS; ++num_slots) {
			std::wstringstream sql_command;
			sql_command << L"select target, iteration, score, docid, sentence, source";
			for (int slot_num = 0; slot_num < num_slots; ++slot_num) {
				sql_command << L", slot" << slot_num;
			}
			sql_command << L" from instance" << num_slots;

			DatabaseConnection::Table_ptr table =_db->exec(sql_command.str());
			BOOST_FOREACH(std::vector<std::wstring>& row, *table) {
				std::vector<std::wstring>::iterator iter = row.begin();
				std::wstring target = *iter++;
				int iteration = boost::lexical_cast<int>(*iter++);
				float score = boost::lexical_cast<float>(*iter++);
				std::wstring docid = *iter++;
				int sentence = boost::lexical_cast<int>(*iter++);
				std::wstring source = *iter++;
				std::vector<std::wstring> slots(iter, row.end());
				instances.push_back(boost::make_shared<Instance>(getTarget(target), 
					slots, iteration, score, docid, sentence, source));
			}
		}
	}

	return instances;
}

Target_ptr LearnItDB::getTarget(const std::wstring& targetName) const {
	std::vector<Target_ptr> targets = getTargets(L" where name='"+targetName+L"'");
	if (targets.size() != 1)
		throw std::runtime_error("Target '"+UnicodeUtil::toUTF8StdString(targetName)+"' not found");
	return targets[0];
}

bool LearnItDB::containsEquivalentNameRow(std::wstring name, std::wstring equivalent_name) const {
	std::wstring sql = L"select count(*) from equivalent_names where name='" +_db->sanitize(name) 
		+ L"' and equiv='" +_db->sanitize(equivalent_name) + L"'";
	DatabaseConnection::Table_ptr table =_db->exec(sql);
	return (*table)[0][0] != L"0";
}

bool LearnItDB::containsEquivalentNamesForName(std::wstring name) const {
	std::wstring sql = L"select count(*) from equivalent_names where name='" +_db->sanitize(name) + L"'";
	DatabaseConnection::Table_ptr table =_db->exec(sql);
	return (*table)[0][0] != L"0";
}

void LearnItDB::insertEquivalentNameRow(std::wstring name, std::wstring equivalent_name, std::wstring score) {
	//SessionLogger::info("LEARNIT") << "Name: " << name << " equiv: " << equivalent_name << " score: " << score << "\n";
	if(containsEquivalentNameRow(name, equivalent_name)){
		return;
	}
	std::wstringstream sql;
	sql << L"insert into equivalent_names (name,equiv,score) values('"; 
	sql <<_db->sanitize(name) << L"', '";
	sql <<_db->sanitize(equivalent_name) << L"', '";
	sql <<_db->sanitize(score) << L"'";
	sql << L")";
	_db->exec(sql.str());
}


double LearnItDB::getPrior() const {
	std::wstringstream sql;
	sql << L"select value from stagetostagenumericdata where name = 'good_seed_prior'";
	DatabaseConnection::Table_ptr tbl =_db->exec(sql.str());

	if (tbl->empty()) {
		throw UnexpectedInputException("LearnItDB::getPrior()",
			"Prior not recorded in DB");
	}

	return boost::lexical_cast<double>((*tbl)[0][0]);
}

bool LearnItDB::readOnly() const {
	return _db->readOnly();
}

DatabaseConnection::Table_ptr LearnItDB::execute(const std::wstring& sql) {
	return _db->exec(sql);
}

void LearnItDB::beginTransaction() {
	_db->beginTransaction();
}

void LearnItDB::endTransaction() {
	_db->endTransaction();
}

FromDBFeatureAlphabet_ptr LearnItDB::getAlphabet(unsigned int alphabetIndex) {
	if (_db->tableExists(L"features")) {
		return make_shared<FromDBFeatureAlphabet>(_db, alphabetIndex);
	} else {
		throw UnexpectedInputException("LearnItDB::getAlphabet",
			"Database does not contain a features table");
	}
}

double LearnItDB::probabilityThreshold() const {
	if (_db->tableExists(L"probability_threshold")) {
		DatabaseConnection::Table_ptr table = _db->exec(L"select * from probability_threshold");

		if (table->size() != 1) {
			throw UnexpectedInputException("LearnItDB::probabilityThreshold",
					"Corrupt probability threshold table");
		}

		DatabaseConnection::TableRow row = (*table)[0];

		if (row.size() != 1) {
			throw UnexpectedInputException("LearnItDB::probabilityThreshold",
					"Corrupt probability threshold table");
		}

		return boost::lexical_cast<double>(row[0]);
	}

	throw UnexpectedInputException("LearnItDB::probabilityThreshold",
			"Database is missing probability threshold table");
}

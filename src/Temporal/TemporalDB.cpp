#include "Generic/common/leak_detection.h"
#include "TemporalDB.h"

#include <vector>
#include <sstream>
#include <map>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "database/SqliteDBConnection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/foreach_pair.hpp"
#include "ActiveLearning/InstanceAnnotationDB.h"
#include "ActiveLearning/alphabet/FromDBFeatureAlphabet.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"
#include "FeatureMap.h"
#include "TemporalTypeTable.h"
#include "Temporal/features/TemporalFeature.h"
#include "Temporal/features/TemporalFeatureFactory.h"

using std::pair;
using std::string; using std::wstring;
using std::vector;
using std::wstringstream;
using boost::make_shared;
using boost::lexical_cast;

TemporalDB::TemporalDB(const string& filename) 
: _db(make_shared<SqliteDBConnection>(filename, false, true)), _nAlphabets(0),
_cached_size(false), _size(0)
{
	initialize_db();
}

void TemporalDB::initialize_db() {
	vector<Symbol> attributes = 
		ParamReader::getSymbolVectorParam("temporal_attributes");
	_nAlphabets = static_cast<int>(attributes.size());

	if (!attributes.size()) {
		throw UnexpectedInputException("TemporalDB::initialize_db",
			"No temporal attributes listed in param file");
	}

	createOrCheckAttributesTable(attributes);
	createOrCheckFeatureAlphabets(static_cast<int>(attributes.size()));
}

void TemporalDB::createOrCheckAttributesTable(const vector<Symbol>& attributes) {
	_db->exec(L"create table if not exists attributes (name VARCHAR, idx INTEGER, "
		L"PRIMARY KEY(idx), UNIQUE(name))");
	DatabaseConnection::Table_ptr tbl = _db->exec(L"select name from attributes");

	if (tbl->size() > 0) {
		bool good = true;
		std::vector<wstring> storedAttributes;
		
		if (tbl->size() == attributes.size()) {
			BOOST_FOREACH(const vector<wstring> row, *tbl) {
				storedAttributes.push_back(row[0]);
			}

			for (size_t i=0; i<attributes.size(); ++i) {
				if (storedAttributes[i] != attributes[i].to_string()) {
					good = false;
					break;
				}
			}
		}

		if (!good) {
			wstringstream err;
			err << L"Attributes in database and parameter file do not match.\n" 
				<< L"\tParam file has: " << std::endl;
			BOOST_FOREACH(const Symbol& attribute, attributes) {
				err << L"\t\t" << attribute.to_string() << std::endl;
			}
			err << L"\tbut database has:" << std::endl;
			BOOST_FOREACH(const std::wstring& attribute, storedAttributes) {
				err << L"\t\t" << attribute << std::endl;
			}
		} else {
			SessionLogger::info("Attributes table matches what's expected.");
		}
	} else {
		SessionLogger::info("populate_db") << "Populating attributes table...";
		_db->beginTransaction();
		for (size_t i=0; i<attributes.size(); ++i) {
			wstringstream sql;
			sql << L"insert into attributes values (\"" << attributes[i].to_string() 
				<< "\", " << i << ")";
			_db->exec(sql.str());
		}
		_db->endTransaction();
	}
}

void TemporalDB::createOrCheckFeatureAlphabets(int numAlphabets) {
	_db->exec(L"create table if not exists features (name VARCHAR, weight REAL, "
		L"annotated INTEGER, positive INTEGER, expectation REAL, idx INTEGER, "
		L"alphabet INTEGER, metadata VARCHAR, regularization REAL, "
		L"prettyprint VARCHAR, PRIMARY KEY(idx, alphabet))");

	assertDense(0);
	int num = sizeOfAlphabet(0);
	for (int i = 1; i < numAlphabets; ++i) {
		assertDense(i);
		int sz = sizeOfAlphabet(i);

		if (sz != num) {
			wstringstream err;
			err << "Mismatched feature alphabet sizes. Alphabet 0 has " <<
				num << " entires, but alphabet " << i << " has " << sz << " entries.";
			throw UnexpectedInputException("TemporalDB::initializeDB", err);
		}
	}
}

void TemporalDB::assertDense(int alphabet) {
	int sz = sizeOfAlphabet(alphabet);
	bool dense = true;
	int maxIdx;

	maxIdx = maxIdxForAlphabet(alphabet);
	if (sz != (maxIdx+1)) {
		dense = false;
	}
	
	if (!dense) {
		wstringstream err;
		err << "Alphabet " << alphabet << " is not dense. It has a size of "
			<< sz;
		if (maxIdx >= 0) {
			err << " but a maximum index of " << maxIdx;
		} else {
			err << " but no maximum index could be found";
		}
		err << std::endl;
		throw UnexpectedInputException("TemporalDB::assertDone", err);
	}
}

void TemporalDB::checkFeatureMapIsConsistentWithDBContents(const FeatureMap& featureMap) {
	// skip this check for now
}

void TemporalDB::syncFeatures(const FeatureMap& featureMap) {
	_cached_size = false;
	checkFeatureMapIsConsistentWithDBContents(featureMap);
	
	int maxIdx = maxIdxForAlphabet(0);
	unsigned int insertedFeatures = 0;

	_db->beginTransaction();
	for (int alphabet = 0; alphabet < _nAlphabets; ++alphabet) {
		for (FeatureMap::left_const_iterator entry = featureMap.left_begin();
			entry!=featureMap.left_end(); ++entry) 
		{
			if ((int)entry->second > maxIdx || maxIdx == -1) {
				// if idx < maxIdx, it's already in the db
				/*
					_db->exec(L"create table if not exists features (name VARCHAR, weight REAL, "
		L"annotated INTEGER, positive INTEGER, expectation REAL, idx INTEGER, "
		L"alphabet INTEGER, metadata VARCHAR, PRIMARY KEY(idx, alphabet))");
				*/

				wstringstream sql;
				sql << L"insert into features values ("
					<< "''" // name (unused; see pretty)
					<< L", " << 0.0  // weight
					<< L", " << 0 // annotated
					<< L", " << 0 // positive
					<< L", " << 0.0 // expectation
					<< L", " << entry->second // idx
					<< L", " << alphabet // alphabet
					<< L", '" << SqliteDBConnection::sanitize(entry->first->metadata()) << L"'" // metadata
					<< L", 1.0, " // regularization,
					<< L"'" << SqliteDBConnection::sanitize(entry->first->pretty()) << L"'"
					<< L")";
				_db->exec(sql.str());
				++insertedFeatures;
			}
		}
	}
	_db->endTransaction();
	SessionLogger::info("temporal_inserted_features") << L"Inserted "
		<< insertedFeatures << L" features into temporal database.";
}

int TemporalDB::maxIdxForAlphabet(int alphabet) const {
	wstringstream sql;
	sql << "select max(idx) from features where alphabet = " << alphabet;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	if (tbl->size() && (*tbl)[0][0] != L"") {
		return lexical_cast<int>((*tbl)[0][0]);
	} else {
		return -1;
	}
}

int TemporalDB::sizeOfAlphabet(int alphabet) {
	if (!_cached_size) {
		wstringstream sql;
		sql << "select count(idx) from features where alphabet = " << alphabet;
		DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());
		_size = boost::lexical_cast<int>((*tbl)[0][0]);
	}
	return _size;
}

FeatureMap_ptr TemporalDB::createFeatureMap() {
	FeatureMap_ptr ret = make_shared<FeatureMap>();

	wstringstream sql;
	sql << L"select idx, metadata from features where alphabet = 0";
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	vector<pair<wstring, unsigned int> > data;
	BOOST_FOREACH(vector<wstring>& row, *tbl) {
		data.push_back(make_pair(row[1], lexical_cast<unsigned int>(row[0])));
	}
	ret->load(data);
	return ret;
}

std::vector<TemporalFeature_ptr> TemporalDB::features(double threshold) const {
	std::vector<TemporalFeature_ptr> ret;

	wstringstream sql;
	sql << L"select idx, alphabet, metadata, weight from features";
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());
	typedef std::map<unsigned int, TemporalFeature_ptr> FeatMap;
	FeatMap featMap;

	BOOST_FOREACH(vector<wstring>& row, *tbl) {
		unsigned int idx = lexical_cast<unsigned int>(row[0]);
		unsigned int alphabet = lexical_cast<unsigned int>(row[1]);
		double weight = lexical_cast<double>(row[3]);
		FeatMap::const_iterator probe = featMap.find(idx);

		if (probe == featMap.end()) {
			TemporalFeature_ptr feat = TemporalFeatureFactory::create(row[2]);
			feat->setWeight(alphabet, weight);
			featMap.insert(std::make_pair(idx, feat));
		} else {
			probe->second->setWeight(alphabet, weight);
		}
	}

	BOOST_FOREACH_PAIR(unsigned int idx, TemporalFeature_ptr feat, featMap) {
		if (feat->passesThreshold(threshold)) {
			ret.push_back(feat);
		}
	}

	return ret;
}

TemporalTypeTable_ptr TemporalDB::makeTypeTable() {
	wstringstream sql;
	sql << "select idx, name from attributes";
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	vector<pair<unsigned int, wstring> > data;
	BOOST_FOREACH(const vector<wstring>& row, *tbl) {
		data.push_back(make_pair(lexical_cast<unsigned int>(row[0]), row[1]));
	}

	return make_shared<TemporalTypeTable>(data);
}

InstanceAnnotationDBView_ptr TemporalDB::instanceAnnotationView() {
	return make_shared<InstanceAnnotationDBView>(_db);
}

MultiAlphabet_ptr TemporalDB::featureAlphabet() {
	TemporalTypeTable_ptr typeTable = makeTypeTable();
	std::vector<FeatureAlphabet_ptr> alphabets;

	BOOST_FOREACH(unsigned int alphaIdx, typeTable->ids()) {
		alphabets.push_back(make_shared<FromDBFeatureAlphabet>(_db, alphaIdx));
	}

	return make_shared<MultiAlphabet>(alphabets);
}

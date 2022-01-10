#include "Generic/common/leak_detection.h"
#include <string>
#include <sstream>
//#include "sqlite/sqlite3.h"
#include "boost/foreach.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/UnexpectedInputException.h"
#include "database/SqliteDBConnection.h"
#include "ActiveLearning/AnnotatedFeatureRecord.h"
#include "LearnIt/Eigen/Core"
#include "FromDBFeatureAlphabet.h"

/**********************************
 * Feature Alphabets
 **********************************/

using boost::lexical_cast;
using boost::make_shared;
using boost::dynamic_pointer_cast;
using namespace Eigen;

FromDBFeatureAlphabet::FromDBFeatureAlphabet(DatabaseConnection_ptr db, unsigned int alphabetIndex) 
: _db(db), _alphIdx(alphabetIndex) {}

int FromDBFeatureAlphabet::size() const {
	std::wstringstream sql;
	sql << L"select max(idx) from features where alphabet = " << _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());
	if (tbl->size() > 0 && (*tbl)[0][0]!=L"") {
		return boost::lexical_cast<int>((*tbl)[0][0])+1;
	} else {
		return 0;
	}
}

double FromDBFeatureAlphabet::firstFeatureWeightByName(const std::wstring& feature_name) const {
	return featureWeightByIndex(firstFeatureIndexByName(feature_name));
}

double FromDBFeatureAlphabet::featureWeightByIndex(int idx) const {
	std::wstringstream sql;
	sql << L"select weight from features where idx = " << idx 
		<< " and alphabet = " << _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	if (tbl->size() == 0) {
		std::wstringstream err;
		err << "No entry in alphabet " <<  _alphIdx << " for feature " << idx;
		throw UnexpectedInputException("FeatureAlphabet::featureWeightByIndex", err);
	} // cannot be > 1 because idex is the primary key

	return lexical_cast<double>((*tbl)[0][0]);
}

int FromDBFeatureAlphabet::firstFeatureIndexByName(const std::wstring& feature_name) const {
	std::wstringstream sql;
	sql << L"select idx from features where name = \"" << feature_name << "\""
		<< " and alphabet = " << _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	if (tbl->size() < 1) {
		std::wstringstream err;
		err << "No feature found with name \"" << feature_name << "\"" <<
			" in alphabet " << _alphIdx;
		throw UnexpectedInputException("FeatureAlphabet::featureIndexByName", err);
	}

	// there could be multiple results, but we only return the first
	return lexical_cast<int>((*tbl)[0][0]);
}

std::vector<AnnotatedFeatureRecord_ptr> FromDBFeatureAlphabet::annotations() const {
	std::vector<AnnotatedFeatureRecord_ptr> ret;
	std::wstringstream sql;
	sql << L"select idx, positive, expectation from features where annotated = 1"
		<< " and alphabet = " << _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());
	
	BOOST_FOREACH(std::vector<std::wstring>& row, *tbl) {
		ret.push_back(make_shared<AnnotatedFeatureRecord>(
			boost::lexical_cast<int>(row[0]),
			boost::lexical_cast<int>(row[1]) == 1,
			boost::lexical_cast<double>(row[2])));
	}

	return ret;
}

void FromDBFeatureAlphabet::recordAnnotations(
				const std::vector<AnnotatedFeatureRecord_ptr>& annotations)
{
	typedef std::pair<int, double> AnnotationEntry;
	
	_db->beginTransaction();
	BOOST_FOREACH(AnnotatedFeatureRecord_ptr ann, annotations) {
		std::wstringstream sql;
		sql << L"update features set annotated = 1, positive = " << 
			(ann->positive_annotation?1:0) << ", expectation = " 
			<< ann->expectation << " where idx = " << ann->idx
			<< " and alphabet = " << _alphIdx;
		_db->exec(sql.str());
	}
	_db->endTransaction();
}

Eigen::VectorXd FromDBFeatureAlphabet::getWeights() const {
	VectorXd ret(VectorXd::Zero(size()));
	std::wstringstream sql;
	sql << L"select idx, weight from features where alphabet = " << _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());
	BOOST_FOREACH(const std::vector<std::wstring>& feat_row, *tbl) {
		ret[lexical_cast<int>(feat_row[0])] =
			lexical_cast<double>(feat_row[1]);
	}
	return ret;
}



DatabaseConnection::Table_ptr FromDBFeatureAlphabet::getFeatureRows( double threshold, 
		bool include_negatives, const std::wstring& constraints) const 
{
	std::wstringstream sql_command;
	sql_command << "select idx, name, metadata, weight from features where ";
	if (include_negatives) {
		sql_command << "abs(weight)";
	} else {
		sql_command << "weight";
	}
	sql_command << " >= " << threshold;

	if (!constraints.empty()) {
		sql_command << " and " << constraints;
	}

	sql_command << " and alphabet = " << _alphIdx;

	return _db->exec(sql_command.str());
}


std::vector<int> FromDBFeatureAlphabet::featureIndicesOfClass(const std::wstring& feature_class_name,
		bool use_metadata_field) const {
	// faster to just load the whole table and do the processing
	// than to do the select over NFS
	const std::wstring test_string = feature_class_name
		+ (use_metadata_field?L"":L"(");

	std::wstringstream sql;
	sql << L"select idx, " << (use_metadata_field?L"metadata":L"name")
		<< L" from features where alphabet = " << _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	std::vector<int> ret;
	BOOST_FOREACH(const std::vector<std::wstring>& feat_row, *tbl) {
		if (feat_row[1].find(test_string) == 0) {
			ret.push_back(boost::lexical_cast<int>(feat_row[0]));
		}
	}

	/*std::wstringstream sql;
	sql << L"select idx from features where " 
	    << (use_metadata_field?L"metadata":L"name") 	
		 << L" like \"" << feature_class_name 
		<< ((use_metadata_field)?L"\t":L"(")
		<< "%\" and alphabet = " << _alphIdx;
	Table_ptr tbl = _db->exec(sql.str());

	std::vector<int> ret;
	BOOST_FOREACH(const std::vector<std::wstring>& feat_row, *tbl) {
		ret.push_back(boost::lexical_cast<int>(feat_row[0]));
	}*/

	return ret;
}

void FromDBFeatureAlphabet::setFeatureWeight(int idx, double weight) {
	std::wstringstream sql;
	sql << L"update features set weight = " << weight << 
		" where idx = " << idx << " and alphabet = " << _alphIdx;
	_db->exec(sql.str());
}

void FromDBFeatureAlphabet::setFeatureWeights(const VectorXd& values) {
	// do this as one transaction for speed
	_db->beginTransaction();

	for (int i=0; i< values.size(); ++i) {
		setFeatureWeight(i, values(i));
	}
	_db->endTransaction();
}

void FromDBFeatureAlphabet::featureRegularizationWeights(VectorXd& regWeights,
		VectorXd& negRegWeights) const
{
	if (size() != regWeights.size()) {
		throw UnexpectedInputException("FeatureAlphabet::featureRegularizationWeights",
			"Regularization weight vector size does not match number of features in DB");
	}

	std::wstringstream sql;
	sql << L"select idx, regularization, prettyprint from features where alphabet = "
		<< _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	BOOST_FOREACH(std::vector<std::wstring>& row, *tbl) {
		int idx = boost::lexical_cast<int>(row[0]);
		double val = boost::lexical_cast<double>(row[1]);
		negRegWeights(idx) = val;

		// KBP evaluation-time hack to ensure that single-slot features
		// only ever act as penalties
		if (row[2].find(L"> CONTAINS ") == std::wstring::npos
				&& row[2].find(L"> IS ") == std::wstring::npos) 
		{
			regWeights(idx) = val;
		} else {
			regWeights(idx) = 500.0 * val;
		}
	}
}

std::pair<VectorXd,VectorXd> FromDBFeatureAlphabet::featureRegularizationWeights() const {
	VectorXd regWeights(size());
	VectorXd negRegWeights(size());
	featureRegularizationWeights(regWeights, negRegWeights);
	return std::make_pair(regWeights, negRegWeights);
}

std::wstring FromDBFeatureAlphabet::getFeatureName(int idx) const {
	std::wstringstream sql;
	sql << L"select prettyprint from features where idx = " << idx
		<< " and alphabet = " << _alphIdx;
	DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

	if (tbl->empty()) {
		std::wstringstream err;
		err << "No feature " << idx << " in alphabet " << _alphIdx;
		throw UnexpectedInputException("FeatureAlphabet::getFeatureName()",
			"No such feature");
	}

	return (*tbl)[0][0];
}

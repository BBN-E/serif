#include "Generic/common/leak_detection.h"
#include "InstanceAnnotationDB.h"

#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/SessionLogger.h"
#include "ActiveLearning/AnnotatedInstanceRecord.h"
#include "database/SqliteDBConnection.h"

using boost::lexical_cast;
using std::pair;
using std::make_pair;

InstanceAnnotationDBView::InstanceAnnotationDBView(DatabaseConnection_ptr db) : _db(db) {}

std::vector<AnnotatedInstanceRecord> InstanceAnnotationDBView::getInstanceAnnotations() const 
{
	std::vector<AnnotatedInstanceRecord> ret;

	if (sizeof(size_t) == 8) {
		if (_db->tableExists(L"instance_annotations")) {
			translateOldStyleInstanceAnnotations();
			_db->exec(L"drop table instance_annotations");
		}

		if (_db->tableExists(L"instance_annotations_split")) {
			std::wstringstream sql;

			sql << "select hash_high, hash_low, positive from instance_annotations_split";
			DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

			BOOST_FOREACH(std::vector<std::wstring>& row, *tbl) {
				try {
					ret.push_back(AnnotatedInstanceRecord(
						makeSizeT(boost::lexical_cast<unsigned int>(row[0]),
						boost::lexical_cast<unsigned int>(row[1])),
						boost::lexical_cast<int>(row[2])));
				} catch (boost::bad_lexical_cast&) {
					std::wstringstream err;
					err << L"Lexical cast failed for instance annotation (high="
						<< row[0] << L", low=" << row[1];
					SessionLogger::err("bad_instance_lexical_cast") 
						<< L"InstanceAnnotationDBView::getInstanceAnnotations()"
						<< L": " << err.str();
				}
			}
		}
	} else {
		SessionLogger::warn("active_learning_wrong_architecture") <<
			L"Loading instance annotations is only supported on 64-bit systems. Skipping.";
	}

	return ret;
}

size_t InstanceAnnotationDBView::makeSizeT(unsigned int highBits,
		unsigned int lowBits)
{
	// hash ids are 64-bit, but sqlite can't store 64 bit unsigned integers
	// instead, we store two 32-bit fields for the high and low bits, 
	// then recombine them. This will produce the wrong results on
	// a 32-bit system, but the trainers (which use the instance annotations)
	// aren't supported on 32-bit anyway
	unsigned long long ret = highBits;
	return (size_t)(ret << 32 | lowBits);
}

void InstanceAnnotationDBView::translateOldStyleInstanceAnnotations() const
{
		SessionLogger::info("translate_instance_annotations") <<
			L"Translating instance annotations from old style to new style";

		createSplitAnnotationTable();

		std::wstringstream sql;
		
		sql << L"select hash, positive from instance_annotations";
		DatabaseConnection::Table_ptr tbl = _db->exec(sql.str());

		BOOST_FOREACH(std::vector<std::wstring>& row, *tbl) {
			try {
				size_t old_hash = boost::lexical_cast<size_t>(row[0]);
				pair<size_t,size_t> parts = split64Bit(old_hash);
				std::wstringstream sql_new;
				sql_new << L"insert into instance_annotations_split values ("
					<< parts.first << L", " << parts.second << L", "
					<< row[1] << L")";
				_db->exec(sql.str());
			} catch (boost::bad_lexical_cast&) {
				std::wstringstream err;
				err << L"During translation, lexical cast failed for instance annotation id " << row[0];
				SessionLogger::err("bad_instance_lexical_cast")
					<< L"InstanceAnnotatationDBView::getInstanceAnnotations()" 
					<< L": " << err.str();
			}
		}
}

std::pair<size_t, size_t> InstanceAnnotationDBView::split64Bit(size_t big_hash) {
	// this won't work on 32-bit systems, but we will only call it on
	// 64-bit systems anyway
	unsigned long long big_hash_long = big_hash;
	unsigned long long high_bits = big_hash_long >> 32;
	unsigned long long low_bits = big_hash_long & 0xFFFFFFFF;

	return make_pair((size_t)high_bits, (size_t)low_bits);
}

void InstanceAnnotationDBView::createSplitAnnotationTable() const {
	SessionLogger::info("create_split_annotation_table")
		<< L"Creating new-style instance annotation table.";
	_db->exec(L"create table instance_annotations_split (hash_high integer, hash_low integer, positive integer)");
}

void InstanceAnnotationDBView::recordAnnotations(
	const std::vector<AnnotatedInstanceRecord>& annotations) 
{
	// if the instance_annotations table does not exists, create it
	if (!_db->tableExists(L"instance_annotations_split")) {
		createSplitAnnotationTable();
	}

	_db->beginTransaction();
	BOOST_FOREACH(AnnotatedInstanceRecord ann, annotations) {
		std::wstringstream sql;
		std::pair<size_t, size_t> parts = split64Bit(ann.instance_hash());
		sql << L"insert or replace into instance_annotations_split (\"hash_high\", \"hash_low\", \"positive\")" 
			<< L" VALUES (" << parts.first << L", " << parts.second 
			<< L", " << ann.annotation() << L")";
		_db->exec(sql.str());
	}
	SessionLogger::info("sync_instance_annotations") << L"Synced " 
		<< annotations.size() << L" instance annotations";
	_db->endTransaction();
}


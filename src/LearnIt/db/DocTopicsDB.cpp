#pragma warning(disable:4996)
#include "Generic/common/leak_detection.h"
#include <string>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "Generic/sqlite/sqlite3.h"
#include "LearnIt/features/Feature.h"
#include "DocTopicsDB.h"
#include "Generic/common/UnexpectedInputException.h"


std::vector<std::wstring> DocTopicsDB::getAllTopics(const std::string& full_docid) const {
	std::vector<std::wstring> docid_parts;
	std::wstring full_doc_id_wide(full_docid.begin(), full_docid.end());
	boost::split(docid_parts, full_doc_id_wide, boost::is_any_of(L"/"));
	DatabaseConnection::Table_ptr t = _db.exec(L"select topic0, topic1, topic2, topic3, topic4, topic5, topic6, topic7, topic8, topic9, topic10, topic11 from doc_topics where docid='"+docid_parts[1]+L"'");
	if (t->size() != 1) {
		throw UnexpectedInputException("DocTopicsDB::getAllTopics",
			"More than one result for docid");
	}
	for (unsigned int i = 0; i < (*t)[0].size(); i++) {
		boost::erase_all( (*t)[0][i], L"\'" );
	}
	return (*t)[0];
}

std::vector<std::wstring> DocTopicsDB::getAllWeights(const std::string& full_docid) const {
	std::vector<std::wstring> docid_parts;
	std::wstring full_doc_id_wide(full_docid.begin(), full_docid.end());
	boost::split(docid_parts, full_doc_id_wide, boost::is_any_of(L"/"));
	DatabaseConnection::Table_ptr t = _db.exec(L"select weight0, weight1, weight2, weight3, weight4, weight5, weight6, weight7, weight8, weight9, weight10, weight11 from doc_topics where docid='"+docid_parts[1]+L"'");
	if (t->size() != 1) {
		throw UnexpectedInputException("DocTopicsDB::getAllWeights",
			"More than one result for docid");
	}
	for (unsigned int i = 0; i < (*t)[0].size(); i++) {
		boost::erase_all( (*t)[0][i], L"\"" );
	}
	return (*t)[0];
}

std::vector<std::pair<int, std::wstring> > DocTopicsDB::getTopicInfo(const std::string& full_docid) const {
	std::vector<std::wstring> topics = getAllTopics(full_docid);
	std::vector<std::wstring> weights = getAllWeights(full_docid);
	std::vector<std::pair<int, std::wstring> > topic_info;
	for (unsigned int i = 0; i < topics.size(); i++) {
		boost::erase_all( topics[i], L"\'" );
		boost::erase_all( weights[i], L"\"" );
		int topic_id = boost::lexical_cast<int>( topics[i] );
		topic_info.push_back(std::make_pair(topic_id, weights[i]));
	}
	return topic_info;
}

std::wstring DocTopicsDB::getTopicInfoString(const std::string& full_docid) const {
	std::vector<std::pair<int, std::wstring> > topic_info = getTopicInfo(full_docid);
	std::vector<std::wstring> pair_strings;
	for (unsigned int i=0; i < topic_info.size(); i++) {
		std::wstring pair_string = boost::lexical_cast<std::wstring>(topic_info[i].first) + L":" + topic_info[i].second;
		pair_strings.push_back(pair_string);
	}
	return L"{ " + boost::algorithm::join(pair_strings, L", ") + L" }";
}

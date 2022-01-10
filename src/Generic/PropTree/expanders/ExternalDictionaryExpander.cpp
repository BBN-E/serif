// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/ExternalDictionaryExpander.h"
#include "../Predicate.h"
#include "../PropNode.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include <map>
#include <string>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp> 

ExternalDictionaryExpander::ExternalDictionaryExpander() : 
  PropTreeExpander()
{
	initializeTables();
}


void ExternalDictionaryExpander::expand(const PropNodes& pnodes) const {
	using namespace boost; using namespace std;

	if (_dictionary.size() == 0)
		return;

	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		BOOST_FOREACH(const PropNode::WeightedPredicate& wpred, node->getPredicates()) {
			const std::wstring word = wpred.first.pred().to_string();
			if (wpred.second < 0.95)
				continue;
			external_dictionary_t::const_iterator iter = _dictionary.find(word);
			if (iter != _dictionary.end()) {
				BOOST_FOREACH(dictionary_entry_t entry, (*iter).second) {
					node->addPredicate(Predicate(entry.pred_type, entry.word, wpred.first.negative()), entry.score * wpred.second);				
				}
			}
		}	
	}
}
void ExternalDictionaryExpander::initializeTables() {
	std::string filename = ParamReader::getParam("external_proptree_dictionary");
	if (filename == "") {
		SessionLogger::warn("prop_trees") << "Running without external proptree dictionary\n";
		return;
	}

	boost::scoped_ptr<UTF8InputStream> input_scoped_ptr(UTF8InputStream::build(filename.c_str()));
	UTF8InputStream& input(*input_scoped_ptr);
	if (input.fail()) {
		std::stringstream errMsg;
		errMsg << "Failed to open external dictionary: " << filename << "\nSpecified by paramter 'external_proptree_dictionary'";
		throw UnexpectedInputException("ExternalDictionaryExpander::initializeTables()", errMsg.str().c_str());
	}

	std::vector<std::wstring> words;
	while( !input.eof() ){
		std::wstring line;
		input.getLine(line);
		if (line.length() == 0)
			continue;
		std::vector<std::wstring> things;
		boost::split(things, line, boost::is_any_of(L" "));
		if (things.size() == 1) {
			boost::split(words, things.at(0), boost::is_any_of(L","));
			BOOST_FOREACH(std::wstring word, words) {
				if (_dictionary.find(word) == _dictionary.end()) {
					std::vector<dictionary_entry_t> entries;			
					_dictionary[word] = entries;
				}
			}
			continue;
		}
		if (words.size() == 0)
			continue;
		if (things.size() == 3) {
			std::vector<std::wstring> new_words;
			boost::split(new_words, things.at(2), boost::is_any_of(L","));
			float score = 0;				
			try {
				score =  boost::lexical_cast<float>(things.at(1));
			} catch (boost::bad_lexical_cast) { 
				continue;
			} 			
			BOOST_FOREACH(std::wstring new_word, new_words) {
				dictionary_entry_t entry;
				entry.word = new_word;
				entry.pred_type = things.at(0);
				entry.score = score;
				BOOST_FOREACH(std::wstring word, words) {
					if (word != new_word)
						_dictionary[word].push_back(entry);
				}
			}
		}
	}

	input.close();
}

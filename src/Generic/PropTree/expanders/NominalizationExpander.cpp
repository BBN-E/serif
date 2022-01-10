// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/NominalizationExpander.h"
#include "../Predicate.h"
#include "../PropNode.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include <map>
#include <string>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

NominalizationExpander::NominalizationExpander(float nom_prob) : 
  PropTreeExpander(), NOMINALIZATION_PROB(nom_prob)
{
	initializeTables();
}


void NominalizationExpander::expand(const PropNodes& pnodes) const {
	using namespace boost; using namespace std;

	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		BOOST_FOREACH(const PropNode::WeightedPredicate& wpred, node->getPredicates()) {
			if (wpred.first.type() == Predicate::DESC_TYPE) {
				const std::wstring word=wpred.first.pred().to_string();
				const std::wstring verb=verbifyWord(word);
				if (verb != L"") {
					node->addPredicate(Predicate(Predicate::VERB_TYPE, verb, wpred.first.negative()), NOMINALIZATION_PROB * wpred.second);				
				}
			} else if (wpred.first.type() == Predicate::VERB_TYPE) {
				const std::wstring word=wpred.first.pred().to_string();
				const std::wstring noun=nominalizeWord(word);
				if (noun != L"") {
					node->addPredicate(Predicate(Predicate::DESC_TYPE, noun, wpred.first.negative()), NOMINALIZATION_PROB * wpred.second);				
				}
			} 
		}	
	}
}

std::wstring NominalizationExpander::nominalizeWord(const std::wstring& word) const {	
	std::map<std::wstring, std::wstring>::const_iterator iter = _nominalizations.find(word);
	if (iter != _nominalizations.end())
		return (*iter).second;		
	else return L"";
}

std::wstring NominalizationExpander::verbifyWord(const std::wstring& word) const {	
	std::map<std::wstring, std::wstring>::const_iterator iter = _verbifications.find(word);
	if (iter != _verbifications.end())
		return (*iter).second;		
	else return L"";
}

std::map<std::wstring, std::wstring> _nominalizations;
std::map<std::wstring, std::wstring> _verbifications;

void NominalizationExpander::initializeTables() {
	std::string filename = ParamReader::getParam("nominalization_table");
	if (filename == "") {
		SessionLogger::warn("prop_trees") << "Running without nominalization expansion table\n";
		return;
	}

	boost::scoped_ptr<UTF8InputStream> input_scoped_ptr(UTF8InputStream::build(filename.c_str()));
	UTF8InputStream& input(*input_scoped_ptr);
	if (input.fail()) {
		std::stringstream errMsg;
		errMsg << "Failed to open nominalization table " << filename << "\nSpecified by parameter nominalization_table";
		throw UnexpectedInputException( "NominalizationExpander::initializeTables()", errMsg.str().c_str());
	}

	while( !input.eof() ){
		std::wstring line;
		input.getLine(line);
		if (line.length() == 0)
			continue;
		size_t space = line.find(L" ");
		if (space == std::wstring::npos || space == line.length() - 1)
			throw UnexpectedInputException( "NominalizationExpander::initializeTables()", "Malformed nominalization table" );

		std::wstring noun = line.substr(0, space);
		std::wstring verb = line.substr(space+1);

		// fail on bad lines
		if( noun.length() == 0 || verb.length() == 0)
			throw UnexpectedInputException( "NominalizationExpander::initializeTables()", "Malformed nominalization table" );

		_verbifications[noun] = verb;
		_nominalizations[verb] = noun;
	}

	input.close();
}

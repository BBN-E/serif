// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.


#include "common/leak_detection.h"
#include "Predicate.h"
#include "theories/Mention.h"
#include "common/UTF8OutputStream.h"
#include "common/ParamReader.h"
#include "common/SymbolUtilities.h"
#include "common/InputUtil.h"

// Mention-derived types
const Symbol Predicate::NONE_TYPE(L"none");
const Symbol Predicate::NAME_TYPE(L"name");
const Symbol Predicate::PRON_TYPE(L"pron");
const Symbol Predicate::DESC_TYPE(L"desc");
const Symbol Predicate::PART_TYPE(L"part");
const Symbol Predicate::APPO_TYPE(L"appo");
const Symbol Predicate::LIST_TYPE(L"list");
const Symbol Predicate::INFL_TYPE(L"infl");

// Other types
const Symbol Predicate::VERB_TYPE(L"verb");
const Symbol Predicate::MOD_TYPE( L"mod");
const Symbol Predicate::MODAL_TYPE( L"modal");

static boost::hash<int> int_hasher();

std::wostream & operator << (std::wostream & s, const Predicate & p){
	s << p.type() << L"(" << (p.negative() ? L"!'" : L"'") << p.pred() << L"')";
	return s;
}

UTF8OutputStream & operator << (UTF8OutputStream & s, const Predicate & p){
	s << p.type() << L"(" << (p.negative() ? L"!'" : L"'") << p.pred() << L"')";
	return s;
}

// Symbols used by Predicate::validPredicate(), which checks against a 
// short list of common, invalid predicates (eg, "'s", etc).
// Pythia version omits "'s". I'm restoring it, but not sure if it should be
bool Predicate::validPredicate( const Symbol & p ){
	
	static bool init = false;
	static std::set<Symbol> invalidPredicates;
	if (!init) {
		std::string input_file = ParamReader::getParam("invalid_proptree_predicates");
		if (!input_file.empty()) {
			invalidPredicates = InputUtil::readFileIntoSymbolSet(input_file, false, true);
		}
		invalidPredicates.insert(L"");
		invalidPredicates.insert(L"'s");
		invalidPredicates.insert(L"/");
		invalidPredicates.insert(L"\\");
		invalidPredicates.insert(L"-");
		init = true;
	}

	Symbol lower_p = SymbolUtilities::lowercaseSymbol(p);
	return (invalidPredicates.size() == 0 || invalidPredicates.find(lower_p) == invalidPredicates.end());
}

Symbol Predicate::mentionPredicateType( const Mention * ment )
{
	switch(ment->getMentionType()){
		case Mention::NONE:
			return Predicate::NONE_TYPE;
		case Mention::NAME:
			return Predicate::NAME_TYPE;
		case Mention::PRON:
			return Predicate::PRON_TYPE;
		case Mention::DESC:
			return Predicate::DESC_TYPE;
		case Mention::PART:
			return Predicate::PART_TYPE;
		case Mention::APPO:
			return Predicate::APPO_TYPE;
		case Mention::LIST:
			return Predicate::LIST_TYPE;
		case Mention::INFL:
			return Predicate::INFL_TYPE;
		default:
			throw UnexpectedInputException("Predicate::mentionPredicateType()", "Unrecognized MentionType");
	}
}


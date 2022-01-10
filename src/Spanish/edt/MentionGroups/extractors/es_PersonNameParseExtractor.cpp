// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/edt/MentionGroups/extractors/es_PersonNameParseExtractor.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include "Spanish/common/es_WordConstants.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <sstream>

namespace {
	const bool DEBUG=false;

	const Symbol::SymbolGroup H_SUFFIX = Symbol::makeSymbolGroup(L"h h."); // son of (jr.)
	const Symbol COMMA(L",");
	const Symbol HYPHEN(L"-");
	const Symbol DE(L"de");
	const Symbol D_APPOS(L"d'");
	const Symbol DEL(L"del");
	const Symbol LE(L"le");
	const Symbol LA(L"la");
	const Symbol LOS(L"los");
	const Symbol LAS(L"las");

}

template<> std::wstring AttributeValuePair<SpanishPersonNameParseExtractor::SpanishNameParseList>::toString() const {
	return L"not-implemented-yet";
}

template<> bool AttributeValuePair<SpanishPersonNameParseExtractor::SpanishNameParseList>::valueEquals(const AttributeValuePair_ptr other) const { 
	boost::shared_ptr< AttributeValuePair<SpanishPersonNameParseExtractor::SpanishNameParseList> > p = boost::dynamic_pointer_cast< AttributeValuePair<SpanishPersonNameParseExtractor::SpanishNameParseList> >(other);
	if (!p) return false;
	return (p->_value == _value);
}

template<> bool AttributeValuePair<SpanishPersonNameParseExtractor::SpanishNameParseList>::equals(const AttributeValuePair_ptr other) const { 
	return (getFullName()==other->getFullName() && valueEquals(other));
}




const Symbol SpanishPersonNameParseExtractor::FEATURE_NAME(L"name-parse"); 
const Symbol SpanishPersonNameParseExtractor::EXTRACTOR_NAME(L"es-person-name-parse"); 
const Symbol SpanishPersonNameParseExtractor::GIVEN_FEATURE(L"possible-given");
const Symbol SpanishPersonNameParseExtractor::SURNAME1_FEATURE(L"possible-surname1");
const Symbol SpanishPersonNameParseExtractor::GIVEN_SURNAME1_FEATURE(L"possible-given-surname1");

SpanishPersonNameParseExtractor::SpanishPersonNameParseExtractor() 
	: AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), SpanishPersonNameParseExtractor::EXTRACTOR_NAME)
{
}

namespace {
	Symbol join_words(const std::vector<Symbol> &words, size_t first, size_t last) {
		// Special cases.
		if (last==first) return Symbol();
		if (last==(first+1)) return words[first];
		// Join with underscores.
		std::wstringstream joined;
		for (size_t i=first; i<last; ++i) {
			if (i!=first) joined << L"_";
			joined << words[i];
		}
		return Symbol(joined.str().c_str());
	}
}

/* This method is a direct adaptation of SpanishRuleNameLinker::generatePERParse() */
std::vector<AttributeValuePair_ptr> SpanishPersonNameParseExtractor::extractFeatures(const Mention& context,
																					LinkInfoCache& cache,
																					const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;

	if (context.getEntityType() != EntityType::getPERType() || context.getMentionType() != Mention::NAME)
		return results; // no features for this mention.

	std::vector<Symbol> words = context.getHead()->getTerminalSymbols();


	// Strip suffix, if it occurs.
	Symbol suffix;
	if (words.back().isInSymbolGroup(H_SUFFIX)) {
		suffix = words.back();
		words.pop_back();
	}

	// Check for composite names, and collapse them.
	size_t pos = 1;
	while ((pos+1) < words.size()) {
		Symbol cur = words[pos];
		Symbol next = words[pos+1];
		if ((pos < words.size()-2) && (cur==DE) && ((next==LE) || (next==LA) || (next==LOS) || (next==LAS))) {
			// Maria de la Mer
			words[pos-1] = join_words(words, pos-1, pos+3);
			words.erase(words.begin()+pos, words.begin()+pos+3);
		} else if ((cur==HYPHEN) || (cur==DE) || (cur==DEL) || (cur==D_APPOS)) {
			// Maria del Mar
			words[pos-1] = join_words(words, pos-1, pos+2);
			words.erase(words.begin()+pos, words.begin()+pos+2);
		} else if (cur==D_APPOS) {
			// Maria d' Apricot -- do not add "_" after "d'".
			std::wstringstream collapsed;
			collapsed << words[pos-1] << L"_" << words[pos] << words[pos+1];
			words[pos-1] = Symbol(collapsed.str().c_str());
			words.erase(words.begin()+pos, words.begin()+pos+2);
		} else if (boost::starts_with(words[pos].to_string(), L"d'")) {
			// Maria d'Apricot
			words[pos-1] = join_words(words, pos-1, pos+1);
			words.erase(words.begin()+pos, words.begin()+pos+1);
		} else {
			++pos;
		}
	}

	// Determine which word in the word vector corresponds to which name
	std::vector<SpanishNameParse> parses;

	// Does the name contain a comma (eg "Perez-Garcia, Maria Paulina")?
	std::vector<Symbol>::iterator comma_pos = std::find(words.begin(), words.end(), COMMA);
	if (comma_pos != words.end()) {
		// Given is everything that comes after the comma.
		std::vector<Symbol> normalized_words;
		normalized_words.insert(normalized_words.end(), comma_pos+1, words.end());
		size_t given_end = normalized_words.size();
		normalized_words.insert(normalized_words.end(), words.begin(), comma_pos);
		for (size_t surname1_end=given_end; surname1_end<=words.size(); ++surname1_end) {
			if ((given_end==surname1_end) && (surname1_end<words.size()))
				continue; // only include surname2 if surname1 is non-empty.
			if (((surname1_end-given_end)>2) && (words.size()<6))
				continue; // only consider surnames w/ >2 words if the name is very long.
			if (((words.size()-surname1_end)>2) && (words.size()<6))
				continue; // only consider surnames w/ >2 words if the name is very long.
			parses.push_back(SpanishNameParse(words, given_end, surname1_end, suffix));
		}
	} else {
		// Otherwise, try all (reasonable) split points.
		for (size_t given_end=0; given_end<=words.size(); ++given_end) {
			for (size_t surname1_end=given_end; surname1_end<=words.size(); ++surname1_end) {
				if (surname1_end==0)
					continue; // given or surname1 must be non-empty.
				if ((given_end==surname1_end) && (surname1_end<words.size()))
					continue; // only include surname2 if surname1 is non-empty.
				if ((given_end>2) && (words.size()<6))
					continue; // only consider given names w/ >2 words if the name is very long.
				if (((surname1_end-given_end)>2) && (words.size()<6))
					continue; // only consider surnames w/ >2 words if the name is very long.
				if (((words.size()-surname1_end)>2) && (words.size()<6))
					continue; // only consider surnames w/ >2 words if the name is very long.
				parses.push_back(SpanishNameParse(words, given_end, surname1_end, suffix));
			}
		}
	}

	if (DEBUG) {
		std::wcout << L"Spanish name: " << context.getHead()->toTextString() << "\n";
		BOOST_FOREACH(const SpanishNameParse& parse, parses) {
			std::wcout << "  " << parse.toString() << std::endl;
		}
	}

	// Add the feature.
	results.push_back(AttributeValuePair<SpanishNameParseList>::create(FEATURE_NAME, parses, getFullName()));

	BOOST_FOREACH(const SpanishNameParse& parse, parses) {
		Symbol given = parse.getPiece(SpanishNameParse::GIVEN);
		Symbol surname1 = parse.getPiece(SpanishNameParse::SURNAME1);
		if (!given.is_null())
			results.push_back(AttributeValuePair<Symbol>::create(GIVEN_FEATURE, given, getFullName()));
		if (!surname1.is_null())
			results.push_back(AttributeValuePair<Symbol>::create(SURNAME1_FEATURE, surname1, getFullName()));
		if ((!given.is_null()) && (!surname1.is_null())) {
			std::wstringstream joined;
			joined << given << L"_" << surname1;
			results.push_back(AttributeValuePair<Symbol>::create(GIVEN_SURNAME1_FEATURE, Symbol(joined.str().c_str()), getFullName()));
		}
	}

	return results;
}

SpanishPersonNameParseExtractor::SpanishNameParse::SpanishNameParse() {
}

SpanishPersonNameParseExtractor::SpanishNameParse::SpanishNameParse(const std::vector<Symbol> &words, size_t given_end, size_t surname1_end, Symbol suffix)
{
	pieces[GIVEN]=join_words(words, 0, given_end);
	pieces[SURNAME1]=join_words(words, given_end, surname1_end);
	pieces[SURNAME2]=join_words(words, surname1_end, words.size());
	pieces[SUFFIX]=suffix;
	for (size_t i=0; i<given_end; ++i)
		sym2piece[words[i]] |= 1<<GIVEN;
	for (size_t i=given_end; i<surname1_end; ++i)
		sym2piece[words[i]] |= 1<<SURNAME1;
	for (size_t i=surname1_end; i<words.size(); ++i)
		sym2piece[words[i]] |= 1<<SURNAME2;
}

SpanishPersonNameParseExtractor::SpanishNameParse SpanishPersonNameParseExtractor::SpanishNameParse::merge(const SpanishNameParse& other) const {
	SpanishNameParse result;
	for (size_t i=0; i<NUM_PIECES; ++i)
		result.pieces[i] = (pieces[i].is_null()?other.pieces[i]:pieces[i]);
	for (Symbol::HashMap<int>::iterator sp=sym2piece.begin(); sp!=sym2piece.end(); ++sp) {
		result.sym2piece[(*sp).first] |= (*sp).second;
	}
	for (Symbol::HashMap<int>::iterator sp=other.sym2piece.begin(); sp!=other.sym2piece.end(); ++sp) {
		result.sym2piece[(*sp).first] |= (*sp).second;
	}
	return result;
}
bool SpanishPersonNameParseExtractor::SpanishNameParse::is_consistent(const SpanishNameParse& other) const {
	// Check if any individual piece conflicts.
	for (size_t i=0; i<NUM_PIECES; ++i) {
		if ((!pieces[i].is_null()) && (!other.pieces[i].is_null()) && (pieces[i]!=other.pieces[i])) {
			return false;
		}
	}
	// Check if the same word is used for multiple pieces.
	for (Symbol::HashMap<int>::iterator sp=sym2piece.begin(); sp!=sym2piece.end(); ++sp) {
		Symbol::HashMap<int>::iterator osp = other.sym2piece.find((*sp).first);
		if (osp != other.sym2piece.end()) {
			if ((*sp).second != (*osp).second)
				return false;
		}
	}
	return true;
}
bool SpanishPersonNameParseExtractor::SpanishNameParse::operator==(const SpanishNameParse& other) const {
	for (size_t i=0; i<NUM_PIECES; ++i)
		if (pieces[i] != other.pieces[i]) return false;
	return true;
}

std::wstring SpanishPersonNameParseExtractor::SpanishNameParse::toString() const {
	static const Symbol EMPTY_SYMBOL(L"");
	std::wstringstream out;
	for (size_t i=0; i<NUM_PIECES; ++i)
		out << L"[" << (pieces[i].is_null()?EMPTY_SYMBOL:pieces[i]) << L"]";
	return out.str();
}

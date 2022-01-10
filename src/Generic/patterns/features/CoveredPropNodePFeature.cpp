// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/CoveredPropNodePFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
	

void CoveredPropNodePFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	out << L"    <focus type=\"covered_prop_node\"";
    printOffsetsForSpan(patternMatcher, _pnode_sent_no, _pnode_start_token, _pnode_end_token, out);
	out << L" val" << val_score << "=\"" << _score << L"\"";
	out << L" val" << val_extra << "=\"" << _matchType << L"\"";
	out << L" val" << val_extra_2 << L"=\"" << _pnode_sent_no << L"\"";
	out << L" val" << val_extra_3 << L"=\"" << _pnode_start_token << L"\"";
	out << L" val" << val_extra_4 << L"=\"" << _pnode_end_token << L"\"";
	out << L" val" << val_extra_5 << L"=\"" << _query_predicate << L"\"";
	out << L" val" << val_extra_6 << L"=\"" << _document_predicate << L"\"";
	out << L" val" << val_extra_7 << L"=\"" << _matcher_index << L"\"";
	out << L" val" << val_extra_8 << L"=\"" << _matcher_total << L"\"";
	out << L" val" << val_extra_9 << L"=\"" << _query_slot << L"\"";
	out << L" />\n";
}

void CoveredPropNodePFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_score, _score);	
	elem.setAttribute(X_pnode_sentence_number, _pnode_sent_no);
	elem.setAttribute(X_pnode_start_token, _pnode_start_token);
	elem.setAttribute(X_pnode_end_token, _pnode_end_token);
	elem.setAttribute(X_query_predicate, _query_predicate);
	elem.setAttribute(X_document_predicate, _document_predicate);
	elem.setAttribute(X_query_slot, _query_slot);
	elem.setAttribute(X_matcher_index, _matcher_index);
	elem.setAttribute(X_matcher_total, _matcher_total);
	elem.setAttribute(X_match_type, int(_matchType));

}

CoveredPropNodePFeature::CoveredPropNodePFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_score = elem.getAttribute<float>(X_score);
	_pnode_sent_no = elem.getAttribute<int>(X_pnode_sentence_number);
	_pnode_start_token = elem.getAttribute<int>(X_pnode_start_token);
	_pnode_end_token = elem.getAttribute<int>(X_pnode_end_token);
	_query_predicate = elem.getAttribute<Symbol>(X_query_predicate);
	_document_predicate = elem.getAttribute<Symbol>(X_document_predicate);
	_query_slot = elem.getAttribute<Symbol>(X_query_slot);
	_matcher_index = elem.getAttribute<int>(X_matcher_index);
	_matcher_total = elem.getAttribute<int>(X_matcher_total);
	_matchType = AbstractSlot::MatchType(elem.getAttribute<int>(X_match_type));

}

// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/SimpleSlot.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/PropTree/PropForestFactory.h"
#include "Generic/PropTree/expanders/DistillationExpander.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/TokenSequence.h"

#include "boost/regex.hpp"

SimpleSlot::SimpleSlot(const Symbol& label, DocTheory* docTheory, int start_token_offset, const NameDictionary& equivalent_names, std::map<std::wstring, float> &weights) : 
_label(label), _docTheory(docTheory), _start_token_offset(start_token_offset), _equivalent_names(equivalent_names) {

	// Set various member variables according to the doc theory.  The order here matters.
	setParseNodes();
	setMainMention();
	setNameMention();
	setNameString();

	// Construct our backoff regex text.  This is a bit of a misnomer.  It is what PatternMatcher uses to determing exact matches.
	_backoff_regex_text = constructBackoffRegexText();

	// Get our slot nodes
	PropNode::PropNodes slot_nodes = constructSlotForest();

	// Construct and apply our expander
	if (SessionLogger::dbg_or_msg_enabled("SimpleSlot")) {
		printEquivalentNames();
	}
	DistillationQueryExpander expander(equivalent_names);
	expander.expand(slot_nodes);

	std::map<PropNode_ptr, float> nodeWeights = getNodeWeights(slot_nodes, weights);

	PropNode::PropNodes slot_nodes_no_modals;
	std::vector<float> weights_no_modals;
	BOOST_FOREACH(PropNode_ptr node, slot_nodes) {	
		// Get rid of modals for prop-node-match
		const Predicate* pred = node->getRepresentativePredicate();
		if (!pred || pred->type() != Predicate::MODAL_TYPE) {
			slot_nodes_no_modals.push_back(node);
			weights_no_modals.push_back(nodeWeights[node]);
		}
	}

	if (ParamReader::isParamTrue("print_debug_prop_nodes")) {
		std::wcout << L"BEGIN prop nodes\n";
		BOOST_FOREACH(PropNode_ptr node, slot_nodes) {		
			node->compactPrint(std::wcout, true, true, true, 0);
			std::wcout << L"\n";
		}
		std::wcout << L"END prop nodes\n";
	}

	// But if modals are all we've got, add them back in (just to be safe)
	if (slot_nodes_no_modals.size() == 0) {
		BOOST_FOREACH(PropNode_ptr node, slot_nodes) {
			slot_nodes_no_modals.push_back(node);
			weights_no_modals.push_back(nodeWeights[node]);
		}
	}

	// Construct our pattern matchers
	//_nodeMatcher = boost::shared_ptr<PropNodeMatch>(_new PropNodeMatch(slot_nodes_no_modals));
	_nodeMatcher = boost::shared_ptr<PropNodeMatch>(_new PropNodeMatch(slot_nodes_no_modals, weights_no_modals));
	_edgeMatcher = boost::shared_ptr<PropEdgeMatch>(_new PropEdgeMatch(slot_nodes));
	_fullMatcher = boost::shared_ptr<PropFullMatch>(_new PropFullMatch(slot_nodes, false));
}

SimpleSlot::~SimpleSlot() {
	_nodeMatcher.reset();
	_edgeMatcher.reset();
	_fullMatcher.reset();
}

Symbol SimpleSlot::getLabel() const {
	return _label;
}

bool SimpleSlot::hasName() const {
	return _name_string != L"";
}

std::wstring SimpleSlot::getBackoffRegexText() const {
	return _backoff_regex_text;
}

std::map<PropNode_ptr, float> SimpleSlot::getNodeWeights(PropNode::PropNodes slot_nodes, std::map<std::wstring, float> &weights) {

	std::map<PropNode_ptr, float> nodeWeights;

	for (PropNode::PropNodes::const_iterator nodeIter = slot_nodes.begin(); nodeIter != slot_nodes.end(); nodeIter++){
		PropNode_ptr node = (*nodeIter);
		const Predicate *pred = node->getRepresentativePredicate();
		if (pred == 0){
			nodeWeights[node] = 1.0f;
			continue;
		}

		std::wstring p = pred->pred().to_string();

		std::map<std::wstring, float>::const_iterator iter = weights.find(p);
		if (iter != weights.end()) {
			nodeWeights[node] = (*iter).second;
		} else {
			nodeWeights[node] = 1.0f;
		}

	}

	return nodeWeights;

}

boost::shared_ptr<PropMatch> SimpleSlot::getMatcher(MatchType matchType) const {
	switch(matchType) {
		case NODE: return _nodeMatcher; 
		case EDGE: return _edgeMatcher;
		case FULL: return _fullMatcher;
		default: throw InternalInconsistencyException("SimpleSlot::getMatcher", "Unexpected vaue for matchType");
	}
}

const SentenceTheory* SimpleSlot::getSlotSentenceTheory() const {
	return _docTheory->getSentenceTheory(0);
}

bool SimpleSlot::requiresProptrees() const {
	return true;
}

std::wstring SimpleSlot::constructBackoffRegexText() {
	std::wstringstream regex_str_stream;
	regex_str_stream << L"(?i)";
	const TokenSequence* toks = getSlotSentenceTheory()->getTokenSequence();
	for(int i = _start_token_offset;  i < toks->getNTokens(); i++) {
		std::wstring token_string = toks->getToken(i)->getSymbol().to_string();
	  
		// make sure to include Chinese full stop character
		if ((toks->getToken(i)->getSymbol() == Symbol(L"the")) || 
			(toks->getToken(i)->getSymbol() == Symbol(L",")) ||
			(toks->getToken(i)->getSymbol() == Symbol(L".")) ||
			(toks->getToken(i)->getSymbol() == Symbol(L"a")) ||
			(toks->getToken(i)->getSymbol() == Symbol(L"\x3002"))) {
			regex_str_stream << L"(";
			regex_str_stream << token_string;
			regex_str_stream << L")?";
		} else {
			for (size_t ch = 0; ch < token_string.size(); ch++) {
				wchar_t t = token_string.at(ch);
				// escape all punctuation!
				if( t == L'^' || t == L'.' || t == L'$' || t == L'|' || t == L'(' ||
					t == L')' || t == L'[' || t == L']' || t == L'*' || t == L'+' ||
					t == L'?' || t == L'{' || t == L'}' || t == L'\\' ) {
						regex_str_stream << L"\\";
				}
				regex_str_stream << t;
				// make all punctuation optional!! even inside tokens!
				if (iswpunct(t)) {
					regex_str_stream << "?";
				}
			}
		}
		// we need two of these... what if there is a hyphen and a space? e.g. ansar -al- sunnah
		regex_str_stream << L"[-,\\.&' ]?[-,\\.&' ]?";
	}
	return regex_str_stream.str();
}

std::vector<PropNode_ptr> SimpleSlot::constructSlotForest() {
	PropNodes all_nodes;			
	int sent_index = 0;
	const SentenceTheory* stheory = _docTheory->getSentenceTheory(sent_index);
	PropNodes slot_pnodes = PropForestFactory(_docTheory, stheory, sent_index).getAllNodes();
	
	// for each token, identify the largest covering (valid) node
	PropNodes tok_roots(stheory->getTokenSequence()->getNTokens(), PropNode_ptr());
	for (PropNodes::iterator p_it = slot_pnodes.begin(); p_it != slot_pnodes.end(); p_it++) {

		// Conditioning on head token rather than start token means we will sometimes
		//   include something we don't want, e.g. "the topic is". Still, this seems
		//   much less bad than dropping the key very in the sentence, as happens in
		//   queries like "the topic is drilling for oil in ANWR"
		if((*p_it)->getHeadStartToken() < (size_t) _start_token_offset) continue;

		// a node covers a token if that token is within the HEAD span of the node or one of its children
		PropNodes child_nodes;	
		PropNodes::iterator next_p_it = p_it;
		next_p_it++;
		enumeratePropNodes( p_it, next_p_it, child_nodes );
		for( PropNodes::iterator child_it = child_nodes.begin(); child_it != child_nodes.end(); child_it++ ) {
			for( size_t t = (*child_it)->getHeadStartToken(); t <= (*child_it)->getHeadEndToken(); t++ ){
				if( tok_roots[t] == PropNode_ptr() || 
					((*p_it)->getEndToken() - (*p_it)->getStartToken()) >
					(tok_roots[t]->getEndToken() - tok_roots[t]->getStartToken()) )
				{	
					tok_roots[t] = *p_it;
				}
			}
		}
	}
	
	size_t old_size = all_nodes.size();
	
	// trim duplicate nodes, & enumerate into all_nodes
	enumeratePropNodes( tok_roots.begin(), unique(tok_roots.begin(), tok_roots.end()), all_nodes );
	
	sort( all_nodes.begin() + old_size, all_nodes.end(), PropNode::propnode_id_cmp() );
	all_nodes.erase( unique(all_nodes.begin() + old_size, all_nodes.end()), all_nodes.end() );

	return all_nodes;

}

void SimpleSlot::enumeratePropNodes(PropNodes::const_iterator begin, PropNodes::const_iterator end, PropNodes& container) {
	while(begin != end) {
		if(*begin) {
			container.insert(container.end(), *begin);
			enumeratePropNodes((*begin)->getChildren().begin(), (*begin)->getChildren().end(), container);
		}
		++begin;
	}
	return;
}

std::vector<std::wstring> SimpleSlot::getEquivalentNames(const std::wstring& name, double minScore) const {	
	std::wstring theName = UnicodeUtil::normalizeNameString(UnicodeUtil::normalizeTextString(name));
	std::vector<std::wstring> synTerms;
	synTerms.push_back(theName);
	NameDictionary::const_iterator ndci = _equivalent_names.find(theName);
	if (ndci != _equivalent_names.end()) {
		NameSynonyms::const_iterator nsci;
        for (nsci = ndci->second.begin(); nsci != ndci->second.end(); nsci++) {
            if (nsci->second >= minScore) {  // Add in anything greater than or equal to our minimum score
                synTerms.push_back(nsci->first);
            }
        }
	}
	return synTerms;
}

std::wstring SimpleSlot::getRegexNameString(float min_eqn_score) const {
	if (_name_string == L"") {
		return L"";
	}
	const std::vector<std::wstring>& altNames = getEquivalentNames(_name_string, min_eqn_score);
	std::wstring altNamesStr=L"";	
	for ( size_t i=0; i < altNames.size(); i++ ) { // for each term
		altNamesStr += L"|";
		altNamesStr += altNames[i];
	}
	std::wstringstream regexstream;
	regexstream << L"^ ?(?i)(";
	const SynNode* atomic_head = _nameMention->getAtomicHead();
	for (int i = atomic_head->getStartToken(); i < atomic_head->getEndToken(); i++) {
		const SynNode * node = atomic_head->getCoveringNodeFromTokenSpan(i, i);
		// if the POS tag is only one character long (eg. punctuation), replace it with a wildcard regex
		if (wcslen(node->getTag().to_string()) < 2) {
			regexstream << L".?\\s?";
		} else {
			regexstream << _docTheory->getSentenceTheory(0)->getTokenSequence()->getToken(i)->getSymbol().to_string();
		}
		// add punctuation regex between tokens
		regexstream << L"([-,\\.&' ])?";
	}		
	regexstream << _docTheory->getSentenceTheory(0)->getTokenSequence()->getToken(atomic_head->getEndToken())->getSymbol().to_string();
	regexstream << altNamesStr;
	regexstream << L")";
	regexstream << L" ?$";
	return regexstream.str();
}

void SimpleSlot::printEquivalentNames() const {
	typedef std::map<std::wstring, double> inner_t;
	typedef std::map<std::wstring, inner_t> outer_t;
	typedef outer_t::const_iterator outer_iter_t;
	typedef inner_t::const_iterator inner_iter_t;
	std::wstringstream wss;	
	wss << "SimpleSlot::printEquivalentNames()\n";
	for (outer_iter_t outer = _equivalent_names.begin(); outer != _equivalent_names.end(); ++outer) {
		for (inner_iter_t inner = (*outer).second.begin(); inner != (*outer).second.end(); ++inner) {
			wss << (*outer).first << ": (" << (*inner).first << ", " << (*inner).second << ")\n";
		}
	}	
	SessionLogger::dbg("SimpleSlot") << wss.str();
}

void SimpleSlot::setMainMention() {
	if (_parseNodes.size() == 1 && _parseNodes[0]->hasMention()) {
		_mainMention = _docTheory->getSentenceTheory(0)->getMentionSet()->getMentionByNode(_parseNodes[0]);
	} else {
		_mainMention = 0;
	}
}

void SimpleSlot::setNameMention() {
	if (_mainMention && _mainMention->getMentionType() == Mention::NAME && _mainMention->getNode()->getHeadPreterm()->getParent() == _mainMention->getNode()) {
		_nameMention = _mainMention;
	} else {
		_nameMention = 0;
	}
}

void SimpleSlot::setNameString() {
	if (_nameMention) {
		_name_string = _nameMention->getNode()->getHeadPreterm()->getParent()->toTextString();
	} else {
		_name_string = L"";
	}
}

void SimpleSlot::setParseNodes() {
	//get the parse nodes that correspond with the slot 
	//(ignore first 3 tokens (The topic is/He is discussing/Tell me about) and last token (.)
	//this matches either a single node, or several sister nodes
	//if neither match works, there is no slot parse
	const SynNode* root = _docTheory->getSentenceTheory(0)->getPrimaryParse()->getRoot();
	int end = root->getEndToken() - 1;
	const SynNode* coveringnode =  root->getCoveringNodeFromTokenSpan(_start_token_offset, end);
	while(coveringnode->getParent() != 0) {
		if((coveringnode->getParent()->getStartToken() == coveringnode->getStartToken()) &&
			(coveringnode->getParent()->getEndToken() == coveringnode->getEndToken())) {
				coveringnode = coveringnode->getParent();
		} else {
			break;
		}
	}
	_parseNodes.clear();
	if ((coveringnode->getStartToken() == _start_token_offset) && (coveringnode->getEndToken() == end)) {
		_parseNodes.push_back(coveringnode);
	} else {
		int s = -1;
		int e = -1;
		int nchildren = coveringnode->getNChildren();
		for(int i =0; i< nchildren ;i++) {
			if(coveringnode->getChild(i)->getStartToken() == _start_token_offset){
				s= i;
			}
			if(coveringnode->getChild(i)->getEndToken() == end) {
				e =i;
			}
		}
		if((s != -1) && (e != -1)){
			for(int i = s; i<= e; i++){
				_parseNodes.push_back(coveringnode->getChild(i));
			}
		}
	}
}

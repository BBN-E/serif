// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <boost/algorithm/string.hpp> 

#include "Generic/common/ParamReader.h"

#include <sstream>

using namespace std;

const char *Proposition::PRED_TYPE_STRINGS[] = {"verb",
												"copula",
												"modifier",
												"noun",
												"poss",
												"loc",
												"set",
												"name",
												"pronoun",
												"comp"};
const wchar_t *Proposition::PRED_TYPE_WSTRINGS[] = {L"verb",
												L"copula",
												L"modifier",
												L"noun",
												L"poss",
												L"loc",
												L"set",
												L"name",
												L"pronoun",
												L"comp"};

bool Proposition::debug_props = false;

Proposition::Proposition(int ID, PredType predType, int n_args)
		: _ID(ID), _predType(predType), _predHead(0), _negation(0),
		  _modal(0), _particle(0), _adverb(0),
		  _n_args(n_args), _args(_new Argument[n_args])
{
	debug_props = ParamReader::isParamTrue("debug_props");
	if (n_args == 0) {
		throw InternalInconsistencyException(
			"Proposition::Proposition()",
			"Attempt to create proposition with no arguments");
	}
}

Proposition::Proposition(const Proposition &other, int sent_offset, const Parse* parse)
: _predType(other._predType), _statuses(other._statuses), _n_args(other._n_args),
_predHead(NULL), _particle(NULL), _adverb(NULL), _negation(NULL), _modal(NULL)
{
	debug_props = ParamReader::isParamTrue("debug_props");
	_ID = other._ID + sent_offset*MAX_SENTENCE_PROPS;

	if (parse != NULL) {
		// Look up parse nodes if necessary
		if (other._predHead != NULL)
			_predHead = parse->getSynNode(other._predHead->getID());
		if (other._particle != NULL)
			_particle = parse->getSynNode(other._particle->getID());
		if (other._adverb != NULL)
			_adverb = parse->getSynNode(other._adverb->getID());
		if (other._negation != NULL)
			_negation = parse->getSynNode(other._negation->getID());
		if (other._modal != NULL)
			_modal = parse->getSynNode(other._modal->getID());
	} else {
		// Copy links to parse nodes
		_predHead = other._predHead;
		_particle = other._particle;
		_adverb = other._adverb;
		_negation = other._negation;
		_modal = other._modal;
	}

	if (_n_args == 0)
		throw InternalInconsistencyException("Proposition::Proposition", "Attempt to deep copy proposition with no arguments");
	const Proposition* otherPropArg = NULL;
	const SynNode* otherTextArg = NULL;
	_args = _new Argument[_n_args];
	for (int i = 0; i < _n_args; i++) {
		Symbol role = other._args[i].getRoleSym();
		switch (other._args[i].getType()) {
		case Argument::MENTION_ARG:
			_args[i].populateWithMention(role, other._args[i].getMentionIndex());
			break;
		case Argument::PROPOSITION_ARG:
			// Link to the existing proposition; we can't update the pointer until the containing propset has been completely filled
			_args[i].populateWithProposition(role, other._args[i].getProposition());
			break;
		case Argument::TEXT_ARG:
			if (parse != NULL)
				otherTextArg = parse->getSynNode(other._args[i].getNode()->getID());
			else
				otherTextArg = other._args[i].getNode();
			_args[i].populateWithText(role, otherTextArg);
			break;
		}
	}
}

void Proposition::setArg(int i, Argument &arg) {
	if ((unsigned) i < (unsigned) _n_args) {
		_args[i] = arg;
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"Proposition::setArgument()", _n_args, i);
	}
}


const char *Proposition::getPredTypeString(PredType predType) {
	if ((unsigned) predType < (unsigned) N_PRED_TYPES)
		return PRED_TYPE_STRINGS[predType];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"Proposition::getPredTypeString()", N_PRED_TYPES, predType);
}

Proposition::PredType Proposition::getPredTypeFromString(const wchar_t* typeString) {
	//Loop over the possible Mention::Types, since they're not hashed
	for (int type = 0; type < N_PRED_TYPES; type++) {
		if (wcscmp(typeString, PRED_TYPE_WSTRINGS[type])==0)
			return static_cast<Proposition::PredType>(type);
	}
	throw InternalInconsistencyException("Proposition::getPredTypeFromString",
		"Unknown predicate type");
}


Argument *Proposition::getArg(int i) const {
	if ((unsigned) i < (unsigned) _n_args)
		return &_args[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"Proposition::getArg()", _n_args, i);
}

Symbol Proposition::getPredSymbol() const {
	if (_predHead == 0)
		return Symbol();
	else
		return _predHead->getHeadWord();
}


const Mention *Proposition::getMentionOfRole(
	Symbol roleSym, const MentionSet *mentionSet) const
{
	for (int i = 0; i < _n_args; i++) {
		if (_args[i].getType() == Argument::MENTION_ARG &&
			_args[i].getRoleSym() == roleSym)
		{
			return _args[i].getMention(mentionSet);
		}
	}
	
	return 0;
}

Symbol Proposition::getRoleOfMention(const Mention* mention, const MentionSet *mentionSet) const {
	for (int i = 0; i < _n_args; i++) {
		if (_args[i].getType() == Argument::MENTION_ARG) {
			const Mention *ment = _args[i].getMention(mentionSet);
			if (ment->getMentionType() == Mention::LIST) {  // Assign list member mentions the role of their containing list
				const Mention *memberMent = ment->getChild();
				while (memberMent != 0) {
					if (memberMent->getIndex() == mention->getIndex()) {
						return _args[i].getRoleSym();					
					}
					memberMent = memberMent->getNext();
				}
			} else if (_args[i].getMentionIndex() == mention->getIndex()) {
				return _args[i].getRoleSym();
			} 
		}
	}
	return Symbol();
}


const Proposition *Proposition::getPropositionOfRole(Symbol roleSym) const
{
	for (int i = 0; i < _n_args; i++) {
		if (_args[i].getType() == Argument::PROPOSITION_ARG &&
			_args[i].getRoleSym() == roleSym)
		{
			return _args[i].getProposition();
		}
	}
	
	return 0;
}

/**
 * Optional entry point. Check if this proposition is in a cycle in the proposition graph.
 **/
bool Proposition::hasCycle(const SentenceTheory* sentTheory) const {
	// Start checking with an empty seen stack
	std::vector<const Proposition*> seen;
	return hasCycle(sentTheory, seen);
}

/**
 * Primary entry point from pattern matching. Check if following a particular argument
 * through the Proposition graph would get back to this proposition, forming a cycle
 * that could crash Serif with a stack overflow from an infinite loop.
 **/
bool Proposition::hasCycle(const SentenceTheory* sentTheory, const Argument* childArg) const {
	// Initialize stack to contain this proposition as already seen
	std::vector<const Proposition*> seen;
	seen.push_back(this);
	return hasCycle(sentTheory, childArg, seen);
}

bool Proposition::hasCycle(const SentenceTheory* sentTheory, std::vector<const Proposition*> & seen) const {
	// If we've seen this proposition already, we've found a cycle
	if (std::find(seen.begin(), seen.end(), this) != seen.end())
		return true;

	// Mark this proposition as seen
	seen.push_back(this);

	// Recurse through any proposition for which this proposition is the <ref> argument
	if (sentTheory == NULL)
		return false;
	const PropositionSet* propSet = sentTheory->getPropositionSet();
	if (propSet == NULL)
		return false;
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		const Proposition* otherProp = propSet->getProposition(p);
		if (otherProp == NULL)
			continue;
		const Proposition* refProp = otherProp->getPropositionOfRole(Argument::REF_ROLE);
		if (refProp == this) {
			if (otherProp->hasCycle(sentTheory, seen)) {
				return true;
			}
		}
	}

	// Recurse through each argument of this proposition
	for (int i = 0; i < _n_args; i++) {
		// Don't follow <ref>s since for cycle detection we check those backwards
		if (_args[i].getRoleSym() != Argument::REF_ROLE) {
			const Argument* arg = &(_args[i]);
			if (hasCycle(sentTheory, arg, seen)) {
				return true;
			}
		}
	}

	// All paths checked, no cycle detected
	return false;
}

bool Proposition::hasCycle(const SentenceTheory* sentTheory, const Argument* childArg, std::vector<const Proposition*> & seen) const {
	if (childArg == NULL)
		return false;

	// Handle each argument type and recurse
	if (childArg->getType() == Argument::PROPOSITION_ARG) {
		// Check if this child proposition participates in a cycle with this one
		const Proposition* childProp = childArg->getProposition();
		if (childProp == NULL)
			return false;
		return childProp->hasCycle(sentTheory, seen);
	} else if (childArg->getType() == Argument::MENTION_ARG) {
		// Check if this child mention's definitional proposition participates in a cycle with this one
		if (sentTheory == NULL)
			return false;
		const PropositionSet* propSet = sentTheory->getPropositionSet();
		if (propSet == NULL)
			return false;
		const Proposition* defProp = propSet->getDefinition(childArg->getMentionIndex());
		return defProp->hasCycle(sentTheory, seen);
	}

	// Other arguments won't result in a cycle
	return false;
}

void Proposition::dump(ostream &out, int indent) const {
	out << "Proposition " << _ID << ": ";
	if (_predHead != 0)
		out << _predHead->getHeadWord().to_debug_string();
	out << "<" << getPredTypeString(_predType) << ">";

	if (_particle != 0)
		out << "[<particle>:" << _particle->getHeadWord().to_debug_string() << "]";
	if (_adverb != 0)
		out << "[<adverb>:" << _adverb->getHeadWord().to_debug_string() << "]";
	if (_negation != 0)
		out << "[<neg>:" << _negation->getHeadWord().to_debug_string() << "]";
	if (_modal != 0)
		out << "[<modal>:" << _modal->getHeadWord().to_debug_string() << "]";

	out << "(" << _args[0];
	for (int i = 1; i < _n_args; i++)
		out << ", " << _args[i];
	out << ")";
	if (_statuses.size() != 0) {
		out << " -- Status: ";
		BOOST_FOREACH(PropositionStatusAttribute status, _statuses) {
			out << UnicodeUtil::toUTF8StdString(status.toString()) << " ";
		}
	}

}

void Proposition::dump(wostream &wout, int indent, bool onlyHead) const {
	if ( ! onlyHead ) wout << "Proposition " << _ID << ": ";
	if (_predHead != 0)
		wout << _predHead->getHeadWord().to_debug_string();
	wout << "<" << getPredTypeString(_predType) << ">";

	if (_particle != 0)
		wout << "[<particle>:" << _particle->getHeadWord().to_debug_string() << "]";
	if (_adverb != 0)
		wout << "[<adverb>:" << _adverb->getHeadWord().to_debug_string() << "]";
	if (_negation != 0)
		wout << "[<neg>:" << _negation->getHeadWord().to_debug_string() << "]";
	if (_modal != 0)
		wout << "[<modal>:" << _modal->getHeadWord().to_debug_string() << "]";
	if ( ! onlyHead ) {
		wout << "(" << _args[0].toString();
		for (int i = 1; i < _n_args; i++)
			wout << ", " << _args[i].toString();
		wout << ")";
	}
}


void Proposition::dump(UTF8OutputStream &out, int indent) const {
	out << L"Proposition " << _ID << L": ";
	if (_predHead != 0)
		out << _predHead->getHeadWord().to_string();
	out << L"<" << getPredTypeString(_predType) << L">";

	if (_particle != 0)
		out << L"[<particle>:" << _particle->getHeadWord().to_string() << L"]";
	if (_adverb != 0)
		out << L"[<adverb>:" << _adverb->getHeadWord().to_string() << L"]";
	if (_negation != 0)
		out << L"[<neg>:" << _negation->getHeadWord().to_string() << L"]";
	if (_modal != 0)
		out << L"[<modal>:" << _modal->getHeadWord().to_string() << L"]";

	out << L"(" << _args[0];
	for (int i = 1; i < _n_args; i++)
		out << L", " << _args[i];
	out << L")";
}

std::wstring Proposition::toString(int indent) const {
	std::wstring str = L"Proposition ";
	wchar_t wc[100];
	swprintf(wc, 100, L"%d", _ID);
	str += wc;
	str += L": ";
	if (_predHead != 0)
		str += _predHead->getHeadWord().to_string();

	wstringstream temp;
	temp << L"<" << getPredTypeString( _predType ) << L">";
	str += temp.str();


	if (_particle != 0) {
		str += L"[<particle>:"; 
		str += _particle->getHeadWord().to_string(); 
		str += L"]";
	}
	if (_adverb != 0) {
		str += L"[<adverb>:"; 
		str += _adverb->getHeadWord().to_string(); 
		str += L"]";
	}
	if (_negation != 0) {
		str += L"[<neg>:"; 
		str += _negation->getHeadWord().to_string(); 
		str += L"]";
	}
	if (_modal != 0) {
		str += L"[<modal>:"; 
		str += _modal->getHeadWord().to_string(); 
		str += L"]";
	}

	str += L"("; 
	str += _args[0].toString();
	for (int i = 1; i < _n_args; i++) {
		str += L", "; 
		str += _args[i].toString();
	}
	str += L")";

	return str;

}

std::string Proposition::toDebugString(int indent) const {
	if (!debug_props) {
		return "enable debug_props parameter to debug props!";
	}
	std::string str = "Proposition ";
	char c[100];
	sprintf(c, "%d", _ID);
	str += c;
	str += ": ";
	if (_predHead != 0)
		str += _predHead->getHeadWord().to_debug_string();

	if (_particle != 0) {
		str += "[<particle>:"; 
		str += _particle->getHeadWord().to_debug_string(); 
		str += "]";
	}
	if (_adverb != 0) {
		str += "[<adverb>:"; 
		str += _adverb->getHeadWord().to_debug_string(); 
		str += "]";
	}
	if (_negation != 0) {
		str += "[<neg>:"; 
		str += _negation->getHeadWord().to_debug_string(); 
		str += "]";
	}
	if (_modal != 0) {
		str += "[<modal>:"; 
		str += _modal->getHeadWord().to_debug_string(); 
		str += "]";
	}

	str += "("; 
	str += _args[0].toDebugString();
	for (int i = 1; i < _n_args; i++) {
		str += ", "; 
		str += _args[i].toDebugString();
	}
	str += ")";

	return str;

}

std::wstring Proposition::getTerminalString(void) const {
	//Make sure we have the highest node in the proposition
	const SynNode* highestHeadNode = _predHead->getHighestHead();

	//Get the terminal tokens
	int nTerminals = highestHeadNode->getNTerminals();
	Symbol* terminalSymbols = _new Symbol[nTerminals];
	highestHeadNode->getTerminalSymbols(terminalSymbols, nTerminals);

	//Generate a flat string
	std::wstringstream spanStream;
	for (int i = 0; i < nTerminals; i++) {
		spanStream << terminalSymbols[i].to_string();
		if (i != nTerminals - 1)
			spanStream << " ";
	}
	delete[] terminalSymbols;

	//Done
	return spanStream.str();
}

void Proposition::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	for (int i = 0; i < _n_args; i++)
		_args[i].updateObjectIDTable();
}

#define BEGIN_PROPOSITION (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::PropositionOffset))
#define BEGIN_PROPOSITIONARGS (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::PropositionArgsOffset))

void Proposition::saveState(StateSaver *stateSaver) const {
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_PROPOSITION, this);
	} else {
		stateSaver->beginList(L"Proposition", this);
	}

	stateSaver->saveInteger(_ID);
	stateSaver->saveInteger(_predType);
	stateSaver->savePointer(_predHead);

	stateSaver->savePointer(_particle);
	stateSaver->savePointer(_adverb);
	stateSaver->savePointer(_negation);
	stateSaver->savePointer(_modal);

	// New in v1.11
	stateSaver->beginList(L"Proposition::_status", this);
	stateSaver->saveInteger(_statuses.size());
	BOOST_FOREACH(PropositionStatusAttribute status, _statuses) {
		stateSaver->saveInteger(status.toInt());
	}
	stateSaver->endList();

	stateSaver->saveInteger(_n_args);
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_PROPOSITIONARGS, this);
	} else {
		stateSaver->beginList(L"Proposition::_args", this);
	}
	for (int i = 0; i < _n_args; i++)
		_args[i].saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

Proposition::Proposition(StateLoader *stateLoader)
{
	//Use the integer replacement for "Proposition" if the state file was compressed
	int id = stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_PROPOSITION : L"Proposition");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	//if (stateLoader->_stateByteBuffer)
	//	std::cerr << "  prop ID: " << stateLoader->_stateByteBuffer->peekInt() << std::endl;
    _ID = stateLoader->loadInteger();
	//if (stateLoader->_stateByteBuffer)
	//	std::cerr << "  prop predType: " << stateLoader->_stateByteBuffer->peekInt() << std::endl;
    _predType = static_cast<PredType>(stateLoader->loadInteger());

	_predHead = static_cast<const SynNode *>(stateLoader->loadPointer());
	_particle = static_cast<const SynNode *>(stateLoader->loadPointer());
	_adverb = static_cast<const SynNode *>(stateLoader->loadPointer());
	_negation = static_cast<const SynNode *>(stateLoader->loadPointer());
	_modal = static_cast<const SynNode *>(stateLoader->loadPointer());

	if (stateLoader->getVersion() >= std::make_pair(1,11)) {
		stateLoader->beginList(L"Proposition::_status");
		int n_statuses = stateLoader->loadInteger();
		for (int i = 0; i < n_statuses; i++) {
			addStatus(PropositionStatusAttribute::getFromInt(stateLoader->loadInteger()));
		}
		stateLoader->endList();
	} else if (stateLoader->getVersion() >= std::make_pair(1,8)) {
		// Just add the one status we do have to our set
		addStatus(PropositionStatusAttribute::getFromInt(stateLoader->loadInteger()));
	} 

	//if (stateLoader->_stateByteBuffer)
	//	std::cerr << "  prop n_args: " << stateLoader->_stateByteBuffer->peekInt() << std::endl;
	_n_args = stateLoader->loadInteger();
	_args = _new Argument[_n_args];
	stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_PROPOSITIONARGS : L"Proposition::_args");
	for (int i = 0; i < _n_args; i++)
		_args[i].loadState(stateLoader);
	stateLoader->endList();

	stateLoader->endList();
}

void Proposition::resolvePointers(StateLoader * stateLoader) {
	_predHead = static_cast<const SynNode *>(
						stateLoader->getObjectPointerTable().getPointer(_predHead));
	_particle = static_cast<const SynNode *>(
						stateLoader->getObjectPointerTable().getPointer(_particle));
	_adverb = static_cast<const SynNode *>(
						stateLoader->getObjectPointerTable().getPointer(_adverb));
	_negation = static_cast<const SynNode *>(
						stateLoader->getObjectPointerTable().getPointer(_negation));
	_modal = static_cast<const SynNode *>(
						stateLoader->getObjectPointerTable().getPointer(_modal));

	for (int i = 0; i < _n_args; i++)
		_args[i].resolvePointers(stateLoader);
}

void Proposition::getStartEndTokenProposition(const MentionSet* mentionSet, int& startToken, int& endToken) const{			
	if(_predHead){
		const SynNode* propHead = _predHead->getHighestHead();
		startToken = propHead->getStartToken();
		endToken = propHead->getEndToken();
	}
	for(int arg_n = 0; arg_n < _n_args; arg_n++){
		Argument* arg = &_args[arg_n];
		int as = arg->getStartToken(mentionSet);
		if (as < startToken){
			startToken = as;
		}		
		int es = arg->getEndToken(mentionSet);
		if (es > endToken){
			endToken = es;
		}
	}	
	//... and modals, particles, adverbs and negation
	if(_adverb && _adverb->getStartToken() < startToken){
		startToken =  _adverb->getStartToken(); 
	}
	if(_adverb && _adverb->getEndToken() > endToken){
		endToken =  _adverb->getEndToken(); 
	}			
	if(_modal && _modal->getStartToken() < startToken){
		startToken =  _modal->getStartToken();
	}
	if(_modal && _modal->getEndToken() > endToken){
		endToken =  _modal->getEndToken(); 
	}
	if(_particle && _particle->getStartToken() < startToken){
		startToken =  _particle->getStartToken(); 
	}
	if(_particle && _particle->getEndToken() > endToken){
		endToken =  _particle->getEndToken(); 
	}			
	if(_negation && _negation->getStartToken() < startToken){
		startToken = _negation->getStartToken(); 
	}
	if(_negation && _negation->getEndToken() > endToken){
		endToken = _negation->getEndToken(); 
	}
	return;
}

const wchar_t* Proposition::XMLIdentifierPrefix() const {
	return L"prop";
}

void Proposition::saveXML(SerifXML::XMLTheoryElement propositionElem, const Theory *context) const {
	using namespace SerifXML;
	const MentionSet *mentionSet = dynamic_cast<const MentionSet*>(context);
	if (context == 0)
		throw InternalInconsistencyException("Proposition::saveXML()", "Expected context to be a MentionSet");

	propositionElem.setAttribute(X_type, getPredTypeString(_predType));
	std::wstring status_string;
	BOOST_FOREACH(PropositionStatusAttribute status, _statuses) {
		if (status_string != L"")
			status_string += L" ";
		status_string += status.toString();
	}
	if (status_string != L"")
		propositionElem.setAttribute(X_status, status_string);

	if (getPredHead())
		propositionElem.saveTheoryPointer(X_head_id, getPredHead());
	if (getParticle())
		propositionElem.saveTheoryPointer(X_particle_id, getParticle());
	if (getAdverb())
		propositionElem.saveTheoryPointer(X_adverb_id, getAdverb());
	if (getNegation())
		propositionElem.saveTheoryPointer(X_negation_id, getNegation());
	if (getModal())
		propositionElem.saveTheoryPointer(X_modal_id, getModal());
	for (int i = 0; i < _n_args; ++i) {
		propositionElem.saveChildTheory(X_Argument, &_args[i], mentionSet);
	}
}

Proposition::Proposition(SerifXML::XMLTheoryElement propElem, int prop_id)
{
	debug_props = ParamReader::isParamTrue("debug_props");

	using namespace SerifXML;
	propElem.loadId(this);
	_ID = prop_id;
	_predType = getPredTypeFromString(propElem.getAttribute<std::wstring>(X_type).c_str());

	_predHead = propElem.loadOptionalTheoryPointer<SynNode>(X_head_id);
	_particle = propElem.loadOptionalTheoryPointer<SynNode>(X_particle_id);
	_adverb = propElem.loadOptionalTheoryPointer<SynNode>(X_adverb_id);
	_negation = propElem.loadOptionalTheoryPointer<SynNode>(X_negation_id);
	_modal = propElem.loadOptionalTheoryPointer<SynNode>(X_modal_id);
	std::wstring temp_status_string = propElem.getAttribute<std::wstring>(X_status, L"");
	if (temp_status_string != L"") {
		std::vector<std::wstring> statuses;
		boost::split(statuses, temp_status_string, boost::is_any_of(L" "));
		BOOST_FOREACH(std::wstring status, statuses) {
			addStatus(PropositionStatusAttribute::getFromString(status.c_str()));
		}
	}

	XMLTheoryElementList argElems = propElem.getChildElementsByTagName(X_Argument);
	_n_args = static_cast<int>(argElems.size());
	_args = _new Argument[_n_args];
	for (int i=0; i<_n_args; ++i)
		_args[i].loadXML(argElems[i]);
}

void Proposition::resolvePointers(SerifXML::XMLTheoryElement propElem) {
	using namespace SerifXML;
	// We assume that the order of the xml children is unchanged from when
	// we loaded it with the Proposition deserializing constructor.
	XMLTheoryElementList argElems = propElem.getChildElementsByTagName(X_Argument);
	for (int i=0; i<_n_args; ++i)
		_args[i].resolvePointers(argElems[i]);
}

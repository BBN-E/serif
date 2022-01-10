// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/theories/SentenceTheory.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/parse/xx_STags.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/theories/DependencyParseTheory.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/SentenceSubtheory.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"

#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp>

std::set<Symbol> SentenceTheory::_temporalWords;

SentenceTheory::SentenceTheory(const Sentence *sentence, Symbol primaryParse, Symbol docID)
: _sentence(sentence), _lex(0)
{
	setPrimaryParse(primaryParse);
	_docID = docID;

	for (int i = 0; i < N_SUBTHEORY_TYPES; i++)
		_subtheories[i] = 0;
}

SentenceTheory::SentenceTheory(const SentenceTheory &other)
: _sentence(other._sentence), _lex(0)
{
	_primary_parse = other._primary_parse;
	_docID = other._docID;
	for (int i = 0; i < N_SUBTHEORY_TYPES; i++) {
		_subtheories[i] = other._subtheories[i];

		if (_subtheories[i] != 0)
			_subtheories[i]->gainReference();
	}
}

SentenceTheory::~SentenceTheory() {
	for (int i = 0; i < N_SUBTHEORY_TYPES; i++) {
		if (_subtheories[i])
			_subtheories[i]->loseReference();
	}
}

void SentenceTheory::setPrimaryParse(Symbol primary_parse) {
	// should we provide a default value if primary_parse.is_null()?

	if (primary_parse != Symbol(L"full_parse") && 
		primary_parse != Symbol(L"npchunk_parse") && 
		primary_parse != Symbol(L"dependency_parse"))
	{
		throw InternalInconsistencyException("SentenceTheory::setPrimaryParse",
			"Bad primary_parse symbol");
	}

	_primary_parse = primary_parse;
}

float SentenceTheory::getScore() const {
	// elf note: while the following heuristic is more robust
	// than "1.0", it is still not correct and should be changed.

	float score = 0;
	if (_subtheories[TOKEN_SUBTHEORY] != 0) 
		score += getTokenSequence()->getScore();
	if (_subtheories[NAME_SUBTHEORY] != 0)
		score += getNameTheory()->getScore();
	if (_subtheories[PARSE_SUBTHEORY] != 0){
		//if you run to parsing and have np chunk defined as the primary parse this
		//will die //mrf
		//score += getPrimaryParse()->getScore();
		score += getFullParse()->getScore();

		// Now we want to avoid the system attaching temporals to the subject as opposed to the verb
		// This is the "South korea today did something" problem.
		score += scoreParseForBadTemporalAttachments(getFullParse());
	}

	if (_subtheories[MENTION_SUBTHEORY] != 0) {
		// name score component comes from this (or from name theories) 
		// once it's implemented
		//score += getMentionSet()->getDescScore();

		// Here we prefer appositives
		score += SymbolUtilities::getDescAdjScore(getMentionSet());

	}

	/* FOR NBEST
	if (_subtheories[RELATION_SUBTHEORY] != 0) {
	score += getRelMentionSet()->getNRelMentions() * 10;
	}
	*/

	// propositions, relations, events not scored

	if (_subtheories[ENTITY_SUBTHEORY] != 0) {
		score += getEntitySet()->getScore();
	}
	//std::cout << "\n" << _sentence->getSentNumber() << " " << this << " score is " << score << "\n";
	return score;
}

SentenceSubtheory *SentenceTheory::getSubtheory(int i) const {
	if ((unsigned) i < (unsigned) N_SUBTHEORY_TYPES)
		return _subtheories[i];
	else
		return 0;
}

TokenSequence *   SentenceTheory::getTokenSequence() const {
	return static_cast<TokenSequence *>(_subtheories[TOKEN_SUBTHEORY]); 
}
PartOfSpeechSequence * SentenceTheory::getPartOfSpeechSequence() const {
	return static_cast<PartOfSpeechSequence *>(_subtheories[POS_SUBTHEORY]); 
}
NameTheory *      SentenceTheory::getNameTheory() const {
	return static_cast<NameTheory *>(_subtheories[NAME_SUBTHEORY]);
}
NestedNameTheory* SentenceTheory::getNestedNameTheory() const {
	return static_cast<NestedNameTheory *>(_subtheories[NESTED_NAME_SUBTHEORY]);
}
Parse *           SentenceTheory::getFullParse() const {
	return static_cast<Parse *>(_subtheories[PARSE_SUBTHEORY]);
}
Parse *           SentenceTheory::getPrimaryParse() const {
	if(_primary_parse == Symbol(L"full_parse")){
		return getFullParse();
	}
	else if(_primary_parse == Symbol(L"npchunk_parse")){
		return getNPChunkParse();
	}
	else if(_primary_parse == Symbol(L"dependency_parse")){
		return getNPChunkParse();
	}
	else return 0;
}
Parse *           SentenceTheory::getNPChunkParse() const {
	if (getNPChunkTheory() != 0)
		return getNPChunkTheory()->getParse();
	else return 0;
}
Parse *           SentenceTheory::getDependencyParse() const {
	if (getDependencyParseTheory() != 0)
		return getDependencyParseTheory()->getParse();
	else return 0;
}
MentionSet *      SentenceTheory::getMentionSet() const {
	return static_cast<MentionSet *>(_subtheories[MENTION_SUBTHEORY]);
}
ActorMentionSet * SentenceTheory::getActorMentionSet() const {
	return static_cast<ActorMentionSet *>(_subtheories[ACTOR_MENTION_SET_SUBTHEORY]);
}
ValueMentionSet * SentenceTheory::getValueMentionSet() const {
	return static_cast<ValueMentionSet *>(_subtheories[VALUE_SUBTHEORY]);
}
PropositionSet *  SentenceTheory::getPropositionSet() const {
	return static_cast<PropositionSet *>(_subtheories[PROPOSITION_SUBTHEORY]);
}
EntitySet *       SentenceTheory::getEntitySet() const {
	return static_cast<EntitySet *>(_subtheories[ENTITY_SUBTHEORY]);
}
RelMentionSet *   SentenceTheory::getRelMentionSet() const {
	return static_cast<RelMentionSet *>(_subtheories[RELATION_SUBTHEORY]);
}
EventMentionSet * SentenceTheory::getEventMentionSet() const {
	return static_cast<EventMentionSet *>(_subtheories[EVENT_SUBTHEORY]);
}
NPChunkTheory*    SentenceTheory::getNPChunkTheory() const {
	return static_cast<NPChunkTheory *>(_subtheories[NPCHUNK_SUBTHEORY]);
}
DependencyParseTheory*    SentenceTheory::getDependencyParseTheory() const {
	return static_cast<DependencyParseTheory *>(_subtheories[DEPENDENCY_PARSE_SUBTHEORY]);
}

Symbol SentenceTheory::getDocID() const {
	return _docID;
}

void SentenceTheory::adoptSubtheory(SubtheoryType type,
									SentenceSubtheory *newSubtheory)
{
	// If we're replacing a subtheory that other subtheories can point
	// to, then update those pointers.
	if ((type == TOKEN_SUBTHEORY) && (_subtheories[type] != 0)) {
		if (getPartOfSpeechSequence() || getNameTheory() ||
			getValueMentionSet() || getNPChunkTheory() ||
			getDependencyParseTheory() || getFullParse())
			throw InternalInconsistencyException("SentenceTheory::adoptSubtheory",
				"TokenSequence can not be replaced once the PartOfSpeechSequence, "
				"NameTheory, ValueMentioNSet, NPChunkTheory, DependencyParseTheory "
				"or Parse is set.");
	}
	if ((type == PARSE_SUBTHEORY) && (_subtheories[type] != 0)) {
		if (getMentionSet() || getPropositionSet() || getEventMentionSet())
			throw InternalInconsistencyException("SentenceTheory::adoptSubtheory",
				"Parse can not be replaced once the MentionSet, "
				"PropositionSet, or EventMentionSet is set.");
	}
	if ((type == MENTION_SUBTHEORY) && (_subtheories[type] != 0)) {
		const MentionSet* mentionSet = dynamic_cast<MentionSet*>(newSubtheory);
		if ((newSubtheory != 0) && (mentionSet == 0))
			throw InternalInconsistencyException("SentenceTheory::adoptSubtheory",
				"Expected newSubtheory to be a MentionSet!");
		if (getPropositionSet())
			getPropositionSet()->replaceMentionSet(mentionSet);
	}

	if (_subtheories[type] != 0) 
		_subtheories[type]->loseReference();
	_subtheories[type]= newSubtheory;
	if (newSubtheory != 0)
		_subtheories[type]->gainReference();
}


void SentenceTheory::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Sentence Theory "
		<< "(document " << _docID.to_debug_string() << " sentence " << _sentence->getSentNumber() << ", score = " << getScore() << "):";

	if (getTokenSequence() != 0) {
		out << newline << "- ";
		getTokenSequence()->dump(out, indent + 2);
	}
	if (getPartOfSpeechSequence() != 0 && !getPartOfSpeechSequence()->isEmpty()) {
		out << newline << "- ";
		getPartOfSpeechSequence()->dump(out, indent + 2);
	}
	if (getFullParse() != 0) {
		out << newline << "- ";
		getFullParse()->dump(out, indent + 2);
	}
	if (getNPChunkTheory() != 0) {
		out << newline << "- ";
		getNPChunkTheory()->dump(out, indent + 2);
	}
	if (getDependencyParseTheory() != 0) {
		out << newline << "- ";
		getDependencyParseTheory()->dump(out, indent + 2);
	}
	if (getMentionSet() != 0) {
		out << newline << "- ";
		getMentionSet()->dump(out, indent + 2);
	}
	if (getValueMentionSet() != 0) {
		out << newline << "- ";
		getValueMentionSet()->dump(out, indent + 2);
	}
	if (getPropositionSet() != 0) {
		out << newline << "- ";
		getPropositionSet()->dump(out, indent + 2);
	}
	if (getEntitySet() != 0) {
		out << newline << "- ";
		getEntitySet()->dump(out, indent + 2);
	}
	if (getRelMentionSet() != 0) {
		out << newline << "- ";
		getRelMentionSet()->dump(out, indent + 2);
	}
	if (getEventMentionSet() != 0) {
		out << newline << "- ";
		getEventMentionSet()->dump(out, indent + 2);
	}

	delete[] newline;
}


void SentenceTheory::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void SentenceTheory::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"SentenceTheory", this);
	stateSaver->saveSymbol(_docID);
	stateSaver->saveSymbol(_primary_parse);

	stateSaver->beginList(L"SentenceTheory::_subtheories");
	for (int i = 0; i < N_SUBTHEORY_TYPES; i++)
		stateSaver->savePointer(_subtheories[i]);
	stateSaver->endList();

	stateSaver->endList();
}

SentenceTheory::SentenceTheory(StateLoader *stateLoader,
							   const Sentence *sentence)
							   : _sentence(sentence)
{
	int id = stateLoader->beginList(L"SentenceTheory");
	_docID = stateLoader->loadSymbol();
	_primary_parse = stateLoader->loadSymbol();
	stateLoader->getObjectPointerTable().addPointer(id, this);

	stateLoader->beginList(L"SentenceTheory::_subtheories");
	for (int i = 0; i < N_SUBTHEORY_TYPES; i++)
		_subtheories[i] = (SentenceSubtheory *) stateLoader->loadPointer();
	stateLoader->endList();

	stateLoader->endList();
}

void SentenceTheory::resolvePointers(StateLoader * stateLoader) {
	for (int i = 0; i < N_SUBTHEORY_TYPES; i++) {
		_subtheories[i] =
			(SentenceSubtheory *) stateLoader->getObjectPointerTable().getPointer(_subtheories[i]);

		if (_subtheories[i] != 0)
			_subtheories[i]->gainReference();
	}
}

// If you want to see the biggest gains from this, run with the default values
// for parser_lambda and parser_max_entries_per_cell.  If these are set to their
// "fast-parsing" values (-1 and 5), then the diversity in the parses is not so good.
float SentenceTheory::scoreParseForBadTemporalAttachments(Parse* p) const {
	if (_temporalWords.size() == 0) {
		_temporalWords.insert(Symbol(L"yesterday"));
		_temporalWords.insert(Symbol(L"today"));
		_temporalWords.insert(Symbol(L"sunday"));
		_temporalWords.insert(Symbol(L"monday"));
		_temporalWords.insert(Symbol(L"tuesday"));
		_temporalWords.insert(Symbol(L"wednesday"));
		_temporalWords.insert(Symbol(L"thursday"));
		_temporalWords.insert(Symbol(L"friday"));
		_temporalWords.insert(Symbol(L"saturday"));
	}

	float score = 0.0;
	std::vector<const SynNode*> nodes;
	p->getRoot()->getAllTerminalNodes(nodes);
	//std::cout << "SentenceTheory::scoreParseForBadTemporalAttachments: Found " << nodes.size() << " terminal nodes\n";
	for (size_t i = 0; i < (nodes.size()-1); i++) { // -1 is to make sure we can have a following verb
		const SynNode* node = nodes[i];
		
		// Do we belong to our set of suspicious words?  Are we not the first word?
		if (_temporalWords.find(node->getTag()) != _temporalWords.end() &&
			node->getStartToken() != 0) {

			// Ascend up any unary projections
			while (node->getParent() && node->getParent()->getNChildren() == 1) {
				node = node->getParent();
			}
	
			// Make sure we are immediately following a noun and our parent is a noun phrase
			const SynNode* prevTerminal = p->getRoot()->getCoveringNodeFromTokenSpan(node->getStartToken()-1, node->getStartToken()-1);
			if (prevTerminal->getParent() && prevTerminal->getParent()->getTag().to_debug_string()[0] == L'N' &&
				node->getParent() && node->getParent()->getTag().to_debug_string()[0] == L'N') {

				// Make sure a verb follows us
				bool foundAFollowingVerb = false;
				for (size_t j = i+1; j < nodes.size(); j++) {
					const SynNode* candidate = nodes[j];
					if (candidate->getParent() && candidate->getParent()->getTag().to_debug_string()[0] == L'V') {
						foundAFollowingVerb = true;
						break;
					}
				}
				if (foundAFollowingVerb) {
					//std::wcout << L"DATELIKE:\n" << node->toTextString() << L"\n";
					score -= 1000;
				}
			}
		}
	}
	return score;
}

const wchar_t* SentenceTheory::XMLIdentifierPrefix() const {
	return L"sent";
}

void SentenceTheory::saveXML(SerifXML::XMLTheoryElement sentenceElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("SentenceTheory::saveXML", "Expected context to be NULL");

	if (getTokenSequence())
		sentenceElem.saveTheoryPointer(X_token_sequence_id, getTokenSequence());
	if (getPartOfSpeechSequence())
		sentenceElem.saveTheoryPointer(X_part_of_speech_sequence_id, getPartOfSpeechSequence());
	if (getNameTheory())
		sentenceElem.saveTheoryPointer(X_name_theory_id, getNameTheory());
	if (getNestedNameTheory())
		sentenceElem.saveTheoryPointer(X_nested_name_theory_id, getNestedNameTheory());
	if (getValueMentionSet())
		sentenceElem.saveTheoryPointer(X_value_mention_set_id, getValueMentionSet());
	if (getNPChunkTheory())
		sentenceElem.saveTheoryPointer(X_np_chunk_theory_id, getNPChunkTheory());
	if (getDependencyParseTheory())
		sentenceElem.saveTheoryPointer(X_dependency_parse_id, getDependencyParseTheory());
	if (getFullParse())
		sentenceElem.saveTheoryPointer(X_parse_id, getFullParse());
	if (getMentionSet())
		sentenceElem.saveTheoryPointer(X_mention_set_id, getMentionSet());
	if (getActorMentionSet())
		sentenceElem.saveTheoryPointer(X_actor_mention_set_id, getActorMentionSet());
	if (getPropositionSet())
		sentenceElem.saveTheoryPointer(X_proposition_set_id, getPropositionSet());
	if (getRelMentionSet())
		sentenceElem.saveTheoryPointer(X_rel_mention_set_id, getRelMentionSet());
	if (getEventMentionSet())
		sentenceElem.saveTheoryPointer(X_event_mention_set_id, getEventMentionSet());
	// We intentionally don't serialize the entity mention set.

	// Additional attributes.
	if (!getPrimaryParseSym().is_null())
		sentenceElem.setAttribute(X_primary_parse, getPrimaryParseSym());
}

SentenceTheory::SentenceTheory(SerifXML::XMLTheoryElement elem, const Sentence *sentence, const Symbol &docID)
: _sentence(sentence), _docID(docID), _lex(0)
{
	using namespace SerifXML;
	elem.loadId(this);
	for (int i = 0; i < N_SUBTHEORY_TYPES; i++)
		_subtheories[i] = 0;

	adoptSubtheory(TOKEN_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<TokenSequence>(X_token_sequence_id));
	adoptSubtheory(POS_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<PartOfSpeechSequence>(X_part_of_speech_sequence_id));
	adoptSubtheory(NAME_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<NameTheory>(X_name_theory_id));
	adoptSubtheory(NESTED_NAME_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<NestedNameTheory>(X_nested_name_theory_id));
	adoptSubtheory(VALUE_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<ValueMentionSet>(X_value_mention_set_id));
	adoptSubtheory(NPCHUNK_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<NPChunkTheory>(X_np_chunk_theory_id));
	adoptSubtheory(DEPENDENCY_PARSE_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<DependencyParseTheory>(X_dependency_parse_id));
	adoptSubtheory(PARSE_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<Parse>(X_parse_id));
	adoptSubtheory(MENTION_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<MentionSet>(X_mention_set_id));
	adoptSubtheory(ACTOR_MENTION_SET_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<ActorMentionSet>(X_actor_mention_set_id));
	adoptSubtheory(PROPOSITION_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<PropositionSet>(X_proposition_set_id));
	adoptSubtheory(RELATION_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<RelMentionSet>(X_rel_mention_set_id));
	adoptSubtheory(EVENT_SUBTHEORY, elem.loadOptionalNonConstTheoryPointer<EventMentionSet>(X_event_mention_set_id));

	// entity subtheories get reconstructed in DocTheory constructor
	//adoptSubtheory(ENTITY_SUBTHEORY, elem.loadTheoryPointer(ENTITY));

	Symbol primary_parse_symbol = elem.getAttribute<Symbol>(X_primary_parse, Symbol(L"full_parse"));
	if ((primary_parse_symbol != Symbol(L"npchunk_parse")) &&
		(primary_parse_symbol != Symbol(L"full_parse")) &&
		(primary_parse_symbol != Symbol(L"dependency_parse")))
		elem.reportLoadError("Bad value for primary_parse_symbol");
	setPrimaryParse(primary_parse_symbol);
}

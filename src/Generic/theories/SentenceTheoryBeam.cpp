// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/theories/SentenceTheoryBeam.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"

#include "Generic/theories/LexicalTokenSequence.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/theories/DependencyParseTheory.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLIdMap.h"
#include "Generic/state/XMLSerializedDocTheory.h"

#include <boost/foreach.hpp>


SentenceTheoryBeam::SentenceTheoryBeam(const Sentence *sentence, size_t width)
	: _sentence(sentence), _n_theories(0), _beam_width(static_cast<int>(width)),
	  _n_unique_subtheories(0), _unique_subtheories_up_to_date(true)
{
	_theories = _new SentenceTheory*[_beam_width];
	for (int i = 0; i < _beam_width; i++)
		_theories[i] = 0;
	_uniqueSubtheories = _new const SentenceSubtheory*[
		_beam_width*SentenceTheory::N_SUBTHEORY_TYPES];
}

SentenceTheoryBeam::~SentenceTheoryBeam() {
	for (int i = 0; i < _n_theories; i++)
		delete _theories[i];
	delete [] _theories;
	delete [] _uniqueSubtheories;
}

void SentenceTheoryBeam::addTheory(SentenceTheory *theory) {
	_unique_subtheories_up_to_date = false;

	//std::cout << "Adding theory to sentence " << theory->getSentNumber() << " with score " << theory->getScore() << " and beam width " << _beam_width << "\n";
	for (int i = 0; i < _beam_width; i++) {
		if (_theories[i] == 0) {
			// Here's a blank entry in the theory array, so put the new
			// theory there.
			_theories[i] = theory;
			_n_theories++;
			
			return;
		} else if (_theories[i]->getScore() < theory->getScore()) {
			// If we encountered a theory with a worse score, put the new
			// theory there in its slot, and move the rest of the
			// theories down one.

			// If there's a theory at the end of the list, it falls off and
			// gets deleted.
			if (_theories[_beam_width - 1]) {
				delete _theories[_beam_width - 1];
				_n_theories--;
			}

			// Bump down all theories from i down:
			for (int j = _beam_width - 1; j > i; j--) {
				_theories[j] = _theories[j-1];
			}

			// Put the new theory in the hole we just created.
			_theories[i] = theory;
			_n_theories++;

			return;
		}
	}

	// if that loop exited, then the theory was wasn't good enough to make
	// the cut
	// JCS 4/26/04 - If it didn't get added, we need to delete it.
	delete theory;
}

SentenceTheory *SentenceTheoryBeam::getTheory(int i) const {
	if (i >= _n_theories) {
		throw InternalInconsistencyException::arrayIndexException(
			"SentenceTheoryBeam::getTheory()", _n_theories, i);
	}
	return _theories[i];
}

SentenceTheory *SentenceTheoryBeam::getBestTheory() {
	if (_n_theories > 0)
		return _theories[0];
	else
		return 0;
}

SentenceTheory *SentenceTheoryBeam::extractBestTheory() {
	if (_n_theories == 0) {
		throw InternalInconsistencyException(
			"SentenceTheoryBeam::extractBestTheory()",
			"Attempt to extract best theory from empty beam");
	}

	for (int i = 1; i < _n_theories; i++)
		delete _theories[i];

	_n_theories = 0;

	return _theories[0];
}


void SentenceTheoryBeam::ensureUniqueSubtheoriesUpToDate() const {
	/* Sometimes subtheories change behind our back -- E.g., entity finding can change
	 * the value of the mentionset subtheory! So comment the following out for now,
	 * until I figure out a better way to deal with it: */
	//if (_unique_subtheories_up_to_date == true)
	//	return;

	_n_unique_subtheories = 0;

	for (int theory_no = 0; theory_no < _n_theories; theory_no++) {
		SentenceTheory *theory = _theories[theory_no];

		for (int subtheory_no = 0;
			 subtheory_no < SentenceTheory::N_SUBTHEORY_TYPES;
			 subtheory_no++)
		{
			const SentenceSubtheory *subtheory =
				theory->getSubtheory(subtheory_no);

			if (subtheory == 0)
				continue;
			
			int i;
			for (i = 0; i < _n_unique_subtheories; i++) {
				if (_uniqueSubtheories[i] == subtheory)
					break;
			}
			if (i == _n_unique_subtheories) {
				_uniqueSubtheories[i] = subtheory;
				_n_unique_subtheories++;
			}
		}

		SentenceSubtheory *subtheory = theory->getSubtheory(SentenceTheory::ACTOR_MENTION_SET_SUBTHEORY);
		if (subtheory != 0) {
			ActorMentionSet *actorMentionSetTheory = static_cast<ActorMentionSet *>(subtheory);
			actorMentionSetTheory->setSentenceTheories(theory);
		}
	}

	_unique_subtheories_up_to_date = true;
}


void SentenceTheoryBeam::updateObjectIDTable() const {
	ensureUniqueSubtheoriesUpToDate();

	// We add our self of course
	ObjectIDTable::addObject(this);

	// Now add all theories
	for (int i = 0; i < _n_theories; i++)
		_theories[i]->updateObjectIDTable();

	// Now add all subtheories
	ensureUniqueSubtheoriesUpToDate();
	for (int j = 0; j < _n_unique_subtheories; j++)
		_uniqueSubtheories[j]->updateObjectIDTable();
}

void SentenceTheoryBeam::saveState(StateSaver *stateSaver) const {
	ensureUniqueSubtheoriesUpToDate();

	stateSaver->beginList(L"SentenceTheoryBeam", this);

	stateSaver->saveInteger(_beam_width);

	stateSaver->saveInteger(_n_theories);
	stateSaver->beginList(L"SentenceTheoryBeam::_theories");
	for (int i = 0; i < _n_theories; i++)
		_theories[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->saveInteger(_n_unique_subtheories);
	stateSaver->beginList(L"SentenceTheoryBeam::_uniqueSubtheories");
	for (int k = 0; k < _n_unique_subtheories; k++)
		saveSubtheory(_uniqueSubtheories[k], stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

void SentenceTheoryBeam::saveSubtheory(const SentenceSubtheory *subtheory,
									   StateSaver *stateSaver) const
{
	stateSaver->saveInteger(subtheory->getSubtheoryType());
	subtheory->saveState(stateSaver);
}

SentenceTheoryBeam::SentenceTheoryBeam(StateLoader *stateLoader,
									   int sent_no,
									   DocTheory *docTheory,
									   int width)
	: _unique_subtheories_up_to_date(false)
{
	// EMB: so I can use in relation training w/o doc theories
	if (docTheory != 0)
		_sentence = docTheory->getSentence(sent_no);
	else _sentence = 0;
	int id = stateLoader->beginList(L"SentenceTheoryBeam");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_beam_width = stateLoader->loadInteger();
	if (width != _beam_width) {
		std::wstringstream wss;
		wss << "beam width " << width << " in params doesn't match width " << _beam_width << " in loaded theories";
		throw InternalInconsistencyException("SentenceTheoryBeam::SentenceTheoryBeam", wss);
	}
	_n_theories = stateLoader->loadInteger();

	stateLoader->beginList(L"SentenceTheoryBeam::_theories");
	_theories = _new SentenceTheory*[_beam_width];
	for (int i = 0; i < _n_theories; i++)
		_theories[i] = _new SentenceTheory(stateLoader, _sentence);
	stateLoader->endList();

	// only need to initialize one of them
	_uniqueSubtheories = _new const SentenceSubtheory*[
		_beam_width*SentenceTheory::N_SUBTHEORY_TYPES];

	_n_unique_subtheories = stateLoader->loadInteger();
	stateLoader->beginList(L"SentenceTheoryBeam::_uniqueSubtheories");
	for (int j = 0; j < _n_unique_subtheories; j++) {		
		_uniqueSubtheories[j] = loadSubtheory(stateLoader);
	}
	stateLoader->endList();
	
	stateLoader->endList();
}

void SentenceTheoryBeam::resolvePointers(StateLoader * stateLoader) {
	for (int i = 0; i < _n_theories; i++)
		_theories[i]->resolvePointers(stateLoader);

	ensureUniqueSubtheoriesUpToDate();
	for (int j = 0; j < _n_unique_subtheories; j++) {
		SentenceSubtheory* subtheory = const_cast<SentenceSubtheory*>(_uniqueSubtheories[j]);
		subtheory->resolvePointers(stateLoader);
	}
}

SentenceSubtheory *SentenceTheoryBeam::loadSubtheory(StateLoader *stateLoader)
{
	SentenceTheory::SubtheoryType subtheoryType = 
		(SentenceTheory::SubtheoryType) stateLoader->loadInteger();

	switch (subtheoryType) {
	case SentenceTheory::TOKEN_SUBTHEORY:
		if (TokenSequence::getTokenSequenceTypeForStateLoading() == TokenSequence::LEXICAL_TOKEN_SEQUENCE)
			return _new LexicalTokenSequence(stateLoader);
		else
			return _new TokenSequence(stateLoader);
	case SentenceTheory::POS_SUBTHEORY:
		return _new PartOfSpeechSequence(stateLoader);
	case SentenceTheory::NAME_SUBTHEORY:
		return _new NameTheory(stateLoader);
	case SentenceTheory::NESTED_NAME_SUBTHEORY:
		return _new NestedNameTheory(stateLoader);
	case SentenceTheory::VALUE_SUBTHEORY:
		return _new ValueMentionSet(stateLoader);
	case SentenceTheory::NPCHUNK_SUBTHEORY:
		return _new NPChunkTheory(stateLoader);
	case SentenceTheory::DEPENDENCY_PARSE_SUBTHEORY:
		return _new DependencyParseTheory(stateLoader);
	case SentenceTheory::PARSE_SUBTHEORY:
		return _new Parse(stateLoader);
	case SentenceTheory::MENTION_SUBTHEORY:
		return _new MentionSet(stateLoader);
	case SentenceTheory::PROPOSITION_SUBTHEORY:
		return _new PropositionSet(stateLoader);
	case SentenceTheory::ENTITY_SUBTHEORY:
		return _new EntitySet(stateLoader);
	case SentenceTheory::EVENT_SUBTHEORY:
		return _new EventMentionSet(stateLoader);
	case SentenceTheory::RELATION_SUBTHEORY:
		return _new RelMentionSet(stateLoader);
	default:
		throw UnexpectedInputException(
			"SentenceTheoryBeam::loadSubtheory()",
			"Invalid subtheory type");
	}
}

namespace {
	const XMLCh* getSubtheoryTag(SentenceTheory::SubtheoryType subtheoryType)
	{
		using namespace SerifXML;
		switch(subtheoryType) {
			case SentenceTheory::TOKEN_SUBTHEORY: return X_TokenSequence;
			case SentenceTheory::POS_SUBTHEORY: return X_PartOfSpeechSequence;
			case SentenceTheory::NAME_SUBTHEORY: return X_NameTheory;
			case SentenceTheory::NESTED_NAME_SUBTHEORY: return X_NestedNameTheory;
			case SentenceTheory::VALUE_SUBTHEORY: return X_ValueMentionSet;
			case SentenceTheory::PARSE_SUBTHEORY: return X_Parse;
			case SentenceTheory::DEPENDENCY_PARSE_SUBTHEORY: return X_DependencyParse;
			case SentenceTheory::NPCHUNK_SUBTHEORY: return X_NPChunkTheory;
			case SentenceTheory::MENTION_SUBTHEORY: return X_MentionSet;
			case SentenceTheory::ACTOR_MENTION_SET_SUBTHEORY: return X_ActorMentionSet;
			case SentenceTheory::PROPOSITION_SUBTHEORY: return X_PropositionSet;
			case SentenceTheory::ENTITY_SUBTHEORY: return X_EntitySet;
			case SentenceTheory::EVENT_SUBTHEORY: return X_EventMentionSet;
			case SentenceTheory::RELATION_SUBTHEORY: return X_RelMentionSet;
			default: throw InternalInconsistencyException("getSubtheoryTag",
						 "Unknown subtheory");
		}
	}
}

void SentenceTheoryBeam::saveXML(SerifXML::XMLTheoryElement sentenceElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("SentenceTheoryBeam::saveXML", "Expected context to be NULL");

	// First register the pointers of the SentenceTheory objects
	// in case a subtheory refers back to the SentenceTheory
	SerifXML::XMLIdMap *idMap = sentenceElem.getXMLSerializedDocTheory()->getIdMap();
	for (int i=0; i<_n_theories; ++i)
		idMap->generateId(_theories[i], 0, false);

	// Serialize all of the subtheories.
	ensureUniqueSubtheoriesUpToDate();
	for (int j = 0; j < _n_unique_subtheories; j++) {
		// We don't serialize the per-sentence entity set: it's an artifact of
		// our current pipeline design, and shouldn't really be preserved.
		if (_uniqueSubtheories[j]->getSubtheoryType() == SentenceTheory::ENTITY_SUBTHEORY)
			continue;

		const XMLCh* subtheoryTag = getSubtheoryTag(_uniqueSubtheories[j]->getSubtheoryType());
		sentenceElem.saveChildTheory(subtheoryTag, _uniqueSubtheories[j]);
	}

	// Now serialize the sentence theories.  They will use pointers
	// to refer to the subtheories.  [xx] later: only bother if there
	// is more than one sentence theory?
	for (int i=0; i<_n_theories; ++i)
		sentenceElem.saveChildTheory(X_SentenceTheory, _theories[i]);
}

namespace {
	// Return true if the given XML element appears to contain a LexicalTokenSequence.
	// This is used to decide how to deserialize token sequences.
	bool isLexicalTokenSequenceElement(SerifXML::XMLTheoryElement tokenSequenceElem) {
		using namespace SerifXML;
		XMLTheoryElementList tokElems = tokenSequenceElem.getChildElementsByTagName(X_Token);
		if (tokElems.empty()) { 
			return (TokenSequence::getTokenSequenceTypeForStateLoading() == TokenSequence::LEXICAL_TOKEN_SEQUENCE); 
		} else {
			return tokElems[0].hasAttribute(X_original_token_index);
		}
	}
}

SentenceTheoryBeam::SentenceTheoryBeam(SerifXML::XMLTheoryElement sentenceElem, const Sentence* sentence, const Symbol& docid)
	: _sentence(sentence), _n_theories(0), _theories(0), _beam_width(0),
	  _n_unique_subtheories(0), _unique_subtheories_up_to_date(false)
{
	using namespace SerifXML;
	int sent_no = sentence->getSentNumber();

	// Get the list of sentence theory elements.
	XMLTheoryElementList sentTheoryElems = sentenceElem.getChildElementsByTagName(X_SentenceTheory, false);

	// Determine our beam width (default = 4).
	_beam_width = sentenceElem.getAttribute<int>(X_beam_width, 4);
	if (_beam_width < 1)
		sentenceElem.reportLoadError("Beam width must be at lest 1");
	if (static_cast<int>(sentTheoryElems.size()) > _beam_width) {
		sentenceElem.reportLoadWarning("Too many theories for beam; increasing beam width");
		_beam_width = static_cast<int>(sentTheoryElems.size());
	}

	// Load the subtheories.
	std::vector<SentenceSubtheory*> subtheories;
	BOOST_FOREACH(XMLTheoryElement elem, sentenceElem.getChildElements()) {
		if (elem.hasTag(X_SentenceTheory))
			continue; // Not a subtheory
		else if (elem.hasTag(X_Contents))
			continue; // Not a subtheory
		else if (elem.hasTag(X_OffsetSpan)) 
			continue; // Not a subtheory
		else if (elem.hasTag(X_TokenSequence)) {
			if (isLexicalTokenSequenceElement(elem))
				subtheories.push_back(_new LexicalTokenSequence(elem, sent_no));
			else
				subtheories.push_back(_new TokenSequence(elem, sent_no));
		}
		else if (elem.hasTag(X_PartOfSpeechSequence))
			subtheories.push_back(_new PartOfSpeechSequence(elem));
		else if (elem.hasTag(X_NameTheory))
			subtheories.push_back(_new NameTheory(elem));
		else if (elem.hasTag(X_NestedNameTheory))
			subtheories.push_back(_new NestedNameTheory(elem));
		else if (elem.hasTag(X_ValueMentionSet))
			subtheories.push_back(_new ValueMentionSet(elem));
		else if (elem.hasTag(X_Parse))
			subtheories.push_back(_new Parse(elem));
		else if (elem.hasTag(X_NPChunkTheory))
			subtheories.push_back(_new NPChunkTheory(elem));
		else if (elem.hasTag(X_DependencyParse))
			subtheories.push_back(_new DependencyParseTheory(elem));
		else if (elem.hasTag(X_MentionSet))
			subtheories.push_back(_new MentionSet(elem));
		else if (elem.hasTag(X_ActorMentionSet))
			subtheories.push_back(_new ActorMentionSet(elem));
		else if (elem.hasTag(X_PropositionSet))
			subtheories.push_back(_new PropositionSet(elem, sent_no));
		else if (elem.hasTag(X_EntitySet))
			elem.reportLoadWarning("Entity sets should only occur at the document level");
		else if (elem.hasTag(X_RelMentionSet))
			subtheories.push_back(_new RelMentionSet(elem, sent_no));
		else if (elem.hasTag(X_EventMentionSet))
			subtheories.push_back(_new EventMentionSet(elem, sent_no));
		else {
			std::wstringstream err;
			err << "Unexpected child \"" << elem.getTag() << "\"";
			sentenceElem.reportLoadError(err.str());
		}
	}

	// Load the SentenceTheories.  Each of these mostly consists of a set of
	// pointers to the SentenceSubtheory objects we just loaded.
	_theories = _new SentenceTheory*[_beam_width];
	for (int i = 0; i < _beam_width; i++)
		_theories[i] = 0;
	if (sentTheoryElems.size() > 0) {
		// Add the sentence theories.
		BOOST_FOREACH(XMLTheoryElement elem, sentTheoryElems) {
			addTheory(_new SentenceTheory(elem, sentence, docid));				
		}
	} else if (subtheories.size() > 0) {
		// If no sentence theory was explicitly provided, then use all of
		// the subtheories that were provided to construct one.
		Symbol primary_parse_symbol = sentenceElem.getAttribute<Symbol>(X_primary_parse, Symbol(L"full_parse"));
		if ((primary_parse_symbol != Symbol(L"npchunk_parse")) &&
			(primary_parse_symbol != Symbol(L"full_parse")))
			sentenceElem.reportLoadError("Bad value for primary_parse_symbol");
		SentenceTheory *defaultTheory = _new SentenceTheory(sentence, primary_parse_symbol, docid);
		BOOST_FOREACH(SentenceSubtheory *subtheory, subtheories) {
			if (defaultTheory->getSubtheory(subtheory->getSubtheoryType()) != 0)
				sentenceElem.reportLoadError("Multiple subtheories with the same type");
			defaultTheory->adoptSubtheory(subtheory->getSubtheoryType(), subtheory);
		}
		addTheory(defaultTheory);
	}
	
	// Populate the uniqueSubtheories array.
	_uniqueSubtheories = _new const SentenceSubtheory*[
		_beam_width*SentenceTheory::N_SUBTHEORY_TYPES];
	ensureUniqueSubtheoriesUpToDate();

	// Check for unused subtheories, and throw them away.
	BOOST_FOREACH(SentenceSubtheory *subtheory, subtheories) {
		if (!subtheory->hasReference()) {
			sentenceElem.reportLoadWarning(
				"Discarding unused sentence subtheory");
			delete subtheory;
		}
	}
}

void SentenceTheoryBeam::resolvePointers(SerifXML::XMLTheoryElement sentenceElem) {
	using namespace SerifXML;
	XMLTheoryElement eventMentionSetElem = sentenceElem.getOptionalUniqueChildElementByTagName(X_EventMentionSet);
	if (!eventMentionSetElem.isNull()) {
		for (int i=0; i<getNTheories(); ++i) {
			getTheory(i)->getEventMentionSet()->resolvePointers(eventMentionSetElem);
		}
	}
	XMLTheoryElement actorMentionSetElem = sentenceElem.getOptionalUniqueChildElementByTagName(X_ActorMentionSet);
	if (!actorMentionSetElem.isNull()) {
		for (int i=0; i<getNTheories(); ++i) {
			getTheory(i)->getActorMentionSet()->resolvePointers(actorMentionSetElem);
		}
	}
}

const wchar_t* SentenceTheoryBeam::XMLIdentifierPrefix() const {
	return L"sent";
}

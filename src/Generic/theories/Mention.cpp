// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionConfidence.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/transliterate/Transliterator.h"
#include <boost/scoped_ptr.hpp>

const size_t Mention::N_TYPE_STRINGS = 9;
const char *Mention::TYPE_STRINGS[] = {"none",
									   "name",
									   "pron",
									   "desc",
									   "part",
									   "appo",
									   "list",
									   "infl",
									   "nest"};
const wchar_t *Mention::TYPE_WSTRINGS[] = {L"none",
									    L"name",
									    L"pron",
									    L"desc",
									    L"part",
									    L"appo",
									    L"list",
										L"infl",
										L"nest"};

const char *Mention::getTypeString(Type type) {
	if ((unsigned) type < N_TYPE_STRINGS)
		return TYPE_STRINGS[type];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"Mention::getTypeString()", N_TYPE_STRINGS, type);
}

const Mention::Type Mention::getTypeFromString(const char *typeString) {
	//Loop over the possible Mention::Types, since they're not hashed
	for (size_t type = 0; type < N_TYPE_STRINGS; type++) {
		if (!strncmp(typeString, TYPE_STRINGS[type], 4))
			return static_cast<Mention::Type>(type);
	}
	throw InternalInconsistencyException("Mention::getTypeFromString()", 
		"Unknown mention type string");
}

const Mention::Type Mention::getTypeFromString(const wchar_t *typeString) {
	//Loop over the possible Mention::Types, since they're not hashed
	for (size_t type = 0; type < N_TYPE_STRINGS; type++) {
		if (!wcsncmp(typeString, TYPE_WSTRINGS[type], 4))
			return static_cast<Mention::Type>(type);
	}
	throw InternalInconsistencyException("Mention::getTypeFromString()", 
		"Unknown mention type string");
}

Mention::Mention()
	: _uid(), mentionType(NONE), entityType(EntityType::getOtherType()),
	  _intendedType(EntityType::getUndetType()),
	  _role(EntityType::getUndetType()), entitySubtype(EntitySubtype::getUndetType()),
      node(0), _parent_index(-1),  _child_index(-1), _next_index(-1), _is_metonymy_mention(false),
	  //startChar(-1), endChar(-1), 
	  _confidence(1.0), _link_confidence(1.0), _mentionSet(0)
{}

MentionUID Mention::makeUID(int sentence, int index) {
	if (index < MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS)
	{
		return MentionUID(sentence, index);
	}
	else {
		throw InternalInconsistencyException(
			"Mention::makeUID()",
			"Mention's index exceeds maximum number of mentions per sentence.");
	}
}

Mention::Mention(MentionSet *mentionSet, int index, const SynNode *node_)
	: _uid(makeUID(mentionSet->getSentenceNumber(), index)), mentionType(NONE), entityType(EntityType::getOtherType()),
	  _intendedType(EntityType::getUndetType()),
	  _role(EntityType::getUndetType()), entitySubtype(EntitySubtype::getUndetType()),
      node(node_),
	  //nWords(0), // JJO 08 Aug 2011 - word inclusion desc. linking
	  _parent_index(-1),  _child_index(-1), _next_index(-1), _is_metonymy_mention(false),
	  //startChar(-1), endChar(-1), 
	  _confidence(1.0), _link_confidence(1.0), _mentionSet(mentionSet)
{}

Mention::Mention(MentionSet *mentionSet, const Mention &source)
	: _mentionSet(mentionSet), _uid(source._uid), mentionType(source.mentionType),
	  entityType(source.entityType), _intendedType(source._intendedType),
	  entitySubtype(source.entitySubtype),
	  _role(source._role), node(source.node), _parent_index(source._parent_index),
	  _child_index(source._child_index), _next_index(source._next_index), 
	  _is_metonymy_mention(source._is_metonymy_mention),//startChar(source.startChar),endChar(source.endChar),
	  _confidence(source._confidence), _link_confidence(source._link_confidence)
{}

int Mention::getSentenceNumber() const {
	return _mentionSet->getSentenceNumber();
}

void Mention::setEntityType(EntityType etype) { 
	entityType = etype; 
	// Reset entity subtype if it isn't legal
	if (entitySubtype.getParentEntityType() != entityType) 
		entitySubtype = EntitySubtype::getUndetType();
}

void Mention::setEntitySubtype(EntitySubtype nsubtype) { 
	if (nsubtype == EntitySubtype::getUndetType()  || 
		nsubtype.getParentEntityType() == getEntityType()) {
			entitySubtype = nsubtype; 
	} else {
		SessionLogger::warn("invalid_entity_subtype") << L"Trying to add invalid entity subtype "
			<< getEntityType().getName().to_string() << L"." << nsubtype.getName().to_string();
		entitySubtype = EntitySubtype::getUndetType();
	}
}

bool Mention::hasApposRelationship() const {
	return (mentionType == APPO ||
			(_parent_index != -1 && getParent()->mentionType == APPO));
}

bool Mention::isPartOfAppositive() const {
	// Top of appositive doesn't count
	if (mentionType == Mention::APPO)
		return false;
	if (mentionType == Mention::NONE)
		return false;

	Mention* parent = getParent();
	if (parent == 0)
		return false;
	if (parent->mentionType != Mention::APPO)
		return false;
	// parent --> child (part 1) --> next (part 2)
	Mention* c1 = parent->getChild();
	if (c1 == 0)
		return false;
	if (c1 == this)
		return true;
	Mention* c2 = c1->getNext();
	if (c2 == 0)
		return false;
	if (c2 == this)
		return true;
	return false;
}

bool Mention::hasIntendedType() const {
	return (_intendedType.isDetermined());
}

bool Mention::hasRoleType() const {
	return (_role.isDetermined());
}

const SynNode *Mention::getHead() const {
	switch (mentionType) {
	case NAME:
		// For names, the child may store the head mention
		if (_child_index != -1){	//want the preterminal head - marj
			Mention* m = getChild();
			while(m->getChild() != 0){
				m = m->getChild();
			}
			return m->node;
		}
		else
			return node;
	case PRON:
		return node;
	case APPO:
	case LIST:
		return getChild()->node;
	default:
		return node->getHeadPreterm();
	}
}

const SynNode* Mention::getEDTHead() const {
	if (mentionType == Mention::NAME) {
		if (_child_index != -1) {
			Mention* m = getChild();
			while (m->getChild() != 0)
				m = m->getChild();			
			return m->node;
		}
		else
			return node;
	}
	if (mentionType == Mention::APPO) {
		Mention* child = getChild();
		if (child == 0)
			throw InternalInconsistencyException("Mention::getEDTHead()",
			"Appositive has no children");
		return child->getEDTHead();
	}
	// TODO: city/state hack : return the city, not the state

	// descend until we either come upon another mention or a preterminal
	const SynNode *n = node;
	if (!n->isPreterminal())
		do {
			n = n->getHead();
		} while (!n->isPreterminal() && !n->hasMention());
	if (n->isPreterminal())
		return n;
	return _mentionSet->getMentionByNode(n)->getEDTHead();
}

const SynNode* Mention::getAtomicHead() const {
	if (getMentionType() == Mention::NEST) {
		return getNode()->getHeadPreterm()->getParent();
	} else if (getMentionType() == Mention::NAME) {
		// Look for the first node with the "name" label in the head chain
		// The simple method of taking the parent of the head preterminal does not work
		//  when nested names exist
		const SynNode* node = getNode();
		while (node->getTag() != LanguageSpecificFunctions::getNameLabel()) {
			if (node->getHead())
				node = node->getHead();
			else break;
		}
		if (node)
			return node;
		else return getNode()->getHeadPreterm()->getParent(); // hopefully this should never happen?
	} else return getNode()->getHeadPreterm();
}


Mention *Mention::getParent() const {
	if (_parent_index == -1)
		return 0;
	else
		return _mentionSet->getMention(_parent_index);
}

Mention *Mention::getChild() const {
	if (_child_index == -1)
		return 0;
	else
		return _mentionSet->getMention(_child_index);
}

Mention *Mention::getNext() const {
	if (_next_index == -1)
		return 0;
	else
		return _mentionSet->getMention(_next_index);
}

void Mention::makeOnlyChildOf(Mention *parent) {
	if (parent->_child_index != -1 || _parent_index != -1) {
		throw InternalInconsistencyException("Mention::makeOnlyChildOf()",
			"Attempt to apply compound mention structure to previously structured mentions");
	}

	parent->_child_index = getIndex();
	_parent_index = parent->getIndex();
	_next_index = -1;
}

void Mention::makeNextSiblingOf(Mention *prevSib) {
	if (prevSib->_next_index != -1 || _parent_index != -1) {
		throw InternalInconsistencyException("Mention::makeNextSiblingOf()",
			"Attempt to apply compound mention structure to previously structured mentions");
	}
	prevSib->_next_index = getIndexFromUID(_uid);
	_parent_index = prevSib->_parent_index;
	_next_index = -1;
}

void Mention::makeOrphan() {
	if (_parent_index != -1) {
		Mention * parent = _mentionSet->getMention(_parent_index);

		if (parent->_child_index != getIndex() ||
			_next_index != -1)
		{
			throw InternalInconsistencyException("Mention::makeOrphan()",
				"Attempt to makeOrphan() failed because mention has siblings");
		}

		parent->_child_index = -1;
		_parent_index = -1;
	}
}

Entity* Mention::getEntity(const DocTheory* docTheory) const {
	return docTheory->getEntitySet()->getEntityByMention(_uid);
}

EntitySubtype Mention::guessEntitySubtype(const DocTheory *docTheory) const {
	Entity* ent = getEntity(docTheory);
	if (ent == 0)
		return EntitySubtype::getUndetType();
	else
		return ent->guessEntitySubtype(docTheory);
}

void Mention::setIndex(int index) {
	_uid = makeUID(_mentionSet->getSentenceNumber(), index);
}

int Mention::getIndex() const {
	return getIndexFromUID(_uid);
}

int Mention::getIndexFromUID(MentionUID uid) {
	return uid.index();
}

int Mention::getSentenceNumberFromUID(MentionUID uid) {
	return uid.sentno();
}

void Mention::dump(std::ostream &out, int indent) const {
	out << "Mention " <<  getUID()
		<< " (" << getTypeString(mentionType) << "/"
		<< entityType.getName().to_debug_string(); 
	if (hasIntendedType())
		out << "." << _intendedType.getName().to_debug_string();
	if (hasRoleType())
		out << "." << _role.getName().to_debug_string();
	out << "/" << getEntitySubtype().getName().to_debug_string();
	out << "): " << "node " << node->getID();
	if (_parent_index != -1)
		out << "; parent = mention " << _parent_index;
	if (_child_index != -1)
		out << "; child = mention " << _child_index;
	if (_next_index != -1)
		out << "; next = mention " << _next_index;
}


void Mention::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void Mention::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"Mention", this);

	stateSaver->saveInteger(_uid.toInt());
	stateSaver->saveInteger(mentionType);
	stateSaver->saveSymbol(entityType.getName());
	stateSaver->saveSymbol(_intendedType.getName());
	if (_is_metonymy_mention)
		stateSaver->saveInteger(1);
	else stateSaver->saveInteger(0);
	stateSaver->saveSymbol(entitySubtype.getName());
	
	stateSaver->saveSymbol(_role.getName());
	stateSaver->savePointer(node);

	stateSaver->savePointer(_mentionSet);

	stateSaver->saveInteger(_parent_index);
	stateSaver->saveInteger(_child_index);
	stateSaver->saveInteger(_next_index);
	if (stateSaver->getVersion() <= std::make_pair(1,5)) {
		stateSaver->saveInteger(0); // startChar
		stateSaver->saveInteger(0); // endChar
	}
	stateSaver->endList();
}

void Mention::loadState(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"Mention");
	stateLoader->getObjectPointerTable().addPointer(id, this);

    _uid = MentionUID(stateLoader->loadInteger());
    mentionType = (Type) stateLoader->loadInteger();
	entityType = EntityType(stateLoader->loadSymbol());
	_intendedType = EntityType(stateLoader->loadSymbol());
	if (stateLoader->loadInteger())
		_is_metonymy_mention = true;
	Symbol justSubtype = stateLoader->loadSymbol();
	try {
		entitySubtype = EntitySubtype(entityType.getName(), justSubtype);
	} catch (UnrecoverableException &e) {
		SessionLogger::warn("mention_load_state") << e.getMessage();
		entitySubtype = EntitySubtype::getUndetType();
	}
	_role = EntityType(stateLoader->loadSymbol());
    node = (SynNode *) stateLoader->loadPointer();
	
	_mentionSet = (MentionSet *) stateLoader->loadPointer();

    _parent_index = stateLoader->loadInteger();
    _child_index = stateLoader->loadInteger();
    _next_index = stateLoader->loadInteger();
	if (stateLoader->getVersion() <= std::make_pair(1,5)) {
		stateLoader->loadInteger(); // startChar
		stateLoader->loadInteger(); // endChar
	}
	stateLoader->endList();
}

void Mention::resolvePointers(StateLoader * stateLoader) {
	node = (SynNode *) stateLoader->getObjectPointerTable().getPointer(node);

	_mentionSet = (MentionSet *) stateLoader->getObjectPointerTable().getPointer(_mentionSet);
}



bool Mention::is1pPronoun() const
{
	return WordConstants::is1pPronoun(getNode()->getHeadWord());
}

bool Mention::is2pPronoun() const
{
	return WordConstants::is2pPronoun(getNode()->getHeadWord());
}

bool Mention::is3pPronoun() const
{
	return WordConstants::is3pPronoun(getNode()->getHeadWord());
}

// This was added for a sanity check in the serialization -- we want to 
// make sure that two mention objects that are different objects actually
// hold identical data.
bool Mention::isIdenticalTo(const Mention& other) const {
	return ((_uid == other._uid) && 
		(mentionType == other.mentionType) &&
		(entityType == other.entityType) &&
		(entitySubtype == other.entitySubtype) &&
		(node == other.node) &&
		//(startChar == other.startChar) &&
		//(endChar == other.endChar) &&
		(_parent_index == other._parent_index) &&
		(_child_index == other._child_index) &&
		(_intendedType == other._intendedType) && 
		(_is_metonymy_mention == other._is_metonymy_mention) &&
		(_confidence == other._confidence) &&
		(_link_confidence == other._link_confidence) &&
		//(_mentionSet == other._mentionSet) &&
		(_next_index == other._next_index) &&
		(_role == other._role));
}

const wchar_t* Mention::XMLIdentifierPrefix() const {
	return L"mention";
}

void Mention::saveXML(SerifXML::XMLTheoryElement mentionElem, const Theory *context) const {
	using namespace SerifXML;
	mentionElem.getXMLSerializedDocTheory()->registerMentionId(this);
	if (context != 0)
		throw InternalInconsistencyException("Mention::saveXML", "Expected context to be NULL");
	// We don't serialize _uid, since it's redundant.
	mentionElem.saveTheoryPointer(X_syn_node_id, node);
	mentionElem.setAttribute(X_mention_type, getTypeString(mentionType));
	mentionElem.setAttribute(X_entity_type, entityType.getName());
	mentionElem.setAttribute(X_intended_type, _intendedType.getName());
	mentionElem.setAttribute(X_is_metonymy, _is_metonymy_mention);
	mentionElem.setAttribute(X_entity_subtype, entitySubtype.getName());
	mentionElem.setAttribute(X_role_type, _role.getName());
	if (_confidence != 1.0)
		mentionElem.setAttribute(X_confidence, _confidence);
	if (_link_confidence != 1.0) 
		mentionElem.setAttribute(X_link_confidence, _link_confidence);
	if (getParent())
		mentionElem.saveTheoryPointer(X_parent, getParent());
	if (getChild())
		mentionElem.saveTheoryPointer(X_child, getChild());
	if (getNext())
		mentionElem.saveTheoryPointer(X_next, getNext());
	if (mentionElem.getOptions().include_mention_transliterations) {
		boost::scoped_ptr<Transliterator> transliterator(Transliterator::build());
		mentionElem.setAttribute(X_transliteration, transliterator->getTransliteration(this));
	}
	if (mentionElem.getOptions().include_spans_as_comments)
		mentionElem.addComment(toCasedTextString());
}

std::wstring Mention::toCasedTextString() const {
	const TokenSequence* tokenSequence = _mentionSet->getParse()->getTokenSequence();
	return getNode()->toCasedTextString(tokenSequence);
}

std::wstring Mention::toAtomicCasedTextString() const {
	const TokenSequence* tokenSequence = _mentionSet->getParse()->getTokenSequence();
	return getAtomicHead()->toCasedTextString(tokenSequence);
}

const Mention *Mention::getMostRecentAntecedent(const DocTheory *docTheory) const {
	const Entity *ent = getEntity(docTheory);
	if (ent == 0)
		return 0;
	const Mention *antecedent = 0;
	int target_sentence_number = getSentenceNumber();
	for (int j = 0; j < ent->getNMentions(); j++) {
		MentionUID uid = ent->getMention(j);
		int entMentSentenceNumber = Mention::getSentenceNumberFromUID(uid);
		if (entMentSentenceNumber > target_sentence_number)
			continue;
		if (antecedent != 0 && entMentSentenceNumber < antecedent->getSentenceNumber())
			continue;
		const Mention* entMent = docTheory->getSentenceTheory(entMentSentenceNumber)->getMentionSet()->getMention(uid);
		if (entMent->getMentionType() != Mention::PRON) {
			if (antecedent == 0 ||
				antecedent->getSentenceNumber() < entMentSentenceNumber ||
				antecedent->getNode()->getStartToken() < entMent->getNode()->getStartToken())
			{
				antecedent = entMent;
			}
		}
	}
	return antecedent;	
}


Mention::Mention(SerifXML::XMLTheoryElement mentionElem, MentionSet *mentionSet, MentionUID uid)
    : _mentionSet(mentionSet), _uid(uid)
{
	using namespace SerifXML;
	Symbol undetSym(L"UNDET");
	mentionElem.loadId(this);
	mentionType = getTypeFromString(mentionElem.getAttribute<std::wstring>(X_mention_type).c_str());
	entityType = EntityType(mentionElem.getAttribute<Symbol>(X_entity_type));
	entitySubtype = EntitySubtype(entityType.getName(), mentionElem.getAttribute<Symbol>(X_entity_subtype, undetSym));
	node = mentionElem.loadTheoryPointer<SynNode>(X_syn_node_id);

	_intendedType = EntityType(mentionElem.getAttribute<Symbol>(X_intended_type, undetSym));
	_role = EntityType(mentionElem.getAttribute<Symbol>(X_role_type, undetSym));
	_is_metonymy_mention = mentionElem.getAttribute<bool>(X_is_metonymy, false);
	_confidence = mentionElem.getAttribute<float>(X_confidence, 1.0);
	_link_confidence = mentionElem.getAttribute<float>(X_link_confidence, 1.0);
	
	_parent_index = -1; // Filled in by rseolverPointers(XMLTheoryElement)
	_child_index = -1; // Filled in by rseolverPointers(XMLTheoryElement)
	_next_index = -1; // Filled in by rseolverPointers(XMLTheoryElement)
}

void Mention::resolvePointers(SerifXML::XMLTheoryElement mentionElem) {
	using namespace SerifXML;
	if (const Mention* parent = mentionElem.loadOptionalTheoryPointer<Mention>(X_parent))
		_parent_index = parent->getUID().index();
	if (const Mention* child = mentionElem.loadOptionalTheoryPointer<Mention>(X_child))
		_child_index = child->getUID().index();
	if (const Mention* next = mentionElem.loadOptionalTheoryPointer<Mention>(X_next))
		_next_index = next->getUID().index();
}

MentionConfidenceAttribute Mention::brandyStyleConfidence(
	const DocTheory* dt, const SentenceTheory* st, std::set<Symbol>& ambiguousLastNames) const 
{
	return MentionConfidence::determineMentionConfidence(dt, st, this, ambiguousLastNames);
}

bool Mention::isBadNeutralPronoun(const MentionSet * mentionSet, PropositionSet * propSet) const
{	
	if (getMentionType() != Mention::PRON)
		return false;
	Symbol headword = getNode()->getHeadWord();
	int ment_token = getNode()->getStartToken();

	if (!WordConstants::isNonPersonPronoun(headword))
		return false;

	const SynNode *root = getNode();
	while (root->getParent() != 0)
		root = root->getParent();

	// find one and only GPE preceding the mention
	const Mention *gpePrecedingMention = 0;
	for (int m = 0; m < mentionSet->getNMentions(); m++) {
		const Mention *other_ment = mentionSet->getMention(m);
		if (other_ment->getMentionType() != Mention::NAME || !other_ment->getEntityType().matchesGPE())
			continue;
		if (other_ment->getNode()->getEndToken() < ment_token) {
			if (gpePrecedingMention != 0)
				return false;
			gpePrecedingMention = other_ment;
		}
	}
	if (gpePrecedingMention == 0)
		return false;
	
	if (WordConstants::isPossessivePronoun(headword)) {
		if (!propSet->isDefinitionArrayFilled())
			propSet->fillDefinitionsArray();
		for (int p = 0; p < propSet->getNPropositions(); p++) {
			const Proposition* prop = propSet->getProposition(p);
			bool gpe_is_subject = false;
			for (int a = 0; a < prop->getNArgs(); a++) {
				if (prop->getArg(a)->getRoleSym() == Argument::SUB_ROLE && 
					prop->getArg(a)->getType() == Argument::MENTION_ARG && 
					prop->getArg(a)->getMention(mentionSet) == gpePrecedingMention) 
				{
					gpe_is_subject = true;
					break;
				}
			}
			if (!gpe_is_subject) continue;
			for (int a = 0; a < prop->getNArgs(); a++) {
				if ((prop->getArg(a)->getRoleSym() == Argument::OBJ_ROLE || prop->getArg(a)->getRoleSym() == Argument::IOBJ_ROLE) && 
					prop->getArg(a)->getType() == Argument::MENTION_ARG) 
				{
					const Proposition *obj_prop = propSet->getDefinition(prop->getArg(a)->getMention(mentionSet)->getIndex());
					if (obj_prop == 0)
						break;
					for (int oa = 0; oa < obj_prop->getNArgs(); oa++) {
						if (obj_prop->getArg(oa)->getType() == Argument::MENTION_ARG && 
							obj_prop->getArg(oa)->getMention(mentionSet) == this) 
						{
							return true;
						}
					}
				}
			}
		}
	} else {
		bool ment_is_subject = false;
		bool gpe_is_subject = false;
		for (int p = 0; p < propSet->getNPropositions(); p++) {
			const Proposition* prop = propSet->getProposition(p);
			for (int a = 0; a < prop->getNArgs(); a++) {
				if (prop->getArg(a)->getRoleSym() == Argument::SUB_ROLE) {
					const Argument *arg = prop->getArg(a);
					if (arg->getType() != Argument::MENTION_ARG)
						continue;
					if (arg->getMention(mentionSet) == this)
						ment_is_subject = true;
					if (arg->getMention(mentionSet) == gpePrecedingMention)
						gpe_is_subject = true;	
				}
			}
		}

		// Russia said that it will...
		if (ment_is_subject && gpe_is_subject)
			return true;
		else return false;
	}

	return false;
}

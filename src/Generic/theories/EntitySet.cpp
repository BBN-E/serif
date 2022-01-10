// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/OutputUtil.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/WordConstants.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include "Generic/common/ParamReader.h"

#include <boost/foreach.hpp>

DebugStream &EntitySet::_debugOut = DebugStream::referenceResolverStream;

EntitySet::EntitySet(int nSentences) : _score(0), _nPrevMentionSets(0),
									   _currMentionSet(0),
									   _prevMentionSets(0)
{
	if (nSentences < 0)
		throw InternalInconsistencyException("EntitySet::EntitySet()", "Invalid number of sentences.");
	_nSentences = nSentences;
	_entitiesByType = _new GrowableArray <Entity *>[EntityType::getNTypes()];
	if (!isEmpty()) {
		_prevMentionSets = _new MentionSet * [(_nSentences > 0) ? nSentences : 1];  // FIXME
	}
}

EntitySet::EntitySet(const EntitySet &other, bool copyMentionSet) 
	: _nPrevMentionSets(other._nPrevMentionSets), _score(other._score),
	_nSentences(other._nSentences)
{
	int i;
	_entitiesByType = _new GrowableArray <Entity *>[EntityType::getNTypes()];
	if (other.isEmpty())
		_prevMentionSets = NULL;
	else {
		_prevMentionSets = _new MentionSet * [_nSentences];
		for (i=0; i<other._nPrevMentionSets; i++) {
			_prevMentionSets[i] = other._prevMentionSets[i];
			_prevMentionSetsOwned.push_back(false);
		}
	}
	if (other._currMentionSet == NULL)
		_currMentionSet = NULL;
	else if (copyMentionSet) {
		_currMentionSet = _new MentionSet(*other._currMentionSet);
		_currMentionSetOwned = true;
	}
	else {
		_prevMentionSets[_nPrevMentionSets++] = other._currMentionSet;
		_prevMentionSetsOwned.push_back(false);
		_currMentionSet = NULL;
	}
	for (i=0; i<other._entities.length(); i++) {
		Entity *newEntity = _new Entity(*other._entities[i]);
		_entities.add(newEntity);
		if (other._entities[i]->type.isDetermined()) {
            int index = other._entities[i]->type.getNumber();
			_entitiesByType[index].add(newEntity);
		}
		else {
			_undetEntities.add(newEntity);
		}
	}
}

EntitySet::EntitySet(const std::vector<EntitySet*> splitEntitySets, const std::vector<int> sentenceOffsets, const std::vector<MentionSet*> mergedMentionSets)
: _nSentences(static_cast<int>(mergedMentionSets.size())), _nPrevMentionSets(static_cast<int>(mergedMentionSets.size()) - 1)
{
	_entitiesByType = _new GrowableArray <Entity *>[EntityType::getNTypes()];

	// Copy in pointers from previous merged mention sets
	if (_nSentences > 0)
		_prevMentionSets = _new MentionSet*[_nSentences];
	else
		_prevMentionSets = NULL;
	for (int ms = 0; ms < _nPrevMentionSets; ms++) {
		_prevMentionSets[ms] = mergedMentionSets.at(ms);
		_prevMentionSetsOwned.push_back(false);
	}
	_currMentionSet = _new MentionSet(*(mergedMentionSets.at(_nPrevMentionSets)));
	_currMentionSetOwned = true;

	// Merge document-level entity sets, shifting entity IDs and mention UIDs
	int entity_offset = 0;
	_score = 0.0;
	for (size_t es = 0; es < splitEntitySets.size(); es++) {
		EntitySet* splitEntitySet = splitEntitySets[es];
		if (splitEntitySet != NULL) {
			for (int e = 0; e < splitEntitySet->getNEntities(); e++) {
				Entity* entity = _new Entity(*(splitEntitySet->getEntity(e)));
				entity->ID += entity_offset;
				if (sentenceOffsets[es] > 0)
					// Loop over old entity's mentions so we don't invalidate iterator
					for (int m = 0; m < splitEntitySet->getEntity(e)->getNMentions(); m++) {
						MentionUID uid = splitEntitySet->getEntity(e)->getMention(m);
						MentionUID newUid = MentionUID(uid.sentno() + sentenceOffsets[es], uid.index());
						entity->removeMention(uid);
						entity->addMention(newUid);
					}
				_entities.add(entity);
			}
			entity_offset += splitEntitySet->getNEntities();
			if (splitEntitySet->getScore() > _score)
				_score = splitEntitySet->getScore();
		}
	}

	// Update entity lookup tables
	for (int e = 0; e < _entities.length(); e++) {
		Entity* entity = _entities[e];
		if (entity->type.isDetermined())
			_entitiesByType[entity->type.getNumber()].add(entity);
		else
			_undetEntities.add(entity);
	}
}

EntitySet::~EntitySet() {
	for (int i=0; i<_entities.length(); i++) {
		delete _entities[i];
		_entities[i] = NULL;
	}

	if (_prevMentionSets) {
		for (int i = 0; i < _nPrevMentionSets; i++) {
			if (_prevMentionSetsOwned.at(i)) {
				delete _prevMentionSets[i];
			}
        }
        delete[] _prevMentionSets;
    }
	delete[] _entitiesByType;
	if (_currMentionSet && _currMentionSetOwned) {
		delete _currMentionSet;
	}
}


Entity *EntitySet::getEntity(int i) const {
	
	if ((unsigned) i < (unsigned) _entities.length())
		return _entities[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"EntitySet::getEntity()", _entities.length(), i);
}

Mention *EntitySet::getMention(MentionUID uid) const {
	if (isEmpty())
		return NULL;

	int sentence = Mention::getSentenceNumberFromUID(uid);
	int index = Mention::getIndexFromUID(uid);
	if(sentence >= getNMentionSets())
		throw InternalInconsistencyException("EntitySet::getMention()", "Requested out-of-bounds MentionSet");
	return getMentionSet(sentence)->getMention(index);
}

Entity* EntitySet::getEntityByMention(MentionUID uid) const
{
	// need to scan each mention of each entity. 
	// We can make it a little faster thanks to 
	// entity type comparison, but that's about it

	if (isEmpty())
		return NULL;

	int typeIndex;
	int i;
	Mention *mention = getMention(uid);
	typeIndex = mention->getEntityType().getNumber();

	// the ?: pairs are because we can't assign to Growable Arrays
	// MRK: use a reference!
	for (i=0; i < (typeIndex < 0 ? _undetEntities : _entitiesByType[typeIndex]).length(); i++) {
		Entity* ent = (typeIndex < 0 ? _undetEntities : _entitiesByType[typeIndex])[i];
		GrowableArray<MentionUID> &ments = ent->mentions;
		int j;
		for (j=0; j < ments.length(); j++) {
			if (ments[j] == uid)
				return ent;
		}
	}
	// didn't find it
	return 0;
}

Entity* EntitySet::getEntityByMentionWithoutType(MentionUID uid) const 
{
	// slower, but necessary if the mention itself may not have been typed yet
	if (isEmpty())
		return NULL;

	int i;
	for (i=0; i < _entities.length(); i++) {
		Entity* ent = _entities[i];
		GrowableArray<MentionUID> &ments = ent->mentions;
		int j;
		for (j=0; j < ments.length(); j++) {
			if (ments[j] == uid)
				return ent;
		}
	}
	// didn't find it
	return 0;
}
		
Entity* EntitySet::getEntityByMention(MentionUID uid, EntityType type) const {
	if (isEmpty())
		return NULL;

	// need to scan each mention of each entity. 
	// We can make it a little faster thanks to 
	// entity type comparison, but that's about it

	int typeIndex;
	if (type.isDetermined())
		typeIndex = type.getNumber();
	else
		typeIndex = -1;

	// the ?: pairs are because we can't assign to Growable Arrays
	// MRK: use a reference!
	for (int i=0;
		 i < (typeIndex < 0 ? _undetEntities
							: _entitiesByType[typeIndex]).length();
		 i++)
	{
		Entity* ent = (typeIndex < 0 ? _undetEntities : _entitiesByType[typeIndex])[i];
		GrowableArray<MentionUID> &ments = ent->mentions;
		int j;
		for (j=0; j < ments.length(); j++) {
			if (ments[j] == uid)
				return ent;
		}
	}
	// didn't find it
	return 0;
}

EntitySubtype EntitySet::guessEntitySubtype(const Entity *entity) const {
	bool prefer_pronouns = ParamReader::getOptionalTrueFalseParamWithDefaultVal("prefer_pronouns_for_per_subtype_guess", false);
	EntitySubtype UNDET = EntitySubtype::getUndetType();
	EntitySubtype namSubtype = UNDET;
	EntitySubtype nomSubtype = UNDET;
	EntitySubtype proSubtype = UNDET;
	

	const GrowableArray<MentionUID> &ments = entity->mentions;

	for (int j = 0; j < ments.length(); j++) {
		const Mention *ment = getMention(ments[j]);
		// just in case there's a leftover subtype from when this
		// mention was another type...
		if (ment->getEntitySubtype() != UNDET &&
			ment->getEntitySubtype().getParentEntityType() == entity->getType())
		{
			if ((ment->getMentionType() == Mention::NAME ||
			     ment->getMentionType() == Mention::NEST) && namSubtype == UNDET)
				namSubtype = ment->getEntitySubtype();
			else if (ment->getMentionType() == Mention::DESC && nomSubtype == UNDET)
				nomSubtype = ment->getEntitySubtype();		
		}	
		/** This code is hacky in at least two ways:
		//  (a) it is ACE 2005 specific (it assumes Per.Group and Per.Individual are subtypes)
		//  (b) it uses the language specific WordConstants functions to get singular/plural information.
		//        As a result, output will be different when SERIF is run compiled 'generic' mode (e.g. Brandy)
		//        from when it is compiled in a specific language (English, Chinese, Arabic).  
		//   I've introduced these hacks because:
		//   (1) You can't assign subtypes to pronouns during subtype(descriptor) classification, because pronouns
		//          have not yet been assigned an entity type
		//   (2) Pronouns are important for making the group vs individual distinction
		//	The rules are by design maximally conservative.  They only use a pronoun to set subtype if it is EXCLUSIVELY
		//  plural or singular.  Pronouns like 'you' in English can return true for both isSingularPronoun() and isPluralPronoun()
		**/
		else if(ment->getMentionType() == Mention::PRON  && entity->getType().matchesPER()){
			Symbol hw = ment->getNode()->getHeadWord();
			if(WordConstants::isSingularPronoun(hw) && !WordConstants::isPluralPronoun(hw)){
				try{
					proSubtype =  EntitySubtype(Symbol(L"PER"), Symbol(L"Individual"));
				}
				catch(UnexpectedInputException){
					//probably not ACE2005, just ignore this
					;
				}
			}
			if(WordConstants::isPluralPronoun(hw) && !WordConstants::isSingularPronoun(hw)){
				try{
					proSubtype =  EntitySubtype(Symbol(L"PER"), Symbol(L"Group"));
				}
				catch(UnexpectedInputException){
					//probably not ACE2005, just ignore this
					;
				}
			}
		}
	}
	if (namSubtype != UNDET) 
		return namSubtype;
	if(entity->getType().matchesPER() && prefer_pronouns && proSubtype != UNDET)
		return proSubtype;
	if (nomSubtype != UNDET){
		return nomSubtype;
	}
	if (proSubtype != UNDET){
		return proSubtype;
	}

	return EntitySubtype::getUndetType();
}

Entity* EntitySet::getIntendedEntityByMention(MentionUID uid) const  {
	if (isEmpty())
		return NULL;

	// need to scan each mention of each entity. 
	// We can make it a little faster thanks to 
	// entity type comparison, but that's about it

	int typeIndex;
	Mention *mention = getMention(uid);
	if (mention->getIntendedType().isDetermined())
		typeIndex = mention->getIntendedType().getNumber();
	else
		typeIndex = -1;

	// the ?: pairs are because we can't assign to Growable Arrays
	// MRK: use a reference!
	for (int i = 0;
		 i < (typeIndex < 0 ? _undetEntities
		                    : _entitiesByType[typeIndex]).length();
		 i++)
	{
		Entity* ent = (typeIndex < 0 ? _undetEntities : _entitiesByType[typeIndex])[i];
		GrowableArray<MentionUID> &ments = ent->mentions;
		int j;
		for (j=0; j < ments.length(); j++) {
			if (ments[j] == uid)
				return ent;
		}
	}
	// didn't find it
	return 0;
}

int EntitySet::getNEntities() const {
	return _entities.length();
}

// copyMentionSet should always be true, unless we are rebuilding after
// saving our state, in which case we are playing some memory games
void EntitySet::loadMentionSet(const MentionSet *newSet) {
	if (!isEmpty()) {
		if(_currMentionSet != NULL) {
			_prevMentionSets[_nPrevMentionSets++] = _currMentionSet;
			_prevMentionSetsOwned.push_back(_currMentionSetOwned);
		}
		_currMentionSet = _new MentionSet(*newSet);
		_currMentionSetOwned = true;
	}
}

void EntitySet::loadDoNotCopyMentionSet(MentionSet *newSet) {
	if (!isEmpty()) {
		if(_currMentionSet != NULL) {
			_prevMentionSets[_nPrevMentionSets++] = _currMentionSet;
			_prevMentionSetsOwned.push_back(_currMentionSetOwned);
		}
		_currMentionSet = newSet;
		_currMentionSetOwned = false;
	}
}


const MentionSet *EntitySet::getMentionSet(int i) const {
	if (i < _nPrevMentionSets)
		return _prevMentionSets[i];
	else if (i < getNMentionSets())
		return _currMentionSet;
	else throw InternalInconsistencyException("EntitySet::getMentionSet()", "Array index out of bounds.");
}
MentionSet *EntitySet::getNonConstMentionSet(int i) const {
	if (i < _nPrevMentionSets)
		return _prevMentionSets[i];
	else if (i < getNMentionSets())
		return _currMentionSet;
	else throw InternalInconsistencyException("EntitySet::getNonConstMentionSet()", "Array index out of bounds.");
}

int EntitySet::getNMentionSets() const {
	return (_currMentionSet != NULL) ? _nPrevMentionSets+1 : _nPrevMentionSets;
}

/*
EntitySet * EntitySet::fork() {
	EntitySet * newSet = _new EntitySet(*this);
	return newSet;
}
*/

const GrowableArray <Entity *> &EntitySet::getEntities() const {
	return _entities;
}

int EntitySet::getNEntitiesByType(EntityType type) const {
	if (type.isDetermined())
		return _entitiesByType[type.getNumber()].length();
	else 
		return _undetEntities.length();
}
const GrowableArray <Entity *> &EntitySet::getEntitiesByType(EntityType type) const {
	if (type.isDetermined())
		return _entitiesByType[type.getNumber()];
	else 
		return _undetEntities;
}

void EntitySet::addNew(MentionUID uid, EntityType type) {
	int ID = _entities.length();
	Entity *newEntity = _new Entity(ID, type);
	//TODO: any other initialization for entity?
	_entities.add(newEntity);
	if (type.isDetermined())
		_entitiesByType[type.getNumber()].add(newEntity);
	else
		_undetEntities.add(newEntity);
	add(uid, ID);
}

void EntitySet::add(MentionUID uid, int ID) {
	Entity *thisEntity = _entities[ID];
	if(thisEntity == NULL) {
		return;	//TODO: complain here
	}
	thisEntity->addMention(uid);
}

void EntitySet::determineMentionConfidences(DocTheory* docTheory) {
	std::set<Symbol> ambiguousLastNames;
	std::set<Symbol> seenLastNames;
	
	for (int e = 0; e < _entities.length(); e++) {
		Entity* entity = _entities[e];
		if (!entity->getType().matchesPER())
			continue;
		std::set<Symbol> entityNames;
		for (int m = 0; m < entity->getNMentions(); m++) {
			MentionUID uid = entity->getMention(m);
			SentenceTheory* sentTheory = docTheory->getSentenceTheory(uid.sentno());
			Mention* mention = sentTheory->getMentionSet()->getMention(uid.index());
			const SynNode *head = mention->getHead();
			int n_terminals = head->getNTerminals();
			if (n_terminals < 2)
				continue;
			// Multiword PER mention
			Symbol lastName = head->getNthTerminal(n_terminals - 1)->getHeadWord();
			if (seenLastNames.find(lastName) != seenLastNames.end())
				ambiguousLastNames.insert(lastName);
			entityNames.insert(lastName);
		}
		BOOST_FOREACH(Symbol lastName, entityNames) {
			seenLastNames.insert(lastName);
		}
	}

	for (int e = 0; e < _entities.length(); e++) {
		Entity* entity = _entities[e];
		for (int m = 0; m < entity->getNMentions(); m++) {
			MentionUID uid = entity->getMention(m);
			SentenceTheory* sentTheory = docTheory->getSentenceTheory(uid.sentno());
			Mention* mention = sentTheory->getMentionSet()->getMention(uid.index());
			entity->setMentionConfidence(uid, mention->brandyStyleConfidence(docTheory, sentTheory, ambiguousLastNames));
		}
	}
}

void EntitySet::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Entity Set (" << _entities.length() << " entities; "
						  << "score: " << _score << "): ";

	if (_entities.length() == 0) {
		out << newline << "  (no entities)";
	} else {
		for (int i = 0; i < _entities.length(); i++) {
			out << newline << "- ";
			_entities[i]->dump(out, 0, this);
		}
	}

	delete[] newline;
}


void EntitySet::customDebugPrint(DebugStream &out) {
	for(int i=0; i<_entities.length(); i++) {
		out << " [" << _entities[i]->ID << ": ";
		for (int j=0; j<_entities[i]->mentions.length(); j++) 
			out << Mention::getSentenceNumberFromUID(_entities[i]->mentions[j]) << "." 
				<< Mention::getIndexFromUID(_entities[i]->mentions[j]) << " ";
		out << "]";
	}
}


void EntitySet::customDebugPrintWcout() {
	for(int i=0; i<_entities.length(); i++) {
		std::wcout << " [" << _entities[i]->ID << ": ";
		for (int j=0; j<_entities[i]->mentions.length(); j++) 
			std::wcout << Mention::getSentenceNumberFromUID(_entities[i]->mentions[j]) << "." 
				<< Mention::getIndexFromUID(_entities[i]->mentions[j]) << " ";
		std::wcout << "]";
	}
}

void EntitySet::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	for (int i = 0; i < _entities.length(); i++)
		_entities[i]->updateObjectIDTable();
}

#define BEGIN_ENTITYSET (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::EntitySetOffset))

void EntitySet::saveState(StateSaver *stateSaver) const {
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_ENTITYSET, this);
	} else {
		stateSaver->beginList(L"EntitySet", this);
	}

	stateSaver->saveInteger(_nSentences);
	stateSaver->saveReal(_score);

	stateSaver->saveInteger(_entities.length());
	stateSaver->beginList(L"EntitySet::_entities");
	for (int i = 0; i < _entities.length(); i++)
		_entities[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

EntitySet::EntitySet(StateLoader *stateLoader) {
	//Use the integer replacement for "EntitySet" if the state file was compressed and we're looking at the sentence level
	//std::cerr << "EntitySet: compressed state " << stateLoader->useCompressedState() << ", sent-level " << sentenceLevel << std::endl;
	int id = stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_ENTITYSET : L"EntitySet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_nSentences = stateLoader->loadInteger();
	_score = stateLoader->loadReal();

	_entitiesByType = _new GrowableArray <Entity *>[EntityType::getNTypes()];
	
	int n_entities = stateLoader->loadInteger();
	_entities.setLength(n_entities);
	stateLoader->beginList(L"EntitySet::_entities");
	for (int i = 0; i < n_entities; i++)
		_entities[i] = _new Entity(stateLoader);
	stateLoader->endList();

	_prevMentionSets = _new MentionSet * [_nSentences];
	_currMentionSet = NULL;
	_nPrevMentionSets = 0;

	stateLoader->endList();
}

bool EntitySet::FakeEntitySet(StateLoader *stateLoader) { // Otherwise unnecessary bool return required by AQtime profiler
	//Use the integer replacement for "EntitySet" if the state file was compressed
	//std::cerr << "FakeEntitySet: compressed state " << stateLoader->useCompressedState() << std::endl;
	stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_ENTITYSET : L"EntitySet");
	stateLoader->loadInteger(); //n_sentences
	stateLoader->loadReal(); //score
	int n_ent = stateLoader->loadInteger(); //n_entities
	stateLoader->beginList(L"EntitySet::_entities");
	for (int i = 0; i < n_ent; i++){
		//fake the entities too
		stateLoader->beginList(L"Entity"); //entity id
		stateLoader->loadInteger(); //ID
		stateLoader->loadSymbol(); //Type

		int n_mentions = stateLoader->loadInteger(); //n_mentions
		stateLoader->beginList(L"Entity::mentions");
		for (int j = 0; j < n_mentions; j++)
			stateLoader->loadInteger(); //mention
		stateLoader->endList();
		if (stateLoader->getVersion() >= std::make_pair(1,7)) {
			stateLoader->loadInteger(); // is_generic
		}
		stateLoader->endList();
	}
	stateLoader->endList();
	stateLoader->endList();
    return true;
}

void EntitySet::resolvePointers(StateLoader * stateLoader) {

	for (int i = 0; i < _entities.length(); i++) {
		_entities[i]->resolvePointers(stateLoader);
		if (_entities[i]->type.isDetermined()) {
            int index = _entities[i]->type.getNumber();
			_entitiesByType[index].add(_entities[i]);
		}
		else {
			_undetEntities.add(_entities[i]);
		}
	}

}

const wchar_t* EntitySet::XMLIdentifierPrefix() const {
	return L"entityset";
}

/** An EntitySet is a collection of Entities.  But in addition to that, it 
  * contains some extra variables, none of which need to be serialized:
  * 
  *   - entitiesByType, undetEntities: pre-indexed subsets of the entity list.
  *   - prevMentionSets, curMentionSets: copies of (or pointers to) the
  *     mention sets of the individual sentences.  I *believe* that these
  *     should always be direct (unmodified) copies of the mentions that
  *     appear in the sentence; therefore, we don't need to serialize them.
  *   - _nSentences: the number of sentences in the document (used to allocate
  *     the right amount of space for prevMentionSets)
  *   - _nPrevMentionSets: the number of prevMentionSets that have been
  *     filled in with meaningful values.
  */
void EntitySet::saveXML(SerifXML::XMLTheoryElement entitysetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("EntitySet::saveXML", "Expected context to be NULL");
	entitysetElem.setAttribute(X_score, _score);
	for (int i = 0; i < getNEntities(); ++i) {
		entitysetElem.saveChildTheory(X_Entity, _entities[i], this); 
		if (_entities[i]->ID != i)
			throw InternalInconsistencyException("EntitySet::saveXML", 
				"Unexpected Entity::ID value");
	}
}

EntitySet::EntitySet(SerifXML::XMLTheoryElement entitySetElem, const std::vector<MentionSet*> &sentenceMentionSets)
: _nSentences(static_cast<int>(sentenceMentionSets.size())), _nPrevMentionSets(0), _prevMentionSets(0), _currMentionSet(0)
{
	using namespace SerifXML;
	entitySetElem.loadId(this);
	_score = entitySetElem.getAttribute<float>(X_score, 0);

	// Load the entities, and populate the _entitiesByType and
	// _undetEntities arrays.
	XMLTheoryElementList entityElems = entitySetElem.getChildElementsByTagName(X_Entity);
	int n_entities = static_cast<int>(entityElems.size());
	_entities.setLength(n_entities);
	_entitiesByType = _new GrowableArray <Entity *>[EntityType::getNTypes()];
	for (int i = 0; i < n_entities; i++) {
		Entity *newEntity = _new Entity(entityElems[i], i);
		_entities[i] = newEntity;
		if (newEntity->type.isDetermined()) {
            int index = newEntity->type.getNumber();
			_entitiesByType[index].add(newEntity);
		} else {
			_undetEntities.add(newEntity);
		}
	}

	// Initialize pointers to mention sets.
	_prevMentionSets = new MentionSet*[_nSentences];
	for (int i=0; i<_nSentences; ++i) {
		if (i < _nSentences-1)
			loadDoNotCopyMentionSet(sentenceMentionSets[i]);
		else
			loadMentionSet(sentenceMentionSets[_nSentences-1]);
	}
}
void EntitySet::populateEntityByMentionTable(){
	for (int i=0; i < _entities.length(); i++) {
		Entity* ent = _entities[i];
		GrowableArray<MentionUID> &ments = ent->mentions;		
		for (int j=0; j < ments.length(); j++) {
			_entityByMention[ments[j]] = ent; 
		}
	}
}
void EntitySet::clearEntityByMentionTable(){
	_entityByMention.clear();
}

const Entity* EntitySet::lookUpEntityForMention(MentionUID uid){
	if(_entityByMention.size() == 0)
		populateEntityByMentionTable();
	if(_entityByMention.find(uid) == _entityByMention.end())
		return 0;
	else
		return _entityByMention[uid];
}

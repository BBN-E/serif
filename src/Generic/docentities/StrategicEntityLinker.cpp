// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/docentities/StrategicEntityLinker.h"

#include <wchar.h>
#include <string>
using namespace std;

StrategicEntityLinker::StrategicEntityLinker() : _debug(L"strategic_entity_linker_debug")
{
	_linkToNames = false;
	_doStrategicLinking = ParamReader::getRequiredTrueFalseParam("do_strategic_linking");
	if (_doStrategicLinking) {
		_linkToNames = ParamReader::isParamTrue("do_strategic_linking_to_name");
	}

	_singletonDescriptorEntityPercentage = 0;
	// singleton descriptor percentage: what percent of all entities may have single mentions
	// and only be descriptors? If 0, strategic linking will perform fully. If 100, strategic
	// linking will not perform at all. If in between, the amount of processing will depend on
	// the number of entities that already exist
	if (_doStrategicLinking) {
		_singletonDescriptorEntityPercentage = ParamReader::getRequiredFloatParam("singleton_descriptor_percentage");
		if (_singletonDescriptorEntityPercentage < 0 || _singletonDescriptorEntityPercentage > 100)
			throw UnexpectedInputException(
			"StrategicEntityLinker::StrategicEntityLinker()",
			"parameter 'singleton_descriptor_percentage' must range from 0 to 100");
	}

	// these parameters are only required if strategic linking is on and names aren't being done

	// singleton merge size: how many individual entities should merge into a single?
	// we had been doing simply pairs, but this allows 3 or more entities to combine.
	// for obvious reasons, the number cannot be less than 2
	_singletonMergeSize = 2;
	if (_doStrategicLinking && !_linkToNames) {
		_singletonMergeSize = ParamReader::getRequiredIntParam("singleton_merge_size");
		if (_singletonMergeSize < 2)
			throw UnexpectedInputException(
			"StrategicEntityLinker::StrategicEntityLinker()",
			"parameter 'singleton_merge_size' must be 2 or greater");
	}

}


StrategicEntityLinker::~StrategicEntityLinker() {}


// master method for all the types of linking that might go on
void StrategicEntityLinker::linkEntities(DocTheory* docTheory)
{
	if (_doStrategicLinking) {
		if (_linkToNames)
			_linkToNamesStrategically(docTheory);
		else
			_linkStrategically(docTheory);
    }
	if (docTheory->getEntitySet() != NULL)
		docTheory->getEntitySet()->determineMentionConfidences(docTheory);
}

// entities that are singleton descriptors are linked in sets of size _singletonMergeSize
void StrategicEntityLinker::_linkStrategically(DocTheory* docTheory)
{
	// since we only have one technique right now, it's all coded in this
	// method. If we expand, this method should call other methods instead.
	const EntitySet* ents = docTheory->getEntitySet();

	if (ents == 0) {
		return;
	}
	Entity** linkableEnts = _new Entity*[ents->getNEntities()];
	int nextEnt = 0;
	// candidate entities have exactly one descriptor mention
	int i;
	for (i=0; i < ents->getNEntities(); i++) {
		Entity* ent = ents->getEntity(i);
		if (ent->getNMentions() != 1)
			continue;
		Mention* ment = ents->getMention(ent->getMention(0));
		if (ment->mentionType != Mention::DESC)
			continue;
		linkableEnts[nextEnt++] = ent;
	}
	int maximumSingleton = (int)(_singletonDescriptorEntityPercentage*.01*ents->getNEntities());
	int currentSingleton = nextEnt;
	_debug << "Singleton descriptor percentage: " << _singletonDescriptorEntityPercentage << "\n";
	_debug << "Singleton merge size: " << _singletonMergeSize << "\n";
	_debug << "Total Entities: " << ents->getNEntities() << "\n";
	_debug << "Total singletons (before): " << currentSingleton << "\n";
	_debug << "Total singletons allowed: " << maximumSingleton << "\n";
	// now traverse through the entities. upon encountering an entity, link it to up to
	// _singletonMergeSize-1 other entities of the same
	// type together. Do this by adding the mention from the latter entities to the first,
	// then removing the latter entities from the list so they doesn't get looked at again.
	// we're actually only going to simulate the merge by grouping the set of entities to be
	// merged in an array and taking them out of the running in the linkableEnts array. After
	// the merge sets have been determined, we'll choose which merge sets to actually process
	Entity*** mergeSet = _new Entity**[nextEnt];
	for (i=0; i < nextEnt; i++)
		mergeSet[i] = NULL;
	int mergeSetSize = 0;
	for (i=0; i < nextEnt; i++) {
		if (linkableEnts[i] == NULL)
			continue;
		Entity** currentMergeSet = _new Entity*[_singletonMergeSize];
		currentMergeSet[0] = linkableEnts[i];
		int k;
		for (k = 1; k < _singletonMergeSize; k++)
			currentMergeSet[k] = NULL;
		k = 1;
		int j;
		for (j=i+1; j < nextEnt && k < _singletonMergeSize; j++) {
			if (linkableEnts[j] == NULL)
				continue;
			if (linkableEnts[j]->getType() == currentMergeSet[0]->getType()) {
				currentMergeSet[k++] = linkableEnts[j];
				linkableEnts[j] = NULL;
			}
		}
		if (k > 1)
			mergeSet[mergeSetSize++] = currentMergeSet;
		else
			delete [] currentMergeSet;
	}
	// Now process the mergeSets, stopping once there aren't too many singletons
	// or once the mergeSets have all been processed.
	while (currentSingleton > maximumSingleton) {
		// the rule of what to merge: entities with the shortest total mention
		// length go first. A helper method will take care of the counting. Rather
		// than sort, we'll just process the set of mergeSets (laziness!)
		size_t minimumSize = 0;
		int minimumIndex = -1;
		for (i = 0; i < mergeSetSize; i++) {
			if (mergeSet[i] == NULL)
				continue;
			size_t size = _getMergeSetMentionLength(mergeSet[i], _singletonMergeSize, ents);
			if (minimumIndex < 0 || size < minimumSize) {
				minimumIndex = i;
				minimumSize = size;
			}
		}
		// no change to minimumIndex means we're out of mergeSets
		if (minimumIndex < 0)
			break;
		// otherwise, process the mergeSet
		Entity** currSet = mergeSet[minimumIndex];
		// remove the mergeSet from further consideration
		mergeSet[minimumIndex] = NULL;
		// we lose a singleton for the first entity
		// all other entities get merged to the first one
		currentSingleton--;
		for (int m = 1; m < _singletonMergeSize; m++) {
			if (currSet[m] == NULL)
				break;
			currSet[0]->addMention(currSet[m]->getMention(0));
			// remove the mention from its former entitiy
			currSet[m]->mentions.removeLast();
			// fewer singletons now
			currentSingleton --;
			// the entity might not get totally merged, which is fine
			if (currentSingleton <= maximumSingleton)
				break;
		}
		// clean up
		delete [] currSet;
	}
	// clean up the merge sets that weren't used
	for (i = 0; i < mergeSetSize; i++) {
		if (mergeSet[i] != NULL)
			delete [] mergeSet[i];
	}
	// and finally clean up the master
	delete [] mergeSet;
	_debug << "Total singletons after: " << currentSingleton << "\n";
	delete [] linkableEnts;
}



// TODO: rather than be a duplicate of linkStrategically, this should be
// a branch under _linkStrategically. But I'm in a hurry.
// merge size is irrelevant here, since we're linking to established name entities
void StrategicEntityLinker::_linkToNamesStrategically(DocTheory* docTheory)
{
	// since we only have one technique right now, it's all coded in this
	// method. If we expand, this method should call other methods instead.
	const EntitySet* ents = docTheory->getEntitySet();

	if (ents == 0) {
		return;
	}
	Entity** linkableEnts = _new Entity*[ents->getNEntities()];
	int nextEnt = 0;
	// candidate source entities have exactly one descriptor mention
	int i;
	for (i=0; i < ents->getNEntities(); i++) {
		Entity* ent = ents->getEntity(i);
		if (ent->getNMentions() != 1)
			continue;
		Mention* ment = ents->getMention(ent->getMention(0));
		if (ment->mentionType != Mention::DESC)
			continue;
		_debug << "Adding " << ment->getNode()->toDebugTextString().c_str() << " as singleton\n";
		linkableEnts[nextEnt++] = ent;
	}
	int maximumSingleton = (int)(_singletonDescriptorEntityPercentage*.01*ents->getNEntities());
	int currentSingleton = nextEnt;
	_debug << "Singleton descriptor percentage: " << _singletonDescriptorEntityPercentage << "\n";
	_debug << "Total Entities: " << ents->getNEntities() << "\n";
	_debug << "Total singletons (before): " << currentSingleton << "\n";
	_debug << "Total singletons allowed: " << maximumSingleton << "\n";

	// now traverse through the entities. upon encountering an entity, link it to
	// some name entity. First try and find a singleton. Finding none, pick a name
	// entity with the fewest total mentions

	// NOTE: The paragraph below is not necessarily applicable!
	// Do this by adding the mention from the latter entities to the first,
	// then removing the latter entities from the list so they doesn't get looked at again.
	// we're actually only going to simulate the merge by grouping the set of entities to be
	// merged in an array and taking them out of the running in the linkableEnts array. After
	// the merge sets have been determined, we'll choose which merge sets to actually process

	// rudimentary tuning stuff
	// the modulus stuff spaces the entities to be linked out.
	int num_to_remove = currentSingleton-maximumSingleton;
	if (num_to_remove > 0) {
		int modulus = nextEnt/num_to_remove;
		while (currentSingleton > maximumSingleton && modulus > 0) {
			for (i=0; i < nextEnt; i++) {
				if (currentSingleton <= maximumSingleton)
					break;
				if (i%modulus != 0)
					continue;
				if (linkableEnts[i] == NULL)
					continue;
				// find a candidate target entity - fewest mentions with a name
				Entity* target = _getSmallestNameEntity(ents, linkableEnts[i]->getType());
				if (target == 0) {
					_debug << "No valid name entities for " << ents->getMention(linkableEnts[i]->getMention(0))->getNode()->toDebugTextString().c_str() << "!\n";
					continue;
				}
				// merge it
				target->addMention(linkableEnts[i]->getMention(0));
				// remove it from the previous entity
				linkableEnts[i]->mentions.removeLast();
				linkableEnts[i] = 0;
				currentSingleton--;
			}
			if (currentSingleton <= maximumSingleton)
				break;
			modulus--;
		}
	}
	delete [] linkableEnts;
}


// return a name entity with fewest possible mentions
Entity* StrategicEntityLinker::_getSmallestNameEntity(const EntitySet* ents, EntityType type) {
	int smallest = 0;
	Entity* currEnt = 0;
	for (int i=0; i < ents->getNEntities(); i++) {
		Entity* ent = ents->getEntity(i);
		if (ent->getType() != type)
			continue;
		if (smallest > 0 && ent->getNMentions() >= smallest)
			continue;
		bool seenName = false;
		for (int j=0; j < ent->getNMentions(); j++) {
			Mention* ment = ents->getMention(ent->getMention(0));
			if (ment->mentionType == Mention::NAME) {
				seenName = true;
				break;
			}
		}
		if (!seenName)
			continue;
		smallest = ent->getNMentions();
		currEnt = ent;
	}
	return currEnt;
}
size_t StrategicEntityLinker::_getMergeSetMentionLength(Entity** set, size_t size, const EntitySet* ents) {
	size_t lengthTotal = 0;
	for (size_t i = 0; i < size; i++) {
		if (set[i] == NULL)
			break;
		lengthTotal += ents->getMention(set[i]->getMention(0))->getNode()->toTextString().length();
	}
	return lengthTotal;
}

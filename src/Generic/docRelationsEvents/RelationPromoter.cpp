// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/RelationPromoter.h"
#include "Generic/theories/Relation.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/relations/RelationUtilities.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/Entity.h"

RelationPromoter::RelationPromoter() {
	_debugStream.init(Symbol(L"relpromoter_debug"));

	ADD_IMAGINARY_RELATIONS = ParamReader::isParamTrue("add_imaginary_relations");
	if (ADD_IMAGINARY_RELATIONS)	{
		_imaginaryRelationType = ParamReader::getParam(Symbol(L"imaginary_relation_type"));
		if (_imaginaryRelationType.is_null()) {
			throw UnexpectedInputException("RelationPromoter::RelationPromoter()",
				"Parameter 'imaginary_relation_type' not defined");
		} else if (RelationTypeSet::isNull(RelationTypeSet::getTypeFromSymbol(_imaginaryRelationType)))
		{
			throw UnexpectedInputException("RelationPromoter::RelationPromoter()",
				"Parameter 'imaginary_relation_type' specifies symbol not in relation type set");
		}
        _imaginary_threshold = ParamReader::getRequiredIntParam("imaginary_threshold");
		if (_imaginary_threshold < 0 || _imaginary_threshold > 100) {
			throw UnexpectedInputException("RelationPromoter::RelationPromoter()",
				"Parameter 'imaginary_threshold' must be in range 0-100");
		}

	}
	_merge_across_type = ParamReader::getRequiredTrueFalseParam("merge_rel_mentions_across_type");
	_allow_events_to_subsume_relations = ParamReader::getRequiredTrueFalseParam("allow_events_to_subsume_relations");
	_ignore_relations_with_matching_heads = ParamReader::getRequiredTrueFalseParam("ignore_relations_with_matching_heads");
}

void RelationPromoter::promoteRelations(DocTheory* docTheory) {

	if (docTheory->getNSentences() == 0){
		RelationSet *relSet = _new RelationSet(0);
		docTheory->takeRelationSet(relSet);
		return;
	}

	EntitySet *entitySet = docTheory->getEntitySet();
	EventSet *eventSet = docTheory->getEventSet();
	_n_relations = 0;

	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		const RelMentionSet *relMentionSet
			= docTheory->getSentenceTheory(sentno)->getRelMentionSet();
		for (int i = 0; i < relMentionSet->getNRelMentions(); i++) {
			RelMention *relMention = relMentionSet->getRelMention(i);
			if (_debugStream.isActive())
				_debugStream << relMention->toString() << L"\n";
			if (relMention->getType() == RelationConstants::IDENTITY)
				continue;
			if (isSubsumedUnderEvent(eventSet, relMention->getLeftMention(),
				relMention->getRightMention()))
			{
				_debugStream << L" -- subsumed!\n";
				continue;
			}
			Entity *leftEntity
				= entitySet->getEntityByMention(relMention->getLeftMention()->getUID());
			Entity *rightEntity
				= entitySet->getEntityByMention(relMention->getRightMention()->getUID());
			if (leftEntity == 0 || rightEntity == 0) {
				_debugStream  << L"DELETED -- ENTITY(IES) IS(ARE) NULL!\n";
				continue;
			}
			if (leftEntity == rightEntity) {
				_debugStream  << L"DELETED -- MENTIONS CO-REFER!\n";
				continue;
			}
			int leftid = leftEntity->getID();
			int rightid = rightEntity->getID();
			Relation *rel = findRelationInList(leftid, rightid);
			if (rel != 0 && (_merge_across_type || rel->getType() == relMention->getType())) {
				if (rel->getType() != relMention->getType()) {
					_debugStream  << L"CHANGING TYPE TO MATCH EARLIER MENTION!\n";
					// fix me later?
				}
				rel->addRelMention(relMention);
			} else {
				if (_ignore_relations_with_matching_heads &&
					!RelationUtilities::get()->isATEARelationType(relMention->getType()) &&
					findRelationWithSameHeads(relMention->getLeftMention(),
					relMention->getRightMention()))
				{
					_debugStream << L" -- ignored since same headwords as some other relation!\n";
					continue;
				}
				if (_n_relations < MAX_DOCUMENT_RELATIONS) {
					rel = _new Relation(relMention, leftid, rightid, _n_relations);
					_relationList[_n_relations] = rel;
					_n_relations++;
				}
			}
		}
	}

	RelMentionSet *set = docTheory->getDocumentRelMentionSet();

	if (set) {
		_debugStream << "Starting Document level relations\n";
		for (int i = 0; i < set->getNRelMentions(); i++) {
			RelMention *relMention = set->getRelMention(i);
			if (_debugStream.isActive())
				_debugStream << relMention->toString() << L"\n";
			if (relMention->getType() == RelationConstants::IDENTITY)
				continue;
			if (isSubsumedUnderEvent(eventSet, relMention->getLeftMention(),
				relMention->getRightMention()))
			{
				_debugStream << L" -- subsumed!\n";
				continue;
			}
			Entity *leftEntity
				= entitySet->getEntityByMention(relMention->getLeftMention()->getUID());
			Entity *rightEntity
				= entitySet->getEntityByMention(relMention->getRightMention()->getUID());
			if (leftEntity == 0 || rightEntity == 0) {
				_debugStream  << L"DELETED -- ENTITY(IES) IS(ARE) NULL!\n";
				continue;
			}
			if (leftEntity == rightEntity) {
				_debugStream  << L"DELETED -- MENTIONS CO-REFER!\n";
				continue;
			}
			int leftid = leftEntity->getID();
			int rightid = rightEntity->getID();
			Relation *rel = findRelationInList(leftid, rightid);
			if (rel != 0 && (_merge_across_type || rel->getType() == relMention->getType())) {
				if (rel->getType() != relMention->getType()) {
					_debugStream  << L"CHANGING TYPE TO MATCH EARLIER MENTION!\n";
					// fix me later?
				}
				rel->addRelMention(relMention);
			} else {
				if (_n_relations < MAX_DOCUMENT_RELATIONS) {
					rel = _new Relation(relMention, leftid, rightid, _n_relations);
					_relationList[_n_relations] = rel;
					_n_relations++;
				}
			}
		}
	}

	if (ADD_IMAGINARY_RELATIONS)
		addImaginaryRelations(entitySet, docTheory->getNSentences());

	RelationSet *relSet = _new RelationSet(_n_relations);
	for (int j = 0; j < _n_relations; j++) {
		_relationList[j]->setModalityFromMentions();
		_relationList[j]->setTenseFromMentions();
		relSet->takeRelation(j, _relationList[j]);
		_relationList[j] = 0;
	}
	docTheory->takeRelationSet(relSet);

}

Relation *RelationPromoter::findRelationInList(int leftid, int rightid) {
	for (int i = 0; i < _n_relations; i++) {
		if (_relationList[i]->getLeftEntityID() == leftid &&
			_relationList[i]->getRightEntityID() == rightid)
			return _relationList[i];
	}
	return 0;
}

Relation *RelationPromoter::findRelationWithSameHeads(const Mention *leftMention,
		const Mention *rightMention)
{
	Symbol leftHead = leftMention->getNode()->getHeadWord();
	Symbol rightHead = rightMention->getNode()->getHeadWord();
	for (int i = 0; i < _n_relations; i++) {
		const Relation::LinkedRelMention *lrm = _relationList[i]->getMentions();
		while (lrm != 0) {
			Symbol leftRelHead = 
				lrm->relMention->getLeftMention()->getNode()->getHeadWord();
			Symbol rightRelHead =
				lrm->relMention->getRightMention()->getNode()->getHeadWord();
			
			if (!RelationUtilities::get()->isATEARelationType(lrm->relMention->getType()) &&
			    ((leftRelHead == leftHead && rightRelHead == rightHead) ||
				 (rightRelHead == leftHead && leftRelHead == rightHead)))
			{
				return _relationList[i];
			}
			lrm = lrm->next;
		}
	}
	return 0;
}

bool RelationPromoter::isSubsumedUnderEvent(EventSet *eventSet,
											const Mention *leftMention,
											const Mention *rightMention)
{
	if (!_allow_events_to_subsume_relations)
		return false;

	for (int i = 0; i < eventSet->getNEvents(); i++) {
		Event *e = eventSet->getEvent(i);
		Event::LinkedEventMention *lem = e->getEventMentions();
		while (lem != 0) {
			EventMention *em = lem->eventMention;
			bool left_matched = false;
			bool right_matched = false;
			for (int j = 0; j < em->getNArgs(); j++) {
				const Mention* ment = em->getNthArgMention(j);
				if (ment == leftMention)
					left_matched = true;
				else if (ment == rightMention)
					right_matched = true;
			}
			if (left_matched && right_matched)
				return true;
			lem = lem->next;
		}
	}

	return false;
}

void RelationPromoter::addImaginaryRelations(const EntitySet *entitySet,
											 int n_sentences)
{
	Entity *imaginaryCandidates[10];
	int num_imaginary_cands = 0;

	for (int i = 0; i < entitySet->getNEntities(); i++) {
		Entity *entity = entitySet->getEntity(i);
		if (entity->getNMentions() > _imaginary_threshold &&
			num_imaginary_cands < 10)
		{
			imaginaryCandidates[num_imaginary_cands++] = entity;
		}
	}

	int count = 0;
	for (int j = 0; j < num_imaginary_cands - 1; j++) {
		addImaginaryRelation(entitySet, imaginaryCandidates[j],
			imaginaryCandidates[j+1], n_sentences, count);
	}

}

void RelationPromoter::addImaginaryRelation(const EntitySet *entitySet,
											Entity *entity1, Entity *entity2,
											int sent, int id) {
	if (_n_relations < MAX_DOCUMENT_RELATIONS) {
		_debugStream << "Adding imaginary relation between: ";

		Mention* leftMention = entitySet->getMention(entity1->mentions[0]);

		if (_debugStream.isActive())
			_debugStream << leftMention->getNode()->toTextString() << L"\nAND\n";

		Mention* rightMention = entitySet->getMention(entity2->mentions[0]);

		if (_debugStream.isActive())
			_debugStream << rightMention->getNode()->toTextString() << L"\n";

		RelMention *relMention = _new RelMention(leftMention, rightMention,
			_imaginaryRelationType, sent, id, 1);

		Relation *rel = _new Relation(relMention, entity1->getID(),
			entity2->getID(), _n_relations);
		_relationList[_n_relations++] = rel;

	}
}

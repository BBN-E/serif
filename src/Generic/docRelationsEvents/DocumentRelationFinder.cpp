// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Sexp.h"
#include "Generic/relations/RelationFinder.h"
#include "Generic/relations/FamilyRelationFinder.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/docRelationsEvents/RelationTimexArgFinder.h"
#include "Generic/docRelationsEvents/DocumentRelationFinder.h"
#include "Generic/docRelationsEvents/StructuralRelationFinder.h"
#include "Generic/docRelationsEvents/ZonedRelationFinder.h"
#include "Generic/relations/discmodel/PropTreeLinks.h"
#include <boost/scoped_ptr.hpp>


DocumentRelationFinder::DocumentRelationFinder(bool use_correct_relations) 
	: _use_correct_relations(use_correct_relations), _do_family_relations(false), 
	  _do_relation_time_attachment(false), _zonedRelationFinder(0),
      _structuralRelFinder(0), _relationFinder(0), _relationTimexArgFinder(0),
	  _familyRelationFinder(0), _relation_time_attachment_use_primary_parse(false) 
{
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
	if (!_use_correct_relations) {
		SessionLogger::info("SERIF") << "Initializing document-level Relation Finders...\n";
		_structuralRelFinder = StructuralRelationFinder::build();
		_relationFinder = RelationFinder::build();
		_relationFinder->disallowMentionSetChanges();
		if (ParamReader::isParamTrue("do_relation_time_attachment")) {
			_do_relation_time_attachment = true;
			_relationTimexArgFinder = _new RelationTimexArgFinder(RelationTimexArgFinder::DECODE);
			_relation_time_attachment_use_primary_parse = ParamReader::getOptionalTrueFalseParamWithDefaultVal("relation_time_attachment_use_primary_parse", false);
		}
	} 

	_eventRelationPatterns = 0;
	_num_event_rel_patterns = 0;
	_observation = 0;
	std::string buffer = ParamReader::getParam("event_relation_patterns");
	if (!buffer.empty()) {
		boost::scoped_ptr<UTF8InputStream> patterns_scoped_ptr(UTF8InputStream::build(buffer.c_str()));
		UTF8InputStream& patterns(*patterns_scoped_ptr);
		Sexp *sexp = _new Sexp(patterns);
		if (!sexp->isList()) {
			std::stringstream errMsg;
			errMsg << "Ill-formed event relation pattern file:\n" << buffer << "\nSpecified by parameter 'event_relation_patterns'";
			throw UnexpectedInputException("DocumentRelationFinder::DocumentRelationFinder()", errMsg.str().c_str());
		}
		patterns.close();
		_num_event_rel_patterns = sexp->getNumChildren();
		_eventRelationPatterns = _new EventRelationPattern[_num_event_rel_patterns];
		for (int i = 0; i < _num_event_rel_patterns; i++) {
			Sexp *pattern = sexp->getNthChild(i);
			if (!pattern->isList() || pattern->getNumChildren() != 4) {
				std::stringstream errMsg;
				errMsg << "Ill-formed event relation pattern file:\n" << buffer << "\nSpecified by parameter 'event_relation_patterns'";
				throw UnexpectedInputException("DocumentRelationFinder::DocumentRelationFinder()", errMsg.str().c_str());
			}
			_eventRelationPatterns[i].eventType = pattern->getFirstChild()->getValue();
			_eventRelationPatterns[i].firstRole = pattern->getSecondChild()->getValue();
			_eventRelationPatterns[i].secondRole = pattern->getThirdChild()->getValue();
			_eventRelationPatterns[i].relationType = pattern->getNthChild(3)->getValue();
		}

		std::string validation_str = ParamReader::getRequiredParam("relation_validation_str");
		_observation = _new RelationObservation(validation_str.c_str());
	}
	if (ParamReader::isParamTrue("do_specific_family_relations")) {
		_do_family_relations = true;
		_familyRelationFinder = FamilyRelationFinder::build();
	}
	_zonedRelationFinder = ZonedRelationFinder::build();
	_expand_relations_in_list_mentions = ParamReader::isParamTrue("expand_relations_in_list_mentions");
}

DocumentRelationFinder::~DocumentRelationFinder() {
	delete _structuralRelFinder;
	delete _relationFinder;
	delete _zonedRelationFinder;
	delete _relationTimexArgFinder;
	delete _observation;
}

void DocumentRelationFinder::cleanup() {
	delete _observation;
	_observation = 0;
	if (_relationTimexArgFinder)
		_relationTimexArgFinder->cleanup();
	if (_relationFinder)
		_relationFinder->cleanup();	
}

void DocumentRelationFinder::findSentenceLevelRelations(DocTheory *docTheory) {
	std::string param = ParamReader::getParam("event_relation_patterns");
	if (!param.empty()) {
		std::string validation_str = ParamReader::getRequiredParam("relation_validation_str");
		_observation = _new RelationObservation(validation_str.c_str());
	}

	SForestUtilities::initializeMemoryPools(); //once for the document
	for (int sent = 0; sent < docTheory->getNSentences(); sent++) {
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "Finding relation mentions in sentence " << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
#endif
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		RelMentionSet *rmSet = 0;
		PropTreeLinks* ptLinks=_new PropTreeLinks(sent,docTheory);
		RelMention *relment;
		if (_use_correct_relations) {
			if (_use_correct_answers) {
				rmSet = _correctAnswers->correctRelationTheory(sTheory->getPrimaryParse(),
					sTheory->getMentionSet(), sTheory->getEntitySet(),
					sTheory->getPropositionSet(), sTheory->getValueMentionSet(),
					docTheory->getDocument()->getName());
			}
		} else {
			_relationFinder->resetForNewSentence(docTheory, sent);
			rmSet = _relationFinder->getRelationTheory(
				docTheory->getEntitySet(),
				sTheory,
				sTheory->getPrimaryParse(),
				sTheory->getMentionSet(), 
				sTheory->getValueMentionSet(),
				sTheory->getPropositionSet(),
				sTheory->getFullParse(),
				ptLinks);
			if (_do_relation_time_attachment) {
				_relationTimexArgFinder->attachTimeArguments(rmSet, sTheory->getTokenSequence(),
					sTheory->getValueMentionSet(), (_relation_time_attachment_use_primary_parse? sTheory->getPrimaryParse(): sTheory->getFullParse()), 
					sTheory->getMentionSet(), sTheory->getPropositionSet());
			}
			if (_do_family_relations) {
				for (int i = 0; i < rmSet->getNRelMentions(); i++) {
					relment = rmSet->getRelMention(i);
					if (relment->getType() == Symbol(L"PER-SOC.Family"))  {
						relment = _familyRelationFinder->getFixedFamilyRelation(relment, docTheory);
					}		
				}
			}
			if (_expand_relations_in_list_mentions)
				expandRelationsInListMentions(rmSet, docTheory);
		}
		delete ptLinks;

		sTheory->adoptSubtheory(SentenceTheory::RELATION_SUBTHEORY, rmSet);
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "\r                                                                        \r";
#endif
	}
	SForestUtilities::clearMemoryPools();
}

void DocumentRelationFinder::findDocLevelRelations(DocTheory *docTheory) {
	std::string param = ParamReader::getParam("event_relation_patterns");
	if (!param.empty()) {
		std::string validation_str = ParamReader::getRequiredParam("relation_validation_str");
		_observation = _new RelationObservation(validation_str.c_str());
	}

	if (_use_correct_relations) {
		docTheory->takeDocumentRelMentionSet(_new RelMentionSet());
		return;
	}
	// don't want a null pointer
	if(docTheory->getNSentences() == 0){
		docTheory->takeDocumentRelMentionSet(_new RelMentionSet());
		return;
	}

	// it wouldn't be THAT hard to combine this with the stuff below,
	// but for now life is easier to only do one or the other
	if (_structuralRelFinder->isActive()) {
		RelMentionSet *structuralRMSet = _structuralRelFinder->findRelations(docTheory);
		docTheory->takeDocumentRelMentionSet(structuralRMSet);
		return;
	}

	// Run the zoned relation finder, which if disabled will initialize an empty set
	RelMentionSet* result = _zonedRelationFinder->findRelations(docTheory);

	// why go through the loops below for nothing
	if (_num_event_rel_patterns == 0) {
		docTheory->takeDocumentRelMentionSet(result);
		return;
	}
	
	RelMention *relBuffer[MAX_SENTENCE_RELATIONS];
	int n_doc_relations = 0;

	for (int sent = 0; sent < docTheory->getNSentences(); sent++) {		
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		EventMentionSet *emSet = sTheory->getEventMentionSet();
		RelMentionSet *rmSet = sTheory->getRelMentionSet();

		_observation->resetForNewSentence(sTheory->getEntitySet(),
			sTheory->getPrimaryParse(), sTheory->getMentionSet(),
			sTheory->getValueMentionSet(),
			sTheory->getPropositionSet());

		
		for (int e = 0; e < emSet->getNEventMentions(); e++) {
			EventMention *ement = emSet->getEventMention(e);
			for (int p = 0; p < _num_event_rel_patterns; p++) {
				if (ement->getEventType() != _eventRelationPatterns[p].eventType)
					continue;
				const Mention *leftMention = ement->getFirstMentionForSlot(_eventRelationPatterns[p].firstRole);
				if (leftMention == 0)
					continue;
				const Mention *rightMention = ement->getFirstMentionForSlot(_eventRelationPatterns[p].secondRole);
				if (rightMention == 0)
					continue;
				_observation->populate(leftMention->getIndex(), rightMention->getIndex());
				if (!_observation->isValidTag(_eventRelationPatterns[p].relationType))
					continue;
				bool already_found = false;
				for (int r = 0; r < rmSet->getNRelMentions(); r++) {
					RelMention *rment = rmSet->getRelMention(r);
					if ((rment->getLeftMention() == leftMention &&
						 rment->getRightMention() == rightMention) ||
						(rment->getRightMention() == leftMention &&
						 rment->getLeftMention() == rightMention))
					{
						already_found = true;
						break;
					}
				}
				
				if (!already_found) {
					relBuffer[n_doc_relations] = _new RelMention(leftMention, rightMention, 
						 _eventRelationPatterns[p].relationType, docTheory->getNSentences(),
						 n_doc_relations, 1.0);
					n_doc_relations++;
				}
			}
		}
	}

	for (int j = 0; j < n_doc_relations; j++) {
		result->takeRelMention(relBuffer[j]);
		relBuffer[j] = 0;
	}

	if (_expand_relations_in_list_mentions)
		expandRelationsInListMentions(result, docTheory);

	docTheory->takeDocumentRelMentionSet(result);
}

void DocumentRelationFinder::expandRelationsInListMentions(RelMentionSet* rmSet, DocTheory* docTheory) {
	// Check each existing relation for list membership
	int originalNRelMentions = rmSet->getNRelMentions(); // We're not at risk of invalidating iterators, but this avoids trying to expand expanded relations
	for (int r = 0; r < originalNRelMentions; r++) {
		// Get the existing subject and object mentions
		RelMention* rm = rmSet->getRelMention(r);
		const Mention* leftMention = rm->getLeftMention();
		const Mention* rightMention = rm->getRightMention();

		// To avoid ambiguity, only expand when the argument types are different
		if (leftMention->getEntityType() == rightMention->getEntityType() && leftMention->getEntitySubtype() == rightMention->getEntitySubtype())
			continue;

		// Check for list membership of either argument
		expandRelationInListMention(rmSet, rm, leftMention, rightMention, docTheory);
		expandRelationInListMention(rmSet, rm, rightMention, leftMention, docTheory);
	}
}

void DocumentRelationFinder::expandRelationInListMention(RelMentionSet* rmSet, RelMention* rm, const Mention* listMember, const Mention* otherMention, DocTheory* docTheory) {
	// Get the parent list, if any
	const Mention* parent = listMember->getParent();
	if (parent == NULL || parent->getMentionType() != Mention::LIST)
		return;
	const Mention* child = parent->getChild();

	// Check each list member
	while (child != NULL) {
		// Ignore the member that's already in this relation and make sure the type matches
		RelMention* expanded = NULL;
		bool expandedLeft = false;
		if (child != listMember && child->getEntityType() == listMember->getEntityType() && child->getEntitySubtype() == listMember->getEntitySubtype()) {
			// Construct the new relation with this list member taking place of the other list member
			if (otherMention == rm->getLeftMention()) {
				expanded = _new RelMention(otherMention, child, rm->getType(), rm->getUID().sentno(), rmSet->getNRelMentions(), rm->getScore());
			} else {
				expanded = _new RelMention(child, otherMention, rm->getType(), rm->getUID().sentno(), rmSet->getNRelMentions(), rm->getScore());
				expandedLeft = true;
			}
		} else if (child != listMember && child->getEntityType() == otherMention->getEntityType() && child->getEntitySubtype() == otherMention->getEntitySubtype()) {
			// Construct the new relation with this list member taking place of the non-list member; this works around a weird list parse
			if (otherMention == rm->getLeftMention()) {
				expanded = _new RelMention(child, listMember, rm->getType(), rm->getUID().sentno(), rmSet->getNRelMentions(), rm->getScore());
				expandedLeft = true;
			} else {
				expanded = _new RelMention(listMember, child, rm->getType(), rm->getUID().sentno(), rmSet->getNRelMentions(), rm->getScore());
			}
		}
		if (expanded != NULL) {
			// Copy over other relation parameters
			expanded->setModality(rm->getModality());
			expanded->setTense(rm->getTense());
			expanded->setTimeArgument(rm->getTimeRole(), rm->getTimeArgument(), rm->getTimeArgumentScore());

			// Check if this relation already exists; we don't check exactly, just to see if replacement is already in a relation of the type, which might mean we're incorrectly cross-producting
			bool found = false;
			for (int r = 0; r < rmSet->getNRelMentions(); r++) {
				RelMention* extant = rmSet->getRelMention(r);
				if ((extant->getLeftMention() == expanded->getLeftMention() && expandedLeft) ||
					(extant->getRightMention() == expanded->getRightMention() && !expandedLeft) ||
					(extant->getLeftMention()->getEntity(docTheory) == expanded->getLeftMention()->getEntity(docTheory) && expandedLeft) ||
					(extant->getRightMention()->getEntity(docTheory) == expanded->getRightMention()->getEntity(docTheory) && !expandedLeft) ||
					(extant->getLeftMention() == expanded->getLeftMention() && extant->getRightMention() == expanded->getRightMention())) {
					found = true;
					break;
				}
			}

			// Add the relation (which indirectly increments the relation number)
			if (!found) {
				rmSet->takeRelMention(expanded);
			} else {
				delete expanded;
			}
		}
		child = child->getNext();
	}
}

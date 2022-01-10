// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/ParamReader.h"
#include "Generic/docentities/DocEntityLinker.h"
#include "Generic/edt/ReferenceResolver.h"
#include "Generic/docentities/StrategicEntityLinker.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/UTCoref/LinkAllMentions.h"

#include "Generic/edt/MentionGroups/EntitySetBuilder.h"

#include <boost/algorithm/string/predicate.hpp>
#include <wchar.h>
#include <string>
#include <boost/scoped_ptr.hpp>
using namespace std;

DocEntityLinker::DocEntityLinker(bool use_correct_coref) : 
	_referenceResolver(0), _stratLinker(0), _correctAnswers(0),
	_entitySetBuilder(0)
{
	cout << "Initializing document-level Entity Linkers...\n";

	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");

	// Note that for now, sentence-level linking is the default behavior
	_mode = SENTENCE;

	if (use_correct_coref == true)
		_mode = CORRECT_ANSWER;
	else {
		_mode = SENTENCE;
		std::string linking_mode = ParamReader::getParam("entity_linking_mode");
		if (!linking_mode.empty()) {
			if (boost::iequals(linking_mode, "SENTENCE"))
				_mode = SENTENCE;
			else if (boost::iequals(linking_mode, "OUTSIDE"))
				_mode = OUTSIDE;
			else if (boost::iequals(linking_mode, "MENTION_TYPE"))
				_mode = MENTION_TYPE;
			else if (boost::iequals(linking_mode, "MENTION_GROUP"))
				_mode = MENTION_GROUP;
			else {
				throw UnexpectedInputException("DocEntityLinker::DocEntityLinker",
					"Parameter 'entity_linking_mode' must be set to 'SENTENCE' (default), 'OUTSIDE', 'MENTION_TYPE' or 'MENTION_GROUP'");
			}
		}
	}

	if (_mode == SENTENCE) {
		_referenceResolver = _new ReferenceResolver(1);
		_stratLinker = _new StrategicEntityLinker();
	} else if (_mode == OUTSIDE) {
		_outside_coref_directory = ParamReader::getRequiredParam("outside_coref_directory");
	} else	if (_mode == MENTION_TYPE) {
		_second_pass_desc_overgen = ParamReader::getRequiredIntParam("second_pass_coref_overgen_threshhold");
		_second_pass_desc_me_threshold = ParamReader::getRequiredFloatParam("second_pass_coref_maxent_threshhold");
		_referenceResolver = _new ReferenceResolver(1);
		_stratLinker = _new StrategicEntityLinker();
	} else if (_mode == MENTION_GROUP) {
		_entitySetBuilder = _new EntitySetBuilder();
	} else if (_mode == CORRECT_ANSWER) {
		; // nothing to initialize
	}
}

DocEntityLinker::~DocEntityLinker() {
	delete _stratLinker;
	delete _referenceResolver;
	delete _entitySetBuilder;
}

void DocEntityLinker::cleanup() {
	if (_referenceResolver)
		_referenceResolver->cleanup();
}

void DocEntityLinker::linkEntities(DocTheory* docTheory) {
	if(_mode == MENTION_TYPE){
		doMentionTypeSentenceBySentenceCoref(docTheory);
	} else if (_mode == MENTION_GROUP) {
		doMentionGroupCoref(docTheory);
	}else if (_mode == SENTENCE || _mode == CORRECT_ANSWER) {
		doSentenceBySentenceCoref(docTheory);
	} else if (_mode == OUTSIDE) {
		importOutsideCoref(docTheory);
	}
	
	if (ParamReader::isParamTrue("utcoref_enable")) {
	   LinkAllMentions::prepareAndDecode(docTheory);
	}

	if (docTheory->getEntitySet() != NULL)
		docTheory->getEntitySet()->determineMentionConfidences(docTheory);
}

void DocEntityLinker::doMentionGroupCoref(DocTheory* docTheory) {
	docTheory->setEntitySet(_entitySetBuilder->buildEntitySet(docTheory));
}

void DocEntityLinker::doSentenceBySentenceCoref(DocTheory* docTheory) {

	/////
	///// SENTENCE-BY-SENTENCE COREF -- including CASerif style
	/////
	int numSent = docTheory->getNSentences();
	if (_mode != CORRECT_ANSWER)
		_referenceResolver->resetForNewDocument(docTheory);
	EntitySet *esets[1];
	esets[0] = 0;
	int sent;
	for (sent = 0; sent < numSent; sent++) {
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "Processing entities in sentence " << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
#endif
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		//std::cout << sTheory->getTokenSequence()->toDebugString()<<"\n";
		if (_mode == CORRECT_ANSWER) {
			if (_use_correct_answers) {
				esets[0] = _correctAnswers->getEntityTheory(sTheory->getMentionSet(),
					numSent, docTheory->getDocument()->getName());
			}
		} else {
			_referenceResolver->resetForNewSentence(docTheory, sent);
			_referenceResolver->addPartOfSpeechTheory(sTheory->getPartOfSpeechSequence());
			_referenceResolver->getEntityTheories(esets, 1, sTheory->getPrimaryParse(),
				sTheory->getMentionSet(),
				sTheory->getPropositionSet());
		}
		/*
		if( sent == (numSent - 1)){
			int nSubtypes = EntitySubtype::getNSubtypes();
			int* subtypes_array= _new int[nSubtypes];
			for(int i =0; i < nSubtypes; i++){
				subtypes_array[i] =0;
			}
			int* mapped_entities = _new int[esets[0]->getNEntities()];
			for(int i =0; i < esets[0]->getNEntities(); i++){
				mapped_entities[i] = 0;
			}

			for(int i = 0; i <esets[0]->getNEntities(); i++){
				Entity* ent = esets[0]->getEntity(i);
				for(int j = 0; j < ent->getNMentions(); j++){
					Mention* ment = esets[0]->getMention(ent->getMention(j));
					Symbol st = ment->getEntitySubtype().getName();
					Symbol type = ment->getEntityType().getName();
					if(ment->getMentionType() != Mention::Type::NAME){
						continue;
					}

					//per/group subtypes are too specific
					if(type == EntityType::getPERType().getName()){
						continue;
					}
					if(st == EntitySubtype::getUndetType().getName()){
						continue;
					}
					if((ment->getMentionType() == Mention::Type::NAME) &&
						(!(type == EntityType::getPERType().getName()) ||
						(st == EntitySubtype::getUndetType().getName())))
					{


						int st_index = EntitySubtype::getSubtypeIndex(type, st);
						if(subtypes_array[st_index] == 0){
							subtypes_array[st_index] = ment->getUID();
							break;
						}
						else{
							subtypes_array[st_index] = -1;
							break;
						}
					}
				}
			}

			
			for(int i = 0; i <esets[0]->getNEntities(); i++){
				Entity* ent = esets[0]->getEntity(i);
				for(int j = 0; j < ent->getNMentions(); j++){
					Mention* ment = esets[0]->getMention(ent->getMention(j));
					Symbol st = ment->getEntitySubtype().getName();
					Symbol type = ment->getEntityType().getName();
					if(ment->getMentionType() != Mention::Type::DESC){
						continue;
					}

					if(st == EntitySubtype::getUndetType().getName()){
						continue;
					}
					//per/group subtypes are too specific
					if(type == EntityType::getPERType().getName()){
						continue;
					}
					if((ment->getMentionType() == Mention::Type::DESC) &&
						(!(type == EntityType::getPERType().getName()) ||
						(st == EntitySubtype::getUndetType().getName())))
					{
						int st_index = EntitySubtype::getSubtypeIndex(type, st);
						if((subtypes_array[st_index] > 0)) {
							//get possible merge entity
							Entity* oth = esets[0]->getEntityByMention(subtypes_array[st_index]);
							if(oth == 0){
								;
							}
							else if(oth->getID() != ent->getID()){
								mapped_entities[i] = oth->getID();

							}
						}
					}
				}
			}
			for(int i = 0; i <esets[0]->getNEntities(); i++){
				Entity* ent = esets[0]->getEntity(i);
				if(mapped_entities[i] == 0){
					continue;
				}
				Entity* oth = esets[0]->getEntity(mapped_entities[i]);
				bool found_name = false;
				for(int j = 0; j < ent->getNMentions(); j++){
					Mention* ment = esets[0]->getMention(ent->getMention(j));
					if(ment->getMentionType() == Mention::Type::NAME){
						found_name = true;
						break;
					}
				}
				//only link entities w/o names
				if(!found_name){
					std::cout<<"*****Merging Entitites"<<std::endl;
					//add all of i mentions to mapped_entities[i]
					
					for(int k = 0; k < ent->getNMentions(); k++){
						oth->addMention(ent->getMention(k));
						
					}
					int nment = ent->getNMentions()-1;
					for(int k = 0; k <nment; k++){
						ent->mentions.removeLast();
					}
					ent->mentions.removeLast();
				}
				
				
			}
			delete subtypes_array;
			delete mapped_entities;
		}
		*/

		sTheory->adoptSubtheory(SentenceTheory::ENTITY_SUBTHEORY, esets[0]);
		// AND-- this entity set contains a COPY of the old mention set, and the copy
		//  may be different than the original. So we need to adopt the new MentionSet too.
		// BUT-- we need to copy it, because otherwise the reference counting on it will be wrong.

		sTheory->adoptSubtheory(SentenceTheory::MENTION_SUBTHEORY,
			_new MentionSet(*esets[0]->getNonConstLastMentionSet()));

		// We run sentence-level relation finding before linking document entities, to help block
		// certain bad coreference cases. Now that we've updated the MentionSet, those RelMention's
		// left and right Mention pointers are invalid. This normally isn't a problem because the
		// RelMentionSet gets replaced in the next stage, doc-relations-events, but if we stop after
		// doc-entities, SerifXML serialization will fail to generate valid XML IDs for these
		// Mention pointers to freed memory. Thus we want to create an empty RelMentionSet.
		sTheory->adoptSubtheory(SentenceTheory::RELATION_SUBTHEORY, _new RelMentionSet());
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "\r                                                                        \r";
#endif


	}

	if (_mode != CORRECT_ANSWER)
		_referenceResolver->cleanUpAfterDocument();



	// To replace sentence-by-sentence coref, you must create a final EntitySet to be
	//   added to the docTheory, as below. Also, if you change the types of any mentions,
	//   (for example, pronouns), you must propagate those changes to the mentionSet subtheories
	//   of the appropriate sentences. This is necessary so that relation and event finding,
	//   which use mentionSets, can do their jobs correctly.

	if (numSent > 0) {
		EntitySet *latestEntitySet = docTheory->getSentenceTheory(numSent-1)->getEntitySet();
		docTheory->setEntitySet(_new EntitySet(*latestEntitySet));
	} else
		docTheory->setEntitySet(_new EntitySet());

	if (_mode != CORRECT_ANSWER)
		_stratLinker->linkEntities(docTheory);
}

void DocEntityLinker::importOutsideCoref(DocTheory* docTheory) {
	if (docTheory->getNSentences() == 0)
		docTheory->setEntitySet(_new EntitySet());

	std::stringstream outside_coref_file;
	outside_coref_file << _outside_coref_directory << "/" << docTheory->getDocument()->getName().to_debug_string() << ".entities";

	boost::scoped_ptr<UTF8InputStream> outsideCoref_scoped_ptr(UTF8InputStream::build(outside_coref_file.str().c_str()));
	UTF8InputStream& outsideCoref(*outsideCoref_scoped_ptr);

	// create new entitySet with mentionSets all in place, etc.
	EntitySet *latestEntitySet
		= docTheory->getSentenceTheory(docTheory->getNSentences()-1)->getEntitySet();
	EntitySet *entitySet = _new EntitySet(*latestEntitySet);

	Sexp *entitySexp = _new Sexp(outsideCoref);
	while (!entitySexp->isVoid()) {
		if (!entitySexp->isList() || entitySexp->getNumChildren() < 3 ||
			!entitySexp->getFirstChild()->isAtom() ||
			!entitySexp->getSecondChild()->isAtom())
		{
			char message[2000];
			_snprintf(message, 2000, "Ill-formed entity sexp (%s) in file %s",
				entitySexp->to_debug_string().c_str(), outside_coref_file.str().c_str());
			throw UnexpectedInputException("DocEntityLinker::importOutsideCoref", message);
		}
		Symbol idSym = entitySexp->getFirstChild()->getValue();
		Symbol typeSym = entitySexp->getSecondChild()->getValue();
		EntityType etype(typeSym);
		Symbol mentIDSym = entitySexp->getThirdChild()->getValue();
		MentionUID ment_id = MentionUID(atoi(mentIDSym.to_debug_string()));
		entitySet->addNew(ment_id, etype);
		// we absolutely have to pass in the entity type here, because
		// otherwise it's going to try to get it from the mention, which
		// may not have been typed in the mention set yet! (e.g. if it's a pronoun)
		int entity_id = entitySet->getEntityByMention(ment_id, etype)->getID();
		for (int ment = 3; ment < entitySexp->getNumChildren(); ment++) {
			Sexp *mentSexp = entitySexp->getNthChild(ment);
			Symbol mentIDSym = mentSexp->getValue();
			ment_id = MentionUID(atoi(mentIDSym.to_debug_string()));
			entitySet->add(ment_id, entity_id);
		}
		delete entitySexp;
		entitySexp = _new Sexp(outsideCoref);
	}

	// Do we really have to change the types of pronouns in every MentionSet that exists?
	//  -- for now we will, just to be safe
	// TODO: I wonder if to be safe we should change the types of all mentions?
	//       Or does that risk screwing things up even more? (what about metonymy?)
	//
	// Yes, I think we do want to change the types of all mentions.  We shouldn't
	// assume that any outside coref will obey the rule that the initial mention type
	// will match the final entity type.  After all, we sometimes change types
	// when linking. -JSM 01/14/07
	for (int sent = 0; sent < docTheory->getNSentences(); sent++) {
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		MentionSet *mset = sTheory->getMentionSet();
		for (int m = 0; m < mset->getNMentions(); m++) {
			Mention *ment = mset->getMention(m);
			
			Entity *entity = entitySet->getEntityByMentionWithoutType(ment->getUID());
			EntityType newType = EntityType::getOtherType();
			if (entity != 0) {
				newType = entity->getType();
			}
			// from MentionSet in docTheory
			ment->setEntityType(newType);
			// from doc-level EntitySet's MentionSet
			entitySet->getMention(ment->getUID())->setEntityType(newType);
			// from each sent-level EntitySet's MentionSet
			for (int future_sent = sent; future_sent < docTheory->getNSentences(); future_sent++) {
				SentenceTheory *sentLevelTheory = docTheory->getSentenceTheory(future_sent);
				sentLevelTheory->getEntitySet()->getMention(ment->getUID())->setEntityType(newType);
			}
		}
	}
	docTheory->setEntitySet(entitySet);
}


/* This is an attempt at a more 'document level' coref scheme using the existing models
	It has only been tested in ARABIC with the P1DescClassifier and only in standard SERIF (not CASERIF) mode.  
	The general scheme is run NAME coref on all sentences, 
	then run DESC coref on all sentences, 
	then run DESC coref a second time but overgenerate links.  
	Then make prelinks, 
	and finally run Pronoun coref. 
    
	For Arabic 2005, this has the rare benefit of increasing both EDR and RDR scores, so it is being checked in.
	But it is using several Serif data structures in unusual ways (it will cause empty entities), for true document
	Coref, we should probably rewrite the whole tree link structure.  

	Also of note, DocLevelCoref invalidates EntitySet->getLastMentionSet(), I've fixed this in the Arabic/P1/ path, 
	(use EntitySet->getMentionSet(sent_num) instead) but if you use this, you'll need to fix it elsewhere.  
*/
void  DocEntityLinker::doMentionTypeSentenceBySentenceCoref(DocTheory* docTheory){
	int numSent = docTheory->getNSentences();
	int sent;
	EntitySet* prevSet = 0;
	EntitySet *esets[1];
	esets[0] = 0;
	_referenceResolver->resetForNewDocument(docTheory);
	for (sent = 0; sent < numSent; sent++) {
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "Processing entities in sentence (linking Names)" << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
#endif
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		_referenceResolver->resetWithPrevEntitySet(prevSet);
		_referenceResolver->addNamesToEntitySet(esets, 1, sTheory->getPrimaryParse(),
				sTheory->getMentionSet(),
				sTheory->getPropositionSet());
		prevSet = esets[0];
	}
	//fix the entity sets for naems
	_referenceResolver->postLinkSpecialNameCases(esets, 1);
	prevSet = esets[0];
	//prevSet = esets[0];
	for (sent = 0; sent < numSent; sent++) {
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "Processing entities in sentence (linking Descriptors)" << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
#endif
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		_referenceResolver->resetWithPrevEntitySet(prevSet);
		_referenceResolver->addDescToEntitySet(esets, 1, sTheory->getPrimaryParse(),
				sTheory->getMentionSet(),
				sTheory->getPropositionSet());
		prevSet = esets[0];
	}
	for (sent = 0; sent < numSent; sent++) {
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "Processing entities in sentence (linking Descriptors pt2)" << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
#endif
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		_referenceResolver->resetWithPrevEntitySet(prevSet);
		_referenceResolver->readdSingletonDescToEntitySet(esets, 1, sTheory->getPrimaryParse(),
				sTheory->getMentionSet(),
				sTheory->getPropositionSet(), _second_pass_desc_overgen, _second_pass_desc_me_threshold);
		prevSet = esets[0];
	}
	for (sent = 0; sent < numSent; sent++) {
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "Processing entities in sentence (linking Pronouns)" << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
#endif
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		Parse* prev_parse = 0;
		if(sent > 0){
			prev_parse = docTheory->getSentenceTheory(sent -1)->getPrimaryParse();
		}
		_referenceResolver->resetWithPrevEntitySet(prevSet, prev_parse);
		if(sTheory->getPartOfSpeechSequence() == 0){
			std::cout<<"pos theory i 0"<<std::endl;
		}
		else{
			const PartOfSpeechSequence* pos =sTheory->getPartOfSpeechSequence();
		}

		_referenceResolver->addPartOfSpeechTheory(sTheory->getPartOfSpeechSequence());
		_referenceResolver->addPronToEntitySet(esets, 1, sTheory->getPrimaryParse(),
				sTheory->getMentionSet(),
				sTheory->getPropositionSet());
		prevSet = esets[0];
//
		//B/C the relation finder is using the ENTITY_SET, it must be adopted here.  This entiy set will
		//include names and descriptors (but not pronouns) that come after the sentence in question....
		//sTheory->adoptSubtheory(SentenceTheory::ENTITY_SUBTHEORY, esets[0]); 
		//The pronoun linker changes the EntitySet's copy of the mention set, this needs to be copied 
		//to the mention set so that we have the correct entity types for pronouns
		// AND-- this entity set contains a COPY of the old mention set, and the copy
		//  may be different than the original. So we need to adopt the new MentionSet too.
		// BUT-- we need to copy it, because otherwise the reference counting on it will be wrong.
		sTheory->adoptSubtheory(SentenceTheory::MENTION_SUBTHEORY,
			_new MentionSet(*esets[0]->getNonConstMentionSet(sent)));
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "\r                                                                        \r";
#endif



	}
	if (_mode != CORRECT_ANSWER)
		_referenceResolver->cleanUpAfterDocument();



	// To replace sentence-by-sentence coref, you must create a final EntitySet to be
	//   added to the docTheory, as below. Also, if you change the types of any mentions,
	//   (for example, pronouns), you must propagate those changes to the mentionSet subtheories
	//   of the appropriate sentences. This is necessary so that relation and event finding,
	//   which use mentionSets, can do their jobs correctly.

	if (numSent > 0) {
		EntitySet *latestEntitySet = prevSet;
		docTheory->setEntitySet(_new EntitySet(*latestEntitySet));
	} else
		docTheory->setEntitySet(_new EntitySet());

	if (_mode != CORRECT_ANSWER)
		_stratLinker->linkEntities(docTheory);
}

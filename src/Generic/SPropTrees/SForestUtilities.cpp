// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/SPropTrees/SForestUtilities.h"
#include "Generic/SPropTrees/SPropTree.h"
#include "Generic/SPropTrees/SPropTreeInfo.h"
#include "Generic/common/ParamReader.h"

#include <set>
#include <boost/scoped_ptr.hpp>

using namespace std;

//===========METHODS OF SPropForest======================

void SForestUtilities::initializeMemoryPools() {
	SNodeContent::initializeMemoryPool();
	STreeNode::initializeMemoryPool();
}

void SForestUtilities::clearMemoryPools() {
	SNodeContent::clearMemoryPool();
	STreeNode::clearMemoryPool();
}

//actually there will be only one tree in this forest!
//just don't forget to call SForestUtilities::cleanPropForest(forest) when you're done
size_t SForestUtilities::populatePropForest(const DocTheory* dt, const SentenceTheory* st, const Proposition* prop, SPropForest& forest, SNodeContent::Language lang) {
	if ( ! dt || ! st || ! prop ) return 0;
	PropositionSet* ps=st->getPropositionSet();
	if ( ! ps ) return 0;
	//bool doStemming=ParamReader::isParamTrue(fromSlot ? "use_slot_stemming_for_tree_matching" : "use_doc_stemming_for_tree_matching");
	//bool doSynonyms=ParamReader::isParamTrue(fromSlot ? "use_slot_synonyms_for_tree_matching" : "use_doc_synonyms_for_tree_matching");
	//bool doHypernyms=ParamReader::isParamTrue(fromSlot ? "use_slot_hypernyms_for_tree_matching" : "use_doc_hypernyms_for_tree_matching");
	//bool doHyponyms=ParamReader::isParamTrue(fromSlot ? "use_slot_hyponyms_for_tree_matching" : "use_doc_hyponyms_for_tree_matching");
	ps->fillDefinitionsArray();
	if ( prop && prop->getPredType() == Proposition::MODIFIER_PRED ) return 0;

	SPropTree* ptree = _new SPropTree(dt,st,prop);
	if ( ! ptree ) {
		SessionLogger::info("SERIF") << "failed to create proposition tree for proposition " << prop->toDebugString() 
			<< ") in document" << st->getDocID().to_debug_string() << "\n";
		return 0;
	}
	ptree->setLanguage(lang);
	if ( ParamReader::isParamTrue("resolve_pronouns") )	ptree->resolveAllPronouns();
	ptree->includeModifiers();
	//ptree->collectAllMentions();
	//ptree->expandAllReferences(doStemming,doSynonyms,doHypernyms,doHyponyms);
	SPropTreeInfo ptinfo;
	ptinfo.tree = ptree;
	ptree->getAllEntityMentions(ptinfo.allEntityMentions);
	forest.insert(ptinfo);
	return forest.size();
}

//create a forest of all propositions that can be created from sister mentions of the given mention 
//(mentions of the same entity), in whichever sentence they occur;
//just don't forget to call SForestUtilities::cleanPropForest(forest) when you're done
size_t SForestUtilities::populatePropForest(const DocTheory* dt, const SentenceTheory* st, const Mention* men, SPropForest& forest, SNodeContent::Language lang) {
	if ( ! dt || ! st || ! men ) return 0;
	set<const Mention*> menSet; //all mentions that belong to the same entity (if there's such an entity at all)
	menSet.insert(men);

	const EntitySet* ents;
	const Entity* ent;
	if ( (ents=dt->getEntitySet()) && (ent=ents->getEntityByMention(men->getUID())) ) {
		//if ( Utilities::DEBUG_MODE ) { std::cerr << "GOT ENTITY: "; ent->dump(std::cerr); }
		for ( int i=0; i < ent->getNMentions(); i++ )
			menSet.insert(ents->getMention(ent->getMention(i)));
	}
	set<const Mention*>::const_iterator mci;
	for ( mci = menSet.begin(); mci != menSet.end(); mci++ ) {
		//remember: these mentions don't even have to be in the same sentence as the one we got!!!
		const SentenceTheory* st2;
		PropositionSet* ps2;
		const Mention* men2;
		if ( ! (men2=*mci) ||
			 ! (st2=dt->getSentenceTheory(men2->getSentenceNumber())) ||
			 ! (ps2=st2->getPropositionSet()) ) continue;
		//if ( Utilities::DEBUG_MODE ) std::cerr << "consider synonym: " << men2->getNode()->toDebugTextString() << "\n";
		ps2->fillDefinitionsArray();
		const Proposition* prop2=ps2->getDefinition(men2->getIndex());
		if ( prop2 && prop2->getPredType() == Proposition::MODIFIER_PRED ) continue;

		if ( ! prop2 ) continue;
		//if ( Utilities::DEBUG_MODE ) std::cerr << "\t==> " << prop2->toDebugString() << "\n";
		populatePropForest(dt, st2, prop2, forest, lang);  //again: it's not necessary the sentence we came here with
	}

	return forest.size();
}


size_t SForestUtilities::populatePropForest(const DocTheory* dt, const SentenceTheory* st, SPropForest& propForest, SNodeContent::Language lang) {
	if ( ! st ) return 0;

	PropositionSet* ps=st->getPropositionSet();
	if ( ! ps ) return 0;
	//bool doStemming=ParamReader::isParamTrue(fromSlot ? "use_slot_stemming_for_tree_matching" : "use_doc_stemming_for_tree_matching");
	//bool doSynonyms=ParamReader::isParamTrue(fromSlot ? "use_slot_synonyms_for_tree_matching" : "use_doc_synonyms_for_tree_matching");
	//bool doHypernyms=ParamReader::isParamTrue(fromSlot ? "use_slot_hypernyms_for_tree_matching" : "use_doc_hypernyms_for_tree_matching");
	//bool doHyponyms=ParamReader::isParamTrue(fromSlot ? "use_slot_hyponyms_for_tree_matching" : "use_doc_hyponyms_for_tree_matching");
	int minSize=0; //( fromSlot ) ? 0 : ParamReader::getRequiredIntParam("minimum_proptree_size");
	ps->fillDefinitionsArray();

	int numProps=ps->getNPropositions();
	SPropTree* ptree;
	SPropForest pps; //local forest (to add to the big one that we want to populate)
	//first of all create proposition trees for all propositions in the sentence
	//but keep only prop trees that are NOT part of some other prop tree
	for ( int l=0; l < numProps; l++ ) {
		//std::cerr << "l=" << l << std::endl;
		const Proposition* newProp=ps->getProposition(l);
		//std::wcerr << "\n*** consider prop " << Proposition::getPredTypeString(newProp->getPredType()) << ": "; newProp->dump(std::cerr); std::wcerr << std::endl;
		if ( newProp && newProp->getPredType() == Proposition::MODIFIER_PRED ) continue;
		if ( ! newProp ) continue;

		//!!!!!!!!!!!!!! if ( newProp->getID() != 17 ) continue;
		ptree = _new SPropTree(dt,st,newProp);
		if ( ! ptree ) {
			SessionLogger::info("SERIF") << "failed to create proposition tree for proposition #" << l 
				<< ": (" << ps->getProposition(l)->toDebugString() << ") in document" 
				<< dt->getDocument()->getName().to_debug_string() << "\n";
			continue;
		}
		ptree->setLanguage(lang);
	    ptree->includeModifiers();
		SPropForest::iterator ptsi = pps.begin(), ptsi2;
		while ( ptsi != pps.end() ) {
			ptsi2 = ptsi;
			ptsi++;
			//std::wcerr << "\n--looking at: " << ptsi2->tree->toString() << "\n";
			if ( ptree->find(ptsi2->tree->getHeadProposition()) != ptree->end() ) {
				//std::cerr << "u";
				delete ptsi2->tree;
				pps.erase(ptsi2);
			} else if ( ptsi2->tree->find(newProp) != ptsi2->tree->end() ) {
				//std::cerr << "v";
				delete ptree;
				ptree = 0;
				break;
			}
			//std::cerr << "y";
		}
		if ( ptree ) {
			//std::wcerr << "\nKEEP PROPTREE (for now): " << ptree->toString();
			SPropTreeInfo ptinfo;
			ptinfo.tree = ptree;
			pps.insert(ptinfo);
		}
	} //all propositions
	

	//do some reference resolution job, expand word sets and remove small proposition trees
	SPropForest::iterator ptsi = pps.begin(), ptsi2;
	while ( ptsi != pps.end() ) {
		ptsi2 = ptsi;
		ptsi++;
		//std::wcerr << "consider ptree: " << ptsi2->tree->toString() << "\n";
		if ( ParamReader::isParamTrue("resolve_pronouns") )	ptsi2->tree->resolveAllPronouns();
		if ( minSize && ptsi2->tree->getSize() < minSize ) {
			delete ptsi2->tree;
			pps.erase(ptsi2);
			continue;
		}
		//ptsi2->tree->collectAllMentions();
		//ptsi2->tree->expandAllReferences(doStemming,doSynonyms,doHypernyms,doHyponyms);
		ptsi2->tree->getAllEntityMentions(const_cast<RelevantMentions&>(ptsi2->allEntityMentions));
	}

	/*/!!!!!!!!!!!!!
	for ( ptsi = pps.begin(); ptsi != pps.end(); ptsi++ ) {
	std::wcerr << "STORE: sent=" << ptsi->sentNumber << ": " << ptsi->tree->toString() << "\n";
	ptsi->tree->serialize(std::wcerr,i);
	}*/
	propForest.insert(pps.begin(),pps.end());
	return pps.size();
}


//extract all not proposition trees (that don't contain each other) from a sentence and ADD the to a set
size_t SForestUtilities::getPropForestFromSentence(const DocTheory* dt, const Sentence* sent, SPropForest& extracted, SNodeContent::Language lang) {
	const SentenceTheory* st;
	if ( ! dt || 
		 ! sent || 
		 ! (st=dt->getSentenceTheory(sent->getSentNumber())) ) return 0;

	SPropForest pps;
	populatePropForest(dt, st, pps, lang);
	if ( pps.size() ) {
		for ( SPropForest::iterator pfi=pps.begin(); pfi != pps.end(); pfi++ )
			const_cast<SPropForest::key_type&>(*pfi).sentNumber = sent->getSentNumber();
		extracted.insert(pps.begin(),pps.end());
	}
	return pps.size();
}

//extract all not proposition trees (that don't contain each other) from a sentence and ADD the to a set
size_t SForestUtilities::getPropForestFromDocument(const DocTheory* dt, SPropForest& extracted, SNodeContent::Language lang) {
	size_t total=0;
	for ( int i=0; i < dt->getNSentences(); i++ ) {

		//if ( i != 18 ) continue;	std::cerr << "sent=" << i << "\n";

		const Sentence* sent=dt->getSentence(i);
		total += getPropForestFromSentence(dt, sent, extracted, lang);
		//std::cerr << "+";
	}
	return total;
}



/*
//iterate over all sentences and put together a list of names that occur there
int SForestUtilities::getAllNames(const SPropForest& forest, std::set<Phrase>& collected) {
	int added=0;
	for ( SPropForest::const_iterator pfci=forest.begin(); pfci != forest.end(); pfci++ ) {
		PhrasesInTree allNames;
		pfci->tree->getAllWords(allNames, false, Mention::NAME);
		for ( PhrasesInTree::const_iterator pitci=allNames.begin(); pitci != allNames.end(); pitci++ ) {
			collected.insert(pitci->first);
			added++;
		}
	}
	return added;
}
*/



//extract all proposition trees stored in a file
size_t SForestUtilities::resurectPropForest(const std::string& filename, const DocTheory* dt, SPropForest& forest, SNodeContent::Language lang) {
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	try {
		uis.open(filename.c_str());
	} catch ( UnrecoverableException &e ) { e.getMessage(); }
	if ( ! uis.fail() ) {
		SPropTreeInfo ptin;
		while ( !uis.eof() && SPropTreeInfo::resurect(uis, dt, ptin) ) {
			if ( ptin.tree ) ptin.tree->setLanguage(lang);
			forest.insert(ptin);

			///* wcerr << "*** resurrected: \n";
			//ptin.tree->serialize(std::wcerr,ptin.sentNumber);
			//for ( PhrasesInTree::const_iterator pitci=ptin.allWords.begin(); pitci != ptin.allWords.end(); pitci++ )
			//std::wcerr << pitci->first << " => " << pitci->second;
			//std::cerr << "\n"; */

		}
	}
	return forest.size();
}



//free all memory taken by the set of proposition trees
void SForestUtilities::cleanPropForest(SPropForest& forest) {

	for ( SPropForest::iterator pfi=forest.begin(); pfi != forest.end(); pfi++ ) {
		if ( pfi->tree ) delete pfi->tree;
	}
	forest.clear();
}



void SForestUtilities::printForest(const SPropForest& forest, std::wostream& o, bool printMentionText) {
	for ( SPropForest::const_iterator pfi=forest.begin(); pfi != forest.end(); pfi++ ) {
		if ( pfi->tree ) {
			o << "id=" << pfi->tree->getID() << "\n";
			o << pfi->tree->toString(printMentionText);
		}
	}
}


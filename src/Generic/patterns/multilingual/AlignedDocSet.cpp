// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/Attribute.h"
#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "AlignedDocSet.h"

#include "Generic/state/XMLSerializedDocTheory.h"

AlignedDocSet::AlignedDocSet() { 
	_alignmentTable = boost::make_shared<AlignmentTable>();
}

void AlignedDocSet::garbageCollect() {
	for (LanguageDocumentMap::iterator it = _docMap.begin(); it != _docMap.end(); ++it) {
		delete (*it).second;
	}
}

//load a doc into the doc set
void AlignedDocSet::loadDocument(const LanguageVariant_ptr& languageVariant, std::string& documentFilename) {
	_fileMap[languageVariant] = documentFilename;
	if (!_defaultLV) {
		_defaultLV = languageVariant;
	}
	//ensureLanguageVariantProcessed(languageVariant); // TODO: we'll want to take this out
}

void AlignedDocSet::loadDocTheory(const LanguageVariant_ptr& languageVariant, const DocTheory* docTheory) {
	_fileMap[languageVariant] = "preloaded";
	_docMap[languageVariant] = docTheory;
	if (!_defaultLV) {
		_defaultLV = languageVariant;
	}
}

Symbol AlignedDocSet::getDocumentName() {
	return getDefaultDocTheory()->getDocument()->getName();
}

bool AlignedDocSet::ensureLanguageVariantProcessed(const LanguageVariant_ptr& languageVariant) {
	if (languageVariantProcessed(languageVariant)) {
		return true;
	} else if (languageVariantExists(languageVariant)) {
		if (!languageVariant->getLanguage().is_null()) {
			SerifVersion::setSerifLanguage(LanguageAttribute::getFromString(languageVariant->getLanguage().to_string()));
		}
		std::pair<Document*, DocTheory*> doc_pair = SerifXML::XMLSerializedDocTheory(_fileMap[languageVariant].c_str()).generateDocTheory();
		_docMap[languageVariant] = doc_pair.second;
		//Ensure cached look up of hasName/hasDesc for entities.  If InstanceFinder is modified so that it changes Entities 
		//(e.g move a mention from Ent1 to Ent2), then the cache will need to be cleared.  
		EntitySet* eSet = doc_pair.second->getEntitySet();
		for(int i =0; i< eSet->getNEntities(); i++){
			eSet->getEntity(i)->initializeHasNameDescCache(eSet);
		}
		return true;
	} else {
		return false;
	}
}

//gets the default
const DocTheory* AlignedDocSet::getDefaultDocTheory() { 
	return getDocTheory(_defaultLV);
}

const DocTheory* AlignedDocSet::getDocTheory(const LanguageVariant_ptr& languageVariant) {
	if (ensureLanguageVariantProcessed(languageVariant)) {
		return _docMap[languageVariant];
	} else {
		return NULL;
	}
}


//load an alignment file for said document
void AlignedDocSet::loadAlignmentFile(std::string& alignmentFilename, 
	const LanguageVariant_ptr& sourceLanguageVariant, 
	const LanguageVariant_ptr& targetLanguageVariant) {
		_alignmentTable->loadAlignmentFile(alignmentFilename, sourceLanguageVariant, targetLanguageVariant);
}

void AlignedDocSet::loadAlignmentFile(std::string& alignmentFilename, std::string& alignmentString) {
	_alignmentTable->loadAlignmentFile(alignmentFilename, alignmentString);
}

//get all language variants that exist in this doc set
std::vector<LanguageVariant_ptr> AlignedDocSet::getLanguageVariants() const {
	std::vector<LanguageVariant_ptr> results;
	for (LanguageFileMap::const_iterator it = _fileMap.begin(); it != _fileMap.end(); ++it) {
		results.push_back(it->first);
	}
	return results;
}

std::vector<const DocTheory*> AlignedDocSet::getAlignedDocTheories(const LanguageVariant_ptr& languageVariant) {
	std::vector<const DocTheory*> results;
	BOOST_FOREACH(LanguageVariant_ptr lv, _alignmentTable->getAlignedLanguageVariants(languageVariant)) {
		results.push_back(getDocTheory(lv));
	}
	return results;
}

SentenceTheory* AlignedDocSet::getAlignedSentenceTheory(const LanguageVariant_ptr& sourceLanguageVariant, 
	const LanguageVariant_ptr& targetLanguageVariant, const SentenceTheory* sentence) {
	 
	if (_alignmentTable->areAligned(sourceLanguageVariant,targetLanguageVariant)) {
		int sentno = sentence->getSentNumber();
		const DocTheory* doc = getDocTheory(targetLanguageVariant);
		if (doc) {
			return doc->getSentenceTheory(sentno);
		}
	}
	return NULL;
}

std::vector<SentenceTheory*> AlignedDocSet::getSentenceTheories(int sentno) {
	std::vector<SentenceTheory*> result;
	BOOST_FOREACH(LanguageVariant_ptr lv, getLanguageVariants()) {
		result.push_back(getDocTheory(lv)->getSentenceTheory(sentno));
	}
	return result;
}

std::vector<SentenceTheory*> AlignedDocSet::getAlignedSentenceTheories(const LanguageVariant_ptr& languageVariant, const SentenceTheory* sentence) {
	std::vector<SentenceTheory*> results;
	int sentno = sentence->getSentNumber();
	BOOST_FOREACH(const DocTheory* doc, getAlignedDocTheories(languageVariant)) {
		results.push_back(doc->getSentenceTheory(sentno));
	}
	return results;
}

std::vector<LanguageVariant_ptr> AlignedDocSet::getCompatibleLanguageVariants(const LanguageVariant_ptr& languageVariant, const LanguageVariant_ptr& languageVariantRestriction) const {
	std::vector<LanguageVariant_ptr> results;
	BOOST_FOREACH(LanguageVariant_ptr lv, _alignmentTable->getAlignedLanguageVariants(languageVariant)) {
		if (lv->matchesConstraint(*languageVariantRestriction)) {
			results.push_back(lv);	
		}
	}
	return results;
}

//provide functions to support retrieving aligned elements
std::vector<int> AlignedDocSet::getAlignedTokens(const LanguageVariant_ptr& sourceLanguageVariant, const LanguageVariant_ptr& targetLanguageVariant, 
												 int sentenceNo, int tokenIndex) {
	return _alignmentTable->getAlignment(sourceLanguageVariant, targetLanguageVariant, sentenceNo, tokenIndex);
}

std::set<int> AlignedDocSet::getSynNodeIndicies(const SynNode* synNode) {
	std::set<int> result;
	std::vector<const SynNode*> terminals;
	synNode->getAllTerminalNodes(terminals);
	BOOST_FOREACH(const SynNode* terminal, terminals) {
		result.insert(terminal->getStartToken());
	}
	return result;
}

std::set<int> AlignedDocSet::getSynNodeAlignedIndicies(const LanguageVariant_ptr& sourceLanguageVariant, const LanguageVariant_ptr& targetLanguageVariant, int sentenceNo, const SynNode* synNode) {
	std::set<int> result;

	BOOST_FOREACH(int index, getSynNodeIndicies(synNode)) {
		std::vector<int> alignedIdxs = getAlignedTokens(sourceLanguageVariant,targetLanguageVariant,sentenceNo,index);
		BOOST_FOREACH(int idx, alignedIdxs) {
			if (idx >= 0) {
				result.insert(idx);
			}
		}
	}

	return result;
}

float AlignedDocSet::getIndexOverlap(std::set<int> set1, std::set<int> set2) {
	std::set<int> overlap;
	std::set_intersection(set1.begin(),set1.end(),set2.begin(),set2.end(),
		std::inserter(overlap,overlap.end()));

	if (overlap.size() > 0) {
		return ((float)overlap.size())/(float)(set1.size()+set2.size()-overlap.size());
	} else {
		return 0;
	}
}

/*static int aligned = 0;
static int total = 0;*/

Mention* AlignedDocSet::getAlignedMention(const LanguageVariant_ptr& sourceLanguageVariant, const LanguageVariant_ptr& targetLanguageVariant, const Mention* mention) {
	const SynNode* sourceMentionSynNode = alignHeads() ? mention->getHead() : mention->getNode();

	std::set<int> alignedIdxs = getSynNodeAlignedIndicies(sourceLanguageVariant,targetLanguageVariant,mention->getSentenceNumber(),sourceMentionSynNode);
	SentenceTheory* targetSentence = getDocTheory(targetLanguageVariant)->getSentenceTheory(mention->getSentenceNumber());

	Mention* bestMention = NULL;
	float bestScore = 0;

	for (int i=0;i<targetSentence->getMentionSet()->getNMentions();++i) {
		Mention* curMention = targetSentence->getMentionSet()->getMention(i);
		const SynNode* targetMentionSynNode = alignHeads() ? curMention->getHead() : curMention->getNode();

		std::set<int> mentionIdxs = getSynNodeIndicies(targetMentionSynNode);

		float overlapPercent = getIndexOverlap(alignedIdxs,mentionIdxs);
		if (overlapPercent >= alignmentThreshold() && overlapPercent > bestScore) {
			bestScore = overlapPercent;
			bestMention = curMention;
		}
	}
	/*
	if (bestMention) aligned++;
	total++;
	SessionLogger::info("Alignment") << boost::lexical_cast<std::string>(aligned) << " of " << boost::lexical_cast<std::string>(total);*/

	return bestMention;
}

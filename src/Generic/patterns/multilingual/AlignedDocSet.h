// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ALIGNED_DOC_SET_H
#define ALIGNED_DOC_SET_H

#include <map>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#include "Generic/common/hash_map.h"
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "LanguageVariant.h"
#include "AlignmentTable.h"

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/ConsoleSessionLogger.h"

BSP_DECLARE(AlignedDocSet);

typedef boost::unordered_map<LanguageVariant_ptr,std::string,LanguageVariant::PtrHashKey,LanguageVariant::PtrEqualKey> LanguageFileMap;
typedef boost::unordered_map<LanguageVariant_ptr,const DocTheory*,LanguageVariant::PtrHashKey,LanguageVariant::PtrEqualKey> LanguageDocumentMap;

/** AlignedDocSet is an object for storing all the different language variants for a single document
  * and their alignments. It is lazy loading, meaning that it will only process a document file
  * if/when it needs to. 
  */
class AlignedDocSet {
private:
	LanguageDocumentMap _docMap;
	LanguageFileMap _fileMap;
	AlignmentTable_ptr _alignmentTable;
	LanguageVariant_ptr _defaultLV;

	bool languageVariantExists(const LanguageVariant_ptr& languageVariant) { return _fileMap.find(languageVariant) != _fileMap.end(); }
	bool languageVariantProcessed(const LanguageVariant_ptr& languageVariant) { return _docMap.find(languageVariant) != _docMap.end(); }
	bool ensureLanguageVariantProcessed(const LanguageVariant_ptr& languageVariant);

	bool alignHeads() { return ParamReader::getParam("mention_alignment_type","head") == "head"; }
	double alignmentThreshold() { return ParamReader::getOptionalFloatParamWithDefaultValue("mention_alignment_threshold",1); }

	/***** helper functions for doing alignments *****/
	// this function will take a SynNode and get the aligned token indicies matching that synNode in the other language
	std::set<int> getSynNodeAlignedIndicies(const LanguageVariant_ptr& sourceLangaugeVariant, 
		const LanguageVariant_ptr& targetLanguageVariant, int sentenceNo, const SynNode* synNode);
	// just get the token indicies of the terminals in a syn node;
	std::set<int> getSynNodeIndicies(const SynNode* synNode);
	float getIndexOverlap(std::set<int> set1, std::set<int> set2);

public:
	AlignedDocSet();
	~AlignedDocSet() {}

	void garbageCollect();

	Symbol getDocumentName();

	//load a doc into the doc set
	void loadDocument(const LanguageVariant_ptr& languageVariant, std::string& documentFilename);
	void loadDocTheory(const LanguageVariant_ptr& languageVariant, const DocTheory* docTheory);

	//sets/gets the default language for this set. Defaults to first language in set.
	void setDefaultLanguageVariant(const LanguageVariant_ptr& languageVariant) { _defaultLV = languageVariant; }
	LanguageVariant_ptr getDefaultLanguageVariant() { return _defaultLV; } 

	//gets the default
	const DocTheory* getDefaultDocTheory();
	const std::string getDefaultDocFilename() { return languageVariantExists(_defaultLV) ? _fileMap[_defaultLV] : ""; }

	const DocTheory* getDocTheory(const LanguageVariant_ptr& languageVariant);

	//load an alignment file for said document
	void loadAlignmentFile(std::string& alignmentFilename, 
		const LanguageVariant_ptr& sourceLangaugeVariant, 
		const LanguageVariant_ptr& targetLanguageVariant);

	void loadAlignmentFile(std::string& alignmentFilename, std::string& alignmentString);

	//provide functions to support retrieving aligned elements
	std::vector<LanguageVariant_ptr> getLanguageVariants() const;
	
	std::vector<LanguageVariant_ptr> getAlignedLanguageVariants(const LanguageVariant_ptr languageVariant) { 
		return _alignmentTable->getAlignedLanguageVariants(languageVariant); }

	//get language variants that are aligned to this one and match the restriction
	std::vector<LanguageVariant_ptr> getCompatibleLanguageVariants (const LanguageVariant_ptr& languageVariant,
		const LanguageVariant_ptr& languageVariantRestriction) const;

	std::vector<const DocTheory*> getAlignedDocTheories (const LanguageVariant_ptr& languageVariant);

	std::vector<SentenceTheory*> getAlignedSentenceTheories (const LanguageVariant_ptr& languageVariant, const SentenceTheory* sentence);
	
	std::vector<SentenceTheory*> getSentenceTheories(int sentno);
	
	SentenceTheory* getAlignedSentenceTheory(const LanguageVariant_ptr& sourceLanguageVariant, 
		const LanguageVariant_ptr& targetLanguageVariant, const SentenceTheory* sentence);	

	std::vector<int> getAlignedTokens(const LanguageVariant_ptr& sourceLangaugeVariant, 
		const LanguageVariant_ptr& targetLanguageVariant,
		int sentenceNo, int tokenIndex);

	Mention* getAlignedMention(const LanguageVariant_ptr& sourceLanguageVariant, 
		const LanguageVariant_ptr& targetLanguageVariant, const Mention* mention);

};

#endif

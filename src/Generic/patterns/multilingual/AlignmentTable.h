// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ALIGNMENT_TABLE_H
#define ALIGNMENT_TABLE_H

#include <string>
#include <vector>
#include <sstream>

#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include "LanguageVariant.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

BSP_DECLARE(AlignmentTable);

typedef boost::unordered_map<int,std::vector<int> > TokenAlignmentMap;
typedef boost::unordered_map<int, TokenAlignmentMap> SentenceAlignmentMap;
typedef boost::unordered_map<LanguageVariant_ptr,SentenceAlignmentMap,LanguageVariant::PtrHashKey,LanguageVariant::PtrEqualKey> LanguageAlignmentMap;
typedef boost::unordered_map<LanguageVariant_ptr,LanguageAlignmentMap,LanguageVariant::PtrHashKey,LanguageVariant::PtrEqualKey> DocumentAlignmentMap;

typedef boost::unordered_map<LanguageVariant_ptr,std::string,LanguageVariant::PtrHashKey,LanguageVariant::PtrEqualKey> AlignedLanguageFileMap;
typedef boost::unordered_map<LanguageVariant_ptr,AlignedLanguageFileMap,LanguageVariant::PtrHashKey,LanguageVariant::PtrEqualKey> AlignmentFileMap;

/** AlignmentTable is an objects for storing and retrieving alignment info for documents
  */
class AlignmentTable {
private:
	DocumentAlignmentMap _alignments;
	AlignmentFileMap _alignmentFiles;

	void _loadAlignments(const LanguageVariant_ptr& sourceLanguageVariant, const LanguageVariant_ptr& targetLanguageVariant);

public:
	AlignmentTable() {}

	//load an alignment file for said document
	void loadAlignmentFile(std::string& alignmentFilename, 
		const LanguageVariant_ptr& sourceLanguageVariant, 
		const LanguageVariant_ptr& targetLanguageVariant);

	void loadAlignmentFile(std::string& alignmentFilename, std::string& alignmentString);

	bool areAligned(const LanguageVariant_ptr& sourceLanguageVariant, 
		const LanguageVariant_ptr& targetLanguageVariant) const;

	//provide functions to support retrieving aligned elements
	std::vector<int> getAlignment(const LanguageVariant_ptr& sourceLanguageVariant, 
		const LanguageVariant_ptr& targetLanguageVariant,
		int sentenceNo, int tokenIndex);

	//maybe this should be a set?
	std::vector<LanguageVariant_ptr> getAlignedLanguageVariants(const LanguageVariant_ptr& languageVariant) const;
};



#endif

// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef DOCUMENT_PFEATURE_H
#define DOCUMENT_PFEATURE_H

#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include "common/Symbol.h"

/** A pseudo-feature used to record information about the document that was
  * used to create a feature set.  In particular, it records information 
  * about how well the document matches a query that was used to generate
  * a QueryDocumentSet:
  *
  *   - docType: The source document's type (extracted from metadata)
  *   - docRank: The rank of this document in a QueryDocumentSet.
  *   - tfIdfScore: The TF/IDF score of this document in a QueryDocumentSet.
  *   - dbScore: The database(?) score of this document in a QueryDocumentSet.
  */
class DocumentPFeature : public PseudoPatternFeature {
private:
	DocumentPFeature(Symbol docType, float doc_rank, float tf_idf_score, float db_score, const LanguageVariant_ptr& languageVariant) 
		: PseudoPatternFeature(languageVariant), _docType(docType), _doc_rank(doc_rank), _tf_idf_score(tf_idf_score), _db_score(db_score) {}

	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(DocumentPFeature, Symbol, float, float, float, const LanguageVariant_ptr&);
	
	DocumentPFeature(Symbol docType, float doc_rank, float tf_idf_score, float db_score) 
		: PseudoPatternFeature(), _docType(docType), _doc_rank(doc_rank), _tf_idf_score(tf_idf_score), _db_score(db_score) {}

	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(DocumentPFeature, Symbol, float, float, float);
public:	
	Symbol getDocType() const { return _docType; }
	float getDocRank() const { return _doc_rank; }
	float getTfIdfScore() const { return _tf_idf_score; }
	float getDbScore() const { return _db_score; }
	bool equals(PatternFeature_ptr other);

	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const ;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	DocumentPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	Symbol _docType;
	float _doc_rank;
	float _tf_idf_score;
	float _db_score;
};

#endif

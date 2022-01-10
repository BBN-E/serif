// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_PARSE_LINK_EXTRACTOR_H
#define ES_PARSE_LINK_EXTRACTOR_H

#include "Generic/common/AttributeValuePairExtractor.h"
class Mention;

/** This extractor identifies several syntactic constructions that
  * link two mentions together, including:
  * 
  *  - pseudo-appositive: (**SN** The quarterback (**SN Tom Jones))
  *  - x-and-his-y: (**SN** John and (SN **his** friend))
  *  - x-is-y: (S (**SN** John) is (**SN a great guy))
  *  - paren-def: (**SN** Apple Picking Company (**SN** -lrb- APC -rrb-))
  *
  * In each case, the extractor adds a feature to a "source" mention
  * (the highest or first of the two mentions), pointing at the
  * "target" mention (the mention that should be linked to the source).
  * 
  * A MentionPointerMerger can then be used to merge these mentions.
  */
class SpanishParseLinkExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	SpanishParseLinkExtractor(); 

	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 

	// Feature names:
	static const Symbol PSEUDO_APPOSITIVE;
	static const Symbol X_AND_HIS_Y;
	static const Symbol X_IS_Y;
	static const Symbol PAREN_DEF;

 private:
	const Mention* findPseudoAppositive(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 
	const Mention* findXandHisY(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 

	const Mention* findXisY(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 

	const Mention* findParenDef(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 

	void coerceOtherOrUndet(const Mention& context, const Mention* target);
};

#endif

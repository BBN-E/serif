// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_PERSON_NAME_PARSE_EXTRACTOR_H
#define ES_PERSON_NAME_PARSE_EXTRACTOR_H

#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolHash.h"

#include <boost/scoped_ptr.hpp>

class Mention;

/**
  *  Extracts variants for PER names.  See:
  *   <http://en.wikipedia.org/wiki/Spanish_naming_customs>
  *   <http://en.wikipedia.org/wiki/Hispanic_American_naming_customs>
  *
  * Name pieces:
  *  GIVEN ("nombre") -- given name
  *  SURNAME1 ("primer apellidos") -- first surname (usually father's first surname)
  *  SURNAME2 ("segundo apellidos") -- second surname (usually mother's first surname)
  *
  * All name pieces may be composite.  Composite names are typically linked by 
  * one of: hyphen, "de", "d'", "del", "de le", "de los", "de las"
  *
  * the suffix "h." (son of) is like english "jr."
  * the suffix "-ez" on surnames can also denote "son of"
  *
  * nicknames are usually only used in very informal/familiar environments
  *
  */
class SpanishPersonNameParseExtractor : public AttributeValuePairExtractor<Mention> {
public: 
	SpanishPersonNameParseExtractor(); 


	// Generated features have std::vector<SpanishNameParse> values.
	struct SpanishNameParse {
		SpanishNameParse(const std::vector<Symbol> &words, size_t given_end, size_t surname1_end, Symbol suffix);
		SpanishNameParse();

		SpanishNameParse merge(const SpanishNameParse& other) const;
		bool is_consistent(const SpanishNameParse& other) const;
		bool operator==(const SpanishNameParse& other) const;
		std::wstring toString() const;
		typedef enum {GIVEN=0, SURNAME1=1, SURNAME2=2, SUFFIX=3} Piece;
		Symbol getPiece(Piece p) const { return pieces[p]; }
	private:
		static const size_t NUM_PIECES=(SUFFIX+1);
		Symbol pieces[NUM_PIECES];
		Symbol::HashMap<int> sym2piece;
	};
	typedef std::vector<SpanishNameParse> SpanishNameParseList;
	const static Symbol FEATURE_NAME;
	const static Symbol EXTRACTOR_NAME;

	const static Symbol GIVEN_FEATURE;
	const static Symbol SURNAME1_FEATURE;
	const static Symbol GIVEN_SURNAME1_FEATURE;

	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory); 
};

#endif

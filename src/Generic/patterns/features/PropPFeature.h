// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROP_PFEATURE_H
#define PROP_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include <boost/shared_ptr.hpp>

class Proposition;

/** A feature used to store information about a successful PropPattern match.
  * In addition to the basic PatternFeature information, each PropPFeature 
  * keeps a reference to the Proposition that was matched.
  */
class PropPFeature: public PatternFeature {
private:
	PropPFeature(Pattern_ptr pattern, const Proposition* prop, int sent_no, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(PropPFeature, Pattern_ptr, const Proposition*, int, const LanguageVariant_ptr&);

	PropPFeature(const Proposition* prop, int sent_no, int start_token, int end_token, const LanguageVariant_ptr& languageVariant, float confidence);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(PropPFeature, const Proposition*, int, int, int, const LanguageVariant_ptr&, float);
	
	PropPFeature(Pattern_ptr pattern, const Proposition* prop, int sent_no);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(PropPFeature, Pattern_ptr, const Proposition*, int);

	PropPFeature(const Proposition* prop, int sent_no, int start_token, int end_token, float confidence);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(PropPFeature, const Proposition*, int, int, int, float);

public:

	/** Return the proposition that was matched to create this feature. */
	const Proposition *getProp() { return _proposition; }

	// Overridden virtual methods.
	int getSentenceNumber() const { return _sent_no; }
	int getStartToken() const { return _start_token; }
	int getEndToken() const { return _end_token; }
	bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<PropPFeature> f = boost::dynamic_pointer_cast<PropPFeature>(other);
		return f && f->getProp() == getProp();
	}
	virtual void setCoverage(const DocTheory * docTheory);
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher);
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	PropPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	const Proposition *_proposition;
	int _sent_no;
	int _start_token, _end_token;
};

typedef boost::shared_ptr<PropPFeature> PropPFeature_ptr;

#endif

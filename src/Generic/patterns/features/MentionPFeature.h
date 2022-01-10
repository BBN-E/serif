// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_PFEATURE_H
#define MENTION_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include <boost/shared_ptr.hpp>

class Mention;

/** A feature used to store information about a successful MentionPattern match.
  * In addition to the basic PatternFeature information, each MentionPFeature 
  * defines:
  *
  *   - mention: A pointer to the mention that was matched (not owned by this
  *     MentionPFeature).
  *
  *   - matchSym: A symbol containing the name of an ACE type that matched an
  *     "acetype" constraint; *or* the name of an entity label that matched an
  *     "entitylabel" constraint; *or* the name of an ACE subtype that matched
  *     an "acesubtype" constraint.  If none of these constraints was used, then
  *     the matchSym will be the empty symbol.
  *
  *   - is_focus: True if the MentionPattern was marked as a "FOCUS".  This is
  *     used by EntityLabelPatterns to mark the mention whose entity should
  *     recieve the label.
  */
class MentionPFeature: public PatternFeature {
private:
	/** Create a new MentionPFeature.  
	  * is_focus will be true if it is explicitly set to true, *or* if the given pattern
	  * is a FocusMentionPattern. */
	MentionPFeature(Pattern_ptr pattern, const Mention *mention, Symbol matchSym,
		const LanguageVariant_ptr& languageVariant, float confidence, bool is_focus = false);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(MentionPFeature, Pattern_ptr, const Mention*, Symbol, const LanguageVariant_ptr&, float);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(MentionPFeature, Pattern_ptr, const Mention*, Symbol, const LanguageVariant_ptr&, float, bool);
	
	MentionPFeature(Pattern_ptr pattern, const Mention *mention, Symbol matchSym,
		float confidence, bool is_focus = false);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(MentionPFeature, Pattern_ptr, const Mention*, Symbol, float);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(MentionPFeature, Pattern_ptr, const Mention*, Symbol, float, bool);
public:

	/** Return the mention that was matched to create this feature. */
	const Mention *getMention() { return _mention; }

	/** Return true if this feature's mention is a focus mention. */
	bool isFocusMention() { return _is_focus; }

	/** Return true if this feature was generated by a pattern match that
	  * required the given entity label.  (Note: this will return false if
	  * the pattern also required an acetype, and that matched instead.) */
	bool matchesLabel(Symbol label) { return _matchSym == label; }

	/** Return true if this feature was generated by a pattern match that
	  * required some entity label.    (Note: this will return false if
	  * the pattern also required an acetype, and that matched instead.) */
	bool matchesEntityLabel();

	// Overridden virtual methods.
	virtual void setCoverage(const DocTheory * docTheory) {/* nothing to do*/}
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) {/* nothing to do*/}
	virtual int getSentenceNumber() const { return _mention->getSentenceNumber(); }
	virtual int getStartToken() const { return _mention->getNode()->getStartToken(); }
	virtual int getEndToken() const { return _mention->getNode()->getEndToken(); }
	virtual bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<MentionPFeature> f = boost::dynamic_pointer_cast<MentionPFeature>(other);
		return f && f->getMention() == getMention();
	}
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	MentionPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	const Mention *_mention;
	Symbol _matchSym; // contains ace type name *or* entity label name *or* ace subtype name (or empty)
	bool _is_focus;	
};

typedef boost::shared_ptr<MentionPFeature> MentionPFeature_ptr;

#endif
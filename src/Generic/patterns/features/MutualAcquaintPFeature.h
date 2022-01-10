// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef MUTUAL_ACQUAINT_PFEATURE_H
#define MUTUAL_ACQUAINT_PFEATURE_H

#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"


/** A pseudo-feature used to record the fact that the person identified
  * by a given mention (the "aquaintanceMention") might be a mutual 
  * aquaintance of mentions with labels AGENT1 and AGENT2.  In particular,
  * this feature indicates that the aquaintanceMention knows the
  * agentMention, who is labeled with either AGENT1 or AGENT2.  In order
  * for this person to be a mutual aquaintance, there must also be a
  * MutualAcquaintPFeature connecting this acquaintanceMention to the other
  * agent.
  */
class MutualAcquaintPFeature : public PseudoPatternFeature {
private:
	MutualAcquaintPFeature(Symbol agentSym, const Mention *acquaintanceMention, const Mention *agentMention, const LanguageVariant_ptr& languageVariant);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(MutualAcquaintPFeature, Symbol, const Mention*, const Mention*, const LanguageVariant_ptr&);
	
	MutualAcquaintPFeature(Symbol agentSym, const Mention *acquaintanceMention, const Mention *agentMention);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(MutualAcquaintPFeature, Symbol, const Mention*, const Mention*);

public:	
	/** Return the potential mutual aquaintance identified by this feature. */
	const Mention* getAcquaintanceMention() { return _acquaintanceMention; }

	/** Return the agent who is an aquaintance of the potential mutual 
	  * aquaintance. */
	const Mention* getAgentMention() { return _agentMention; }

	/** Return the entity label (AGENT1 or AGENT2) of the agent who is an
	  * aquaintance of the potential mutual aquaintance. */
	Symbol getAgentSym() { return _agentSym; }

	virtual void setCoverage(const DocTheory * docTheory) { /* nothing to do */ }
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) { /* nothing to do */ }
	virtual int getSentenceNumber() const { return _acquaintanceMention->getSentenceNumber(); }
	virtual int getStartToken() const { return _acquaintanceMention->getNode()->getStartToken(); }
	virtual int getEndToken() const { return _acquaintanceMention->getNode()->getEndToken(); }
	bool equals(PatternFeature_ptr other);
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	MutualAcquaintPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	const Mention *_acquaintanceMention;
	const Mention *_agentMention;
	Symbol _agentSym;
};

typedef boost::shared_ptr<MutualAcquaintPFeature> MutualAcquaintPFeature_ptr;

#endif

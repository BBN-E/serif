// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_TYPES_H
#define PATTERN_TYPES_H

#include <vector>
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
class Mention;
class Argument;
class SynNode;
class SentenceTheory;
class EventMention;
class RelMention;
class Proposition;
class ValueMention;
class SpeakerQuotation;
class PropStatusManager;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;
typedef boost::shared_ptr<PropStatusManager> PropStatusManager_ptr;

/*********************************************************************
 * Top-Level Matching Interfaces (Sentence & Document)
 *********************************************************************
 * Each Pattern that can act as a top-level pattern must be either a
 * SentenceMatchingPattern or a DocumentMatchingPattern (but not 
 * both).  A Pattern should be a DocumentMatchingPattern if it needs
 * to match a unit of text that is larger than a single sentence (e.g.
 * QuotationPattern matches quotations, which may span across multiple
 * sentences).
 */

/** An abstract base class for patterns that can be used to match sentences. */
class SentenceMatchingPattern {
public: // should these be "const SentenceTheory*" instead?
	virtual std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0) = 0;
	virtual PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0) = 0;
	
	PatternFeatureSet_ptr matchesAlignedSentence (PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, 
		LanguageVariant_ptr restriction, UTF8OutputStream *debug = 0)
	{
		PatternFeatureSet_ptr result = boost::make_shared<PatternFeatureSet>();

		BOOST_FOREACH(LanguageVariant_ptr lv, patternMatcher->getAlignedLanguageVariants(restriction)) {
			SentenceTheory* aligned_sent = patternMatcher->getDocumentSet()->getAlignedSentenceTheory(
				patternMatcher->getActiveLanguageVariant(), lv, sTheory); 
			
			if (aligned_sent) {
				PatternMatcher::LanguageSwitcher languageSwitcher(patternMatcher, lv);
				PatternFeatureSet_ptr features = matchesSentence(patternMatcher, aligned_sent, debug);
				if (features) { result->addFeatures(features); }
			}
		}

		return result->getNFeatures() > 0 ? result : PatternFeatureSet_ptr();
	} 

	std::vector<PatternFeatureSet_ptr> multiMatchesAlignedSentence (PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, 
		LanguageVariant_ptr restriction, UTF8OutputStream *debug = 0)
	{
		std::vector<PatternFeatureSet_ptr> result;

		BOOST_FOREACH(LanguageVariant_ptr lv, patternMatcher->getAlignedLanguageVariants(restriction)) {
			SentenceTheory* aligned_sent = patternMatcher->getDocumentSet()->getAlignedSentenceTheory(
				patternMatcher->getActiveLanguageVariant(), lv, sTheory); 

			if (aligned_sent) {
				PatternMatcher::LanguageSwitcher languageSwitcher(patternMatcher, lv);
				std::vector<PatternFeatureSet_ptr> feats = multiMatchesSentence(patternMatcher, aligned_sent, debug);
				for (std::size_t i=0;i<feats.size();i++)  {
					result.push_back(feats[i]);
				}
			}
		}

		return result;
	}
};

/** An abstract base class for patterns that can be used to match documents. 
  * This should be used for */
class DocumentMatchingPattern {
public:
	virtual std::vector<PatternFeatureSet_ptr> multiMatchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug = 0) = 0;
	virtual PatternFeatureSet_ptr matchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug = 0) = 0;
};

/*********************************************************************
 * Theory-Level Matching Interfaces (Mention, EventMention, etc)
 *********************************************************************/

/** An abstract base class for patterns that can be used to match event mentions. */
class EventMentionMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesEventMention(PatternMatcher_ptr patternMatcher, int sent_no, const EventMention *em) = 0;
};
class RelMentionMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesRelMention(PatternMatcher_ptr patternMatcher, int sent_no, const RelMention *rm) = 0;
};
class PropMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesProp(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition* prop, bool fall_through_children, PropStatusManager_ptr statusOverrides) = 0;
	virtual bool allowFallThroughToChildren() const = 0;
};
class ArgumentMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesArgument(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children) = 0;
	virtual bool allowFallThroughToChildren() const = 0;
};
class ArgumentValueMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) = 0;
	virtual bool allowFallThroughToChildren() const = 0;
};
class MentionAndRoleMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const Mention *mention, bool fall_through_children) = 0;
};
class MentionMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children) = 0;
	virtual bool allowFallThroughToChildren() const = 0;

	PatternFeatureSet_ptr matchesAlignedMention(PatternMatcher_ptr patternMatcher, const Mention *mention, 
		bool fall_through_children, LanguageVariant_ptr restriction)
	{
		PatternFeatureSet_ptr result = boost::make_shared<PatternFeatureSet>();
		
		BOOST_FOREACH(LanguageVariant_ptr lv, patternMatcher->getAlignedLanguageVariants(restriction)) {
			Mention* aligned_mention = patternMatcher->getDocumentSet()->getAlignedMention(
				patternMatcher->getActiveLanguageVariant(), lv, mention); 
			
			if (aligned_mention) {
				PatternMatcher::LanguageSwitcher languageSwitcher(patternMatcher, lv);
				PatternFeatureSet_ptr features = matchesMention(patternMatcher, aligned_mention, fall_through_children);
				if (features) { result->addFeatures(features); }
			}
		}

		return result->getNFeatures() > 0 ? result : PatternFeatureSet_ptr();
	} 
};
class ValueMentionAndRoleMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesValueMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const ValueMention *valueMention) = 0;
};
class ValueMentionMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesValueMention(PatternMatcher_ptr patternMatcher, const ValueMention *valueMent) = 0;
};
class SpeakerQuotationMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesSpeakerQuotation(PatternMatcher_ptr patternMatcher, const SpeakerQuotation *quotation) = 0;
};
class ParseNodeMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node) = 0;
};

typedef boost::shared_ptr<SentenceMatchingPattern> SentenceMatchingPattern_ptr;
typedef boost::shared_ptr<DocumentMatchingPattern> DocumentMatchingPattern_ptr;
typedef boost::shared_ptr<EventMentionMatchingPattern> EventMentionMatchingPattern_ptr;
typedef boost::shared_ptr<RelMentionMatchingPattern> RelMentionMatchingPattern_ptr;
typedef boost::shared_ptr<PropMatchingPattern> PropMatchingPattern_ptr;
typedef boost::shared_ptr<ArgumentMatchingPattern> ArgumentMatchingPattern_ptr;
typedef boost::shared_ptr<ArgumentValueMatchingPattern> ArgumentValueMatchingPattern_ptr;
typedef boost::shared_ptr<MentionAndRoleMatchingPattern> MentionAndRoleMatchingPattern_ptr;
typedef boost::shared_ptr<MentionMatchingPattern> MentionMatchingPattern_ptr;
typedef boost::shared_ptr<ValueMentionAndRoleMatchingPattern> ValueMentionAndRoleMatchingPattern_ptr;
typedef boost::shared_ptr<ValueMentionMatchingPattern> ValueMentionMatchingPattern_ptr;
typedef boost::shared_ptr<SpeakerQuotationMatchingPattern> SpeakerQuotationMatchingPattern_ptr;
typedef boost::shared_ptr<ParseNodeMatchingPattern> ParseNodeMatchingPattern_ptr;


#endif

// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_FEATURE_H
#define PATTERN_FEATURE_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/state/XMLElement.h"
#include "Generic/state/XMLIdMap.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"

class Pattern;
class PatternMatcher;
class UTF8OutputStream;
class Mention;
class DocTheory;
class PatternFeatureSet;
typedef boost::shared_ptr<Pattern> Pattern_ptr;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;
typedef boost::shared_ptr<PatternFeatureSet> PatternFeatureSet_ptr;

class PatternFeature;
typedef boost::shared_ptr<PatternFeature> PatternFeature_ptr;

/** Abstract base class for "pattern features," which are data structures created by
  * patterns when they match that given information about how they matched.  Pattern
  * features should always be stored using a boost::shared_ptr.  They are not copyable.
  * Typically they are not mutable.
  *
  * Each pattern feature has a location associated with it, consisting of a sentence
  * number, a start token index, and an end token index.  This typically indicates
  * the region of text that was matched by each feature's pattern.  If a feature does
  * not correspond to any specific part of the text, then its sentence number and
  * token numbers will be -1.  If a feature corresponds to a sentence, but not to a
  * specific part of that sentence, then its token numbers will be -1.
  *
  * If you wish to test whether a PatternFeature has a specific type, use 
  * boost::dynamic_pointer_cast.  E.g.:
  *
  *     if (MentionFeature_ptr mf=boost::dynamic_pointer_cast<MentionFeature>(feat)) {
  *         ...use mf...;
  *     }
  *
  * Naming convention: classes that end in "PatternFeature", such as "PseudoPatternFeature"
  * and "ReturnPatternFeature", are abstract base classes.  Classes that end in "PFeature"
  * are concrete subclasses of PatternFeature.
  */
class PatternFeature: private boost::noncopyable, public boost::enable_shared_from_this<PatternFeature> {
public:	
	virtual ~PatternFeature() {};

	/** Return a pointer to the pattern that created this feature.  If this feature
	  * was not created by a pattern, return an empty pointer. */
	Pattern_ptr getPattern() const { return _pattern; }

	/** Return this feature's confidence score. */
	float getConfidence() const { return _confidence; }

	/** Return the language-variant from which this feature was generated */
	const LanguageVariant_ptr getLanguageVariant() const { return _languageVariant; }

	/** This currently needs to be called *before* you call getSentenceNumber(),
	  * getStartToken(), or getEndToken().
	  */
	virtual void setCoverage(const DocTheory * docTheory) = 0;
	/** This method will always end up calling setCoverage(const DocTheory *) or doing nothing, but due to
	 *  method hiding, you always have to define it to do that.
	 *  Example: DerivedPatternFeature::setCoverage(const PatternMatcher_ptr patternMatcher) {setCoverage(patternMatcher->getDocTheory());}
	 */
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) = 0;

	/** Return the sentence number for the sentence containing the region of text
	  * that was matched by this feature's pattern.  If no specific sentence was
	  * matched, return -1. */
	virtual int getSentenceNumber() const = 0;

	/** Return the start token index for the region of text that was matched 
	  * by this feature's pattern.  This is an inclusive index in the token 
	  * sequence of the sentence indicated by getSentenceNumber().  If no 
	  * specific token sequence was matched, return -1. */
	virtual int getStartToken() const = 0;

	/** Return the end token index for the region of text that was matched 
	  * by this feature's pattern.  This is an inclusive index in the token 
	  * sequence of the sentence indicated by getSentenceNumber().  If no 
	  * specific token sequence was matched, return -1. */
	virtual int getEndToken() const = 0;

	/** Return whether two Pattern Features are identical */
	virtual bool equals(PatternFeature_ptr other) = 0;

	/** Typically (but not always) called by a subclass as part of its equality function */
	bool simpleEquals(PatternFeature_ptr other);

	/** Send an XML-encoded representation of this feature's information to
	  * the given stream.  This output format consists of zero or more
	  * elements with the form:
	  *
	  *     <focus type="..." valN="..." valN="..." .../>
	  *
	  * Where N is an integer.
	  */
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const = 0;

	/** Serialize Feature */
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;

	/** Deserialize Feature */
	PatternFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

	/** Cast this feature to the given type.  If it does not actually have the
	  * appropriate type, then throw an InternalInconsistencyException.  If you
	  * are not sure whether the feature should have the given type, then you
	  * should use boost::dynamic_pointer_cast<> instead, and check if the result
	  * is zero. */
	template<class FeatureType>
	boost::shared_ptr<FeatureType> castTo() {
		boost::shared_ptr<FeatureType> ptr = boost::dynamic_pointer_cast<FeatureType>(shared_from_this());
		if (!ptr)
			throw InternalInconsistencyException("PatternFeature::castTo", "Feature does not have the expected type!");
		return ptr;
	}

protected:
	/** Constructor -- called by subclasses. */
	PatternFeature(Pattern_ptr pattern, const LanguageVariant_ptr& languageVariant, float confidence=1) : 
		 _pattern(pattern), _confidence(confidence), _languageVariant(languageVariant) {}
		 
	PatternFeature(Pattern_ptr pattern, float confidence=1) : 
		 _pattern(pattern), _confidence(confidence), _languageVariant(LanguageVariant::getLanguageVariant()) {}

	/*********************************************************************
	 * Helper Methods for Subclasses (mostly for printFeatureFocus)
	 *********************************************************************/

	/** Escape any characters in the given string that have special meaning in
	  * XML, and return the resulting escaped string. Moved to OutputUtil. */
	static std::wstring getXMLPrintableString(const std::wstring & str) {
		return OutputUtil::escapeXML(str);
	}

	/** Send XML attributes containing the start and end characters for the given
	  * range of tokens to the specified output stream.  This is a helper method
	  * for subclasses' printFeatureFocus() methods. */
	void printOffsetsForSpan(const PatternMatcher_ptr patternMatcher, int sentenceNumber, int startTokenIndex, int endTokenIndex, 
		UTF8OutputStream &out) const;

	/** Return an escaped XML string containing the best name for the given
	  * mention.  If the mention's entity type is not recognized, then this
	  * will be "NON_ACE".  Otherwise, it will be the value returned by 
	  * Entity::getBestName() for the entity corresponding to the mention. */
	std::wstring getBestNameForMention(const Mention *mention, const DocTheory *docTheory) const;

public:
	/*********************************************************************
	 * Constants used by printFeatureFocus (and also DRFunctionManager, etc)
	 *********************************************************************/
	static const int val_sent_no = 0;
	static const int val_id = 1;
	static const int val_score = 2;
	static const int val_confidence = 3;
	static const int val_start_token = 4;
	static const int val_end_token = 5;
	static const int val_extra = 6;
	static const int val_extra_2 = 7;
	static const int val_extra_3 = 8;
	static const int val_extra_4 = 9;
	static const int val_extra_5 = 10;
	static const int val_extra_6 = 11;
	static const int val_extra_7 = 12;
	static const int val_extra_8 = 13;
	static const int val_extra_9 = 14;
	static const int val_extra_10 = 15;

	static const int val_arg_parent_id = 1;
	static const int val_arg_role = 2;
	static const int val_arg_id = 3;
	static const int val_arg_canonical_ref = 4;
	static const int val_arg_parent_type = 5;
	static const int val_arg_entity_type = 6;

	static const int val_topic_sent_no = 1;
	static const int val_topic_strategy = 2;
	static const int val_topic_score = 3;
	static const int val_topic_query_slot = 4;
	static const int val_topic_context = 5;
	static const int val_topic_backward_range = 6;
	static const int val_topic_forward_range = 7;
	
	static const int val_top_level_pattern = 0;
	static const int val_backoff_level = 0;

private:
	/*********************************************************************
	 * Private Data Members
	 *********************************************************************/
	const Pattern_ptr _pattern;
	float _confidence;
	const LanguageVariant_ptr _languageVariant;
};

#endif

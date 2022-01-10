#pragma once

#include <set>
#include "LearnIt/SlotConstraints.h"
#include "LearnIt/Target.h"
#include "LearnIt/BestName.h"
#include "LearnIt/MentionToEntityMap.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "boost/noncopyable.hpp"
#include "boost/shared_ptr.hpp"
#include "Generic/common/bsp_declare.h"
#include "SlotFillerTypes.h"

class TokenSequence;  // defined in "Generic/theories/TokenSequence.h"
class SentenceTheory; // defined in "Generic/theories/SentenceTheory.h"
class ValueMention;   // defined in "Generic/theories/ValueMention.h"
class Feature; // defined in "distill-generic/features/SnippetFeature.h"
class Argument;
class DocTheory;

BSP_DECLARE(Seed);
BSP_DECLARE(ObjectWithSlots)
BSP_DECLARE(PatternFeature)
BSP_DECLARE(AbstractLearnItSlot)

struct entityCompare {
  bool operator() (const Entity* lhs, const Entity* rhs) const
  {return lhs->getID() < rhs->getID();}
};

class SlotBase {
public:
	SlotBase(SlotConstraints_ptr constraints);
	SlotBase(SlotConstraints_ptr constraints, SlotConstraints::SlotType t);
	SlotBase(const SlotBase& oth);
	virtual const std::wstring& name(bool normalize = true) const = 0;
	SlotConstraints::SlotType getType() const;
protected:
	const std::vector<std::wstring>& asYYMMDD() const;
	double YYMMDDMatchAgainst(const SlotBase & other) const;

private:
	void _initYYMMDD() const;
	mutable std::vector<std::wstring> _yymmdd;
	mutable bool _yymmdd_initialized;
	const bool _is_yymmdd;
	// Must match non-null pointer, or be a special type
	SlotConstraints::SlotType _slot_type;
};

class AbstractLearnItSlot : public SlotBase {
public:
	AbstractLearnItSlot(SlotConstraints_ptr constraints, const std::wstring& name);
	virtual const std::wstring& name(bool normalize = true) const;
private:
	const std::wstring _name;
	mutable std::wstring _normalized_name;
};

/**
 * A wrapper for a mention or value_mention that fills a relation slot.  This 
 * class keeps a pointer to either a mention or a value-mention.  It does *not* 
 * own that mention/value-mention, and is not responsible for deleting it.  The 
 * user of this class is responsible for ensuring that the mention/value-mention
 * pointed to by a SlotFiller is not deleted before the SlotFiller itself is
 * deleted.
 *
 * Implementation note: at some point, it might make sense to replace this
 * with an abstract base class with two subclasses, one for mentions and
 * one for value-mentions.  The SnippetFeature constructor would need to be
 * replaced w/ a factory method.
 */
class SlotFiller: public SlotBase, private boost::noncopyable {
public:
	/** Construct a slot filler that points at a given mention. */
	SlotFiller(const Mention *mention, const DocTheory* doc,
		SlotConstraints_ptr slot_constraints, const LanguageVariant_ptr& languageVariant);

	/** Construct a slot filler that points at a given ValueMention */
	SlotFiller(const ValueMention *value_mention, const DocTheory* doc, 
		SlotConstraints_ptr slot_constraints, const LanguageVariant_ptr& languageVariant);

	/** Construct a slot filler that points at whatever Mention or ValueMention 
	  * is contained in the given PatternFeature.  The PatternFeature's feature
	  * type should be MENTION, VALUE_MENTION, or RETURN.  If it is a RETURN 
	  * feature, then its return type must be MENTION or VALUE_MENTION. */
	//SlotFiller(PatternFeature_ptr slot_feature, const DocTheory* doc, 
	//		SlotConstraints_ptr slot_constraints);

	/** Constructs a slot filler containing a special placeholder slot type
	  * to indicate a particular issue with the slot. */
	SlotFiller(SlotConstraints::SlotType slot_type);
	
	/** Constructs a slot filler which is a copy of another SlotFiller with 
		a score multiplied by some penalty */
	SlotFiller(const SlotFiller& sf, double penalty);

	/** Factory function to construct a slot filler that points at whatever 
	  * Mention or ValueMention is contained in the given SnippetFeature.  
	  * The SnippetFeature's feature type should be MENTION, VALUE_MENTION, 
	  * or RETURN.  If it is a RETURN feature, then its return type must be 
	  * MENTION or VALUE_MENTION. */
	static SlotFiller_ptr fromPatternFeature(PatternFeature_ptr slot_feature, 
		const DocTheory* dt, SlotConstraints_ptr slot_constraints);
	  
	static SlotFiller_ptr fromPatternFeature(PatternFeature_ptr slot_feature, 
		const DocTheory* dt, SlotConstraints_ptr slot_constraints, const LanguageVariant_ptr& languageVariant);

	/** Factory function: return a map from Target_ptr to vector of potential slot fillers
	  *(doesn't filter any)*/
	static TargetToFillers getAllSlotFillers(const AlignedDocSet_ptr doc_set, Symbol docid,
						int sentno,
						const std::vector<Target_ptr>& targets);

	static void getSlotFillers(const AlignedDocSet_ptr doc_set,
			SentenceTheory* sent_theory, const Target_ptr target,
			SlotFillersVector & slotFillers, const LanguageVariant_ptr& languageVariant);

	/** Factory function: return a vector of SlotFiller objects that are potential 
	  * "distractors" for a given slot.  A good distractor is a slot filler that 
	  * (1) satisfies all of the slot constraints for the desired slot; and (2) does
	  * not actually match any of the given seed's slot strings. */
	static std::vector<SlotFiller_ptr> getSlotDistractors(const DocTheory* doc_info, 
		SentenceTheory *sent_theory, Target_ptr target, Seed_ptr seed, int slot_num);

	/** Return the serialization identifier for this slot filler from 
	  * the ObjectIDTable. */
	//int getSerializedObjectID() const;

	/** Return the serialization identifier for this slot filler's entity (if
	  * it has one) from the ObjectIDTable. */
	//int getSerializedObjectIDForEntity() const;

	/** Return the Mention::Type of a mention; or Mention::NONE for value mentions. */
	Mention::Type getMentionType() const;

	/** Return the Entity associated with a mention, if any (NULL for value mentions). */
	/** Warning: Non-value mentions can also return 0.  Some mentions do not have entities.*/
	const Entity* getMentionEntity() const;

	/** Return the index of the first token in the slot filler. */
	int getStartToken() const;

	/** Return the index of the last token in the slot filler. */
	int getEndToken() const;

	/** Return the index of the first token in the slot filler's head.
		For values, this is the same as getStartToken() */
	int getHeadStartToken(const Target_ptr target, const int slot_num) const;

	/** Return the index of the last token in the slot filler's head.
		For values, this is the same as getEndToken(). */
	int getHeadEndToken(const Target_ptr target, const int slot_num) const;

	/** Return the character offset of the first character in the slot filler. */
	EDTOffset getStartOffset() const;

	/** Return the character offset of the last character in the slot filler. */
	EDTOffset getEndOffset() const;

	/** Return the literal text of this slot filler. */
	std::wstring getText() const;

	/** Return the original text of this slot filler from the
	  * document, based on offsets */
	std::wstring getOriginalText() const;

	/** Return the best string we can find for this slot filler's entity.  This
	  * is the string we use to represent this slot filler in seeds.  (N.b.: 
	  * the returned string is not necessarily a name.) */
	const std::wstring& getBestName(bool normalize=true) const;

	// just alias for getBestName, needed for base class. 
	// eventually we can change all uses of getBestName to just name()
	virtual const std::wstring& name(bool normalize = true) const;

	/** Return a score from 0-1, indicating how confident we are that the string 
	  * returned by getBestName() is a good string for this slot filler. */
	double getBestNameConfidence() const{
		return _bestName.confidence();
	}

	/** Set the score from 0-1, indicating how confident we are that the string 
	  * returned by getBestName() is a good string for this slot filler. */
	void setBestNameConfidence(double new_score){
		_bestName.setConfidence(new_score);
	}

	/** Return the Mention::Type of the mention that the best name came from. */
	Mention::Type getBestNameMentionType() const { return _bestName.mentionType(); }

	/** Get the offsets of the mention that the best name came from. */
	void getBestNameMentionOffsets(EDTOffset &start, EDTOffset &end) const;

	/** Return true if this slot filler is a wrapper for the given mention. **/
	bool contains(Mention const *mention) const {
		return (mention == _mention);
	}
	/** Return true if this slot filler is a wrapper for the given value mention. */
	bool contains(ValueMention const *value_mention) const {
		return (value_mention == _value_mention); 
	}
	/** Return true if this slot filler wraps a mention or value that matches this argument */
	bool matchesArgument(Argument const *argument) const; 

	/** Returns whether this slot filler is a good match for a given slot of a 
		seed. This just calls slotMatchScore and compares it against a
		threshold **/
	static bool isGoodSlotMatch(SlotFiller_ptr filler, const SlotBase& slot, 
		SlotConstraints_ptr slotConstraints);
	
	/** Takes a list of slot fillers and keeps only those which are good matches
		with a given seed string **/
	static void filterBySeed(
		const std::vector<SlotFiller_ptr>& fillerList, SlotFillers& output,
		const SlotBase& slot, SlotConstraints_ptr slotConstraints,  bool update_score = true,
		const std::set<Symbol>& overlapIgnoreWords = std::set<Symbol>());

	/** Given possibilities for each slot, returns all possible combinations
		of slot fillers **/
	static bool findAllCombinations(Target_ptr target,
		const SlotFillersVector& slotFillerPossibilities,
		std::vector<SlotFillerMap>& output);

	/** Return the mention that this slot filler contains, or NULL if it contains a ValueMention.
	  * Only use this method when necessary -- it is usually better to treat Mentions and ValueMentions
	  * uniformly. */
	Mention const *getMention() const { return _mention; }

	/** Return the value mention that this slot filler contains, or NULL if it contains a Mention.
	  * Only use this method when necessary -- it is usually better to treat Mentions and ValueMentions
	  * uniformly. */
	ValueMention const *getValueMention() const { return _value_mention; }

	/** Return true if this slot filler satisfies the given slot constraints.
	  * E.g., if the slot constrains include YYMMDD, and our best name is
	  * "Tuesday", then return False. */
	bool satisfiesSlotConstraints(SlotConstraints_ptr slot_constraints) const;


	/** Return true under the following conditions, for two slots from the same document and sentence:
	*   (a) slotA and slotB have the same token offsets 
	*   (b) SlotA and slotB are both mention-slots and have the same 'atomic head'
	*/
	bool isEquvialentSlot(const SlotFiller_ptr other_slot) const;

	typedef std::map<const Entity*, std::set<std::wstring> , entityCompare> 
		EntityToStringSetMapping;
	typedef EntityToStringSetMapping::const_iterator EntityStringsIterator;
	// these return pointers to a cache which will be invalidated whenever a
	// new document is loaded!
	EntityStringsIterator getEntityStrings(const DocTheory* docTheory) const;
	static EntityStringsIterator noEntityStrings();

	// when comparing a seed slot to a mention, this is the function used
	// to get the string representation of coreferent mentions
	static std::wstring corefName(const DocTheory* dt, const Mention* coref_ment);

	static double GOOD_MATCH;

	bool filled() const;
	bool unfilledOptional() const;
	bool unmetConstraints() const;
	bool sameReferent(const SlotFiller& b) const;
	bool operator==(const SlotFiller& rhs) const;
	bool operator<(const SlotFiller& rhs) const;

	static bool useCoref();

	const LanguageVariant_ptr getLanguageVariant() const { return _language_variant; }
private:

	// Exactly one of these should be null at all times:
	Mention const *_mention;
	ValueMention const *_value_mention;
	const DocTheory* _doc;
	const LanguageVariant_ptr _language_variant;

	SentenceTheory *_sent_theory;

	BestName _bestName;
	mutable std::wstring _normalizedBestName;
	
	void _init(SlotConstraints_ptr slot_constraints);
	void _initializeYYMMDDParts(SlotConstraints_ptr slot_constraints);
	std::vector<std::wstring> _YYMMDDParts;

	/** Determine whether this slot filler "matches" a given slot of a seed.  This is used by
	  * SeedMatchFinder to find sentences that contain the desired seeds. */
	double slotMatchScore(const SlotBase& slot, SlotConstraints_ptr slotConstraints,
                 const std::set<Symbol>& overlapIgnoreWords = std::set<Symbol>()) const;

	/** Returns True if the following conditions are met:
	  *    (a) this slot filler contains a mention
	  *    (b) that mention is a member of an entity
	  *    (c) at least one name mention in the entity has a head 
	  *        (i.e. the name string) that matches the given seed string
	  * TODO: Use of this method should be replaced with the EntityLabeling patterns from Brandy.  
	  *       This would give fuzzy matching for punctuation, equivalent names, etc.
	  */
	bool matchSeedStringToEntity(const std::wstring& seed_string) const;
	bool matchSeedStringToEntity(const std::wstring& seed_string, const Entity* entity) const;

	//Cache for matchSeedStringToEntity, needs to be cleared anytime a new document is loaded.
	static EntityToStringSetMapping entityToStringSet;
	//marks if stringToEntitySet cache is dirty
	static Symbol _curr_doc;

	static void fillCache( const AlignedDocSet_ptr& doc_set );

	/** Initialize bestName and _best_name_confidence. */
	void _findBestName(SlotConstraints_ptr slot_constraints);

	static void initSlotFillerStatics();
	static bool _initialized;
	static bool _useCoref;
};

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_ITEM_IDENTIFIER_H
#define SENTENCE_ITEM_IDENTIFIER_H

#include <boost/functional/hash.hpp>
#include "Generic/common/limits.h"

/** A document-level identifier that consists of two parts: a sentence
  * number and an index into a list within that sentence.  This is used
  * to define both MentionUID and ValueMentionUID.  The motivation
  * behind using an identifier (rather than directly pointing at
  * the object) is that we may want to swap in a new annotation layer
  * (e.g., to modify the mentions' entity types), and this lets us do so
  * without breaking any pointers. */
template<typename Tag>
class SentenceItemIdentifier {
private:
	int _sentno; // Index of sentence containing this mention.
	int _index;  // Index of the mention within this sentence.
public:
	SentenceItemIdentifier(): _sentno(-1), _index(-1) {}
	SentenceItemIdentifier(int sentno, int index): _sentno(sentno), _index(index) {}
	int sentno() const { return _sentno; }
	int index() const { return _index; }
	bool isValid() const { return _sentno != -1; }
	// Legacy functions (for serialization/deserialization):
	int toInt() const { return static_cast<int>(_sentno*Tag::max_items_per_sentence()) + _index; }
	explicit SentenceItemIdentifier(int n): _sentno(static_cast<int>(n/Tag::max_items_per_sentence())), _index(static_cast<int>(n%Tag::max_items_per_sentence())) {
		if (n == -1) { _sentno = -1; } // Backwards compatibility
	}
	// Comparison functions.
	bool operator==(const SentenceItemIdentifier<Tag> &other) const { return _sentno==other._sentno && _index==other._index; }
	bool operator!=(const SentenceItemIdentifier<Tag> &other) const { return !(*this==other); }
	bool operator< (const SentenceItemIdentifier<Tag> &other) const { return toInt() <  other.toInt(); }
	bool operator<=(const SentenceItemIdentifier<Tag> &other) const { return toInt() <= other.toInt(); }
	bool operator> (const SentenceItemIdentifier<Tag> &other) const { return toInt() >  other.toInt(); }
	bool operator>=(const SentenceItemIdentifier<Tag> &other) const { return toInt() >= other.toInt(); }
	// Operators (for sets & hash-maps)
	struct EqualOp { size_t operator()(const SentenceItemIdentifier<Tag> &a, const SentenceItemIdentifier<Tag> &b) const { return a==b; } };
	struct LessThanOp { size_t operator()(const SentenceItemIdentifier<Tag> &a, const SentenceItemIdentifier<Tag> &b) const { return a<b; } };
	struct HashOp { 
		size_t operator()(const SentenceItemIdentifier<Tag> &u) const { 
			size_t hash_value = 0;
			boost::hash_combine(hash_value, u.sentno());
			boost::hash_combine(hash_value, u.index());
			return hash_value;
		}
	};
};

struct MentionUID_tag { 
	static int REAL_MAX_SENTENCE_MENTIONS; // Warning: changing this can invalidate UIDs!
	static size_t max_items_per_sentence() { 
		return REAL_MAX_SENTENCE_MENTIONS; 
	}};
typedef SentenceItemIdentifier<MentionUID_tag> MentionUID;

struct ValueMentionUID_tag { 
	static size_t max_items_per_sentence() { 
		return MAX_SENTENCE_VALUES; }};
typedef SentenceItemIdentifier<ValueMentionUID_tag> ValueMentionUID;

struct RelMentionUID_tag {
	static size_t max_items_per_sentence() {
		return MAX_SENTENCE_RELATIONS; }};
typedef SentenceItemIdentifier<RelMentionUID_tag> RelMentionUID;

struct EventMentionUID_tag {
	static size_t max_items_per_sentence() {
		return MAX_SENTENCE_EVENTS; }};
typedef SentenceItemIdentifier<EventMentionUID_tag> EventMentionUID;

struct PropositionUID_tag {
	static size_t max_items_per_sentence() {
		return MAX_SENTENCE_PROPS; }};
typedef SentenceItemIdentifier<PropositionUID_tag> PropositionUID;

#endif

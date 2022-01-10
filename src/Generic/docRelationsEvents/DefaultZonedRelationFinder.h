// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_ZONED_RELATION_FINDER_H
#define DEFAULT_ZONED_RELATION_FINDER_H

#include <boost/unordered_set.hpp>

#include "Generic/common/Symbol.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"

#include "Generic/docRelationsEvents/ZonedRelationFinder.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED	
#endif

// Represents one Mention matcher by region tag, entity type and subtype, and mention type.
typedef struct {
	Symbol regionTag;
	EntityType entityType;
	Mention::Type mentionType;
} ZonedMentionMatcher;
typedef boost::shared_ptr<ZonedMentionMatcher> ZonedMentionMatcher_ptr;

// Define less than so we can use unordered_set
inline bool operator==(const ZonedMentionMatcher_ptr& lhs, const ZonedMentionMatcher_ptr& rhs) {
	return lhs->regionTag == rhs->regionTag && lhs->entityType.getName() == rhs->entityType.getName() && lhs->mentionType == rhs->mentionType;
}

typedef struct {
	Symbol relationType;
	ZonedMentionMatcher_ptr leftMention;
	ZonedMentionMatcher_ptr rightMention;
} ZonedRelationMatcher;
typedef boost::shared_ptr<ZonedRelationMatcher> ZonedRelationMatcher_ptr;

// Define less than so we can use unordered_set
inline bool operator==(const ZonedRelationMatcher_ptr& lhs, const ZonedRelationMatcher_ptr& rhs) {
	return lhs->relationType == rhs->relationType && lhs->leftMention == rhs->leftMention && lhs->rightMention == rhs->rightMention;
}

class SERIF_EXPORTED DefaultZonedRelationFinder : public ZonedRelationFinder {
public:
	// Override the constructor to read the region tag table
	DefaultZonedRelationFinder();

	// Entry point: finds relations in a document's regions
	virtual RelMentionSet* findRelations(DocTheory* docTheory);

protected:
	// Helper method that creates all possible relations between two sets of mentions
	void createRelations(RelMentionSet* relations, Symbol relationType, int nSentences, std::vector<Mention*> & leftMentions, std::vector<Mention*> & rightMentions);

private:
	// Stores rules used for matching mentions; separated out so if mention restrictions
	// are reused we're not searching for the same mention match multiple times.
	boost::unordered_set<ZonedMentionMatcher_ptr> _mentionMatchers;

	// Stores rules used for connecting matched mentions into relations
	boost::unordered_set<ZonedRelationMatcher_ptr> _relationMatchers;

	// Stores threshold for limiting large gaps between maching Mentions
	int _maxMentionSpacing;
};

#endif

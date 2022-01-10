// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_RULE_DESC_LINKER_H
#define es_RULE_DESC_LINKER_H

#include "Generic/edt/RuleDescLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/DebugStream.h"

/**
 * Rule-based methods for associating descriptor
 * mentions with entities
 */
class SpanishRuleDescLinker : public RuleDescLinker {
private:
	friend class SpanishRuleDescLinkerFactory;

public:
	/**
	 * using a complicated set of rules, either create a new entity from currMention or associate it with
	 * an entity in currSolution.
	 *
	 * @param currSolution The set of entities up to this point
	 * @param currMention The mention to be linked (or not linked)
	 * @param results The possibly many but realistically one set of entities after this linking
	 * @param max_results The maximum size of results
	 * @return the size of results
	 */
	int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results);
private:
	/**
	 * Dummy constructor that doesn't do anything
	 */
	SpanishRuleDescLinker();
	// match ment to an entity in ents by looking at head NPs of mentions
	EntityGuess* _headNPMatch(Mention* ment, LexEntitySet* ents, EntityType linkType);

	// detect and return numbers in the premods
	const SynNode* _getNumericPremod(const SynNode* node) const ;

	// detect name mentions in the premods
	bool _hasPremodName(Mention* ment, EntitySet* ents);

	// detect and return premod name mentions
	int _getPremodNames(Mention* ment, EntitySet* ents,
						const SynNode* results[], const int max_results);

	// detect and return name mentions nested within mention nodes, including node itself 
	int _getMentionInternalNames(const SynNode* node, const MentionSet* ms,
								 const SynNode* results[], int max_results);

	// true if the premods of ment1 and any ent mention have mentions of name entities that are not the same
	// entities. (That. btw, is why the EntitySet is needed - to identify mentions from synnodes, and
	// to then identify entity ids)
	bool _hasPremodNameEntityClash(Mention* ment, Entity* ent, EntitySet* ents);
	// create a "new entity" guess from this mention
	EntityGuess* _guessNewEntity(Mention* ment, EntityType linkType);

	// true if the entity has a mention with numeric premods that clash with the node
	// Change (Mike): now we need the LexEntitySet to resolve mention IDs
	bool _hasNumericClash(const SynNode* node, Entity* ent, LexEntitySet *ents);

	int _getHeadNPWords(const SynNode* node, Symbol results[], int max_results);

	bool _hasNonDPPremod(const SynNode* node);

	DebugStream _debug;

};

class SpanishRuleDescLinkerFactory: public RuleDescLinker::Factory {
	virtual RuleDescLinker *build() { return _new SpanishRuleDescLinker(); }
};

#endif

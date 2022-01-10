// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_DESC_LINKER_H
#define en_DESC_LINKER_H

#include "Generic/edt/DescLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/DebugStream.h"

/**
 * Rule-based methods for associating descriptor
 * mentions with entities
 */
class EnglishDescLinker : public DescLinker {
private:
	friend class EnglishDescLinkerFactory;

public:
	virtual ~EnglishDescLinker() {};
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
	EnglishDescLinker();
	// match ment to an entity in ents by looking at heads of mentions
	EntityGuess* _headMatch(Mention* ment, LexEntitySet* ents, EntityType linkType);

	// match ment to an entity if there is a singleton name mention in the mention's premod
	EntityGuess* _unlinkedPremodNameMatch(Mention* ment, LexEntitySet* ents, EntityType linkType);

	// TODO: move this to some utility class?
	// utility needed for matching descs to names
	// we want the last premod in the name mention
	Symbol _getFinalPremodWord(const SynNode* node) const ;

	// detect and return numbers in the premods
	const SynNode* _getNumericPremod(const SynNode* node) const ;

	// detect and return numbers in the postmods
	const SynNode* _getNumericPostmod(const SynNode* node) const ;

	// true if there is at least one premod of ment that has a mention that is a name entity
	bool _hasPremodName(Mention* ment, LexEntitySet* ents);

	// true if the premods of ment1 and ment2 have mentions of name entities that are not the same
	// entities. (That. btw, is why the LexEntitySet is needed - to identify mentions from synnodes, and
	// to then identify entity ids)
	bool _hasPremodNameEntityClash(Mention* ment1, Mention* ment2, LexEntitySet* ents);

	// true if premods of ment have a name mention which belongs to a different entity than name.
	bool _nameClashesWithPremods(Mention* name, Mention* ment, LexEntitySet* ents);

	// create a "new entity" guess from this mention
	EntityGuess* _guessNewEntity(Mention* ment, EntityType linkType);

	// true if the entity has a mention with numeric premods that clash with the node
	// Change (Mike): now we need the LexEntitySet to resolve mention IDs
	bool _hasNumericPremodClash(const SynNode* node, Entity* ent, LexEntitySet *ents);

	// true if the entity has a mention with numeric postmods that clash with the node
	// Change (Mike): now we need the LexEntitySet to resolve mention IDs
	bool _hasNumericPostmodClash(const SynNode* node, Entity* ent, LexEntitySet *ents);

	// true if certain words appear in the premods of the mention
	bool _premodsHaveAntiLinkWords(Mention* ment);

	DebugStream _debug;

};

class EnglishDescLinkerFactory: public DescLinker::Factory {
	virtual DescLinker *build() { return _new EnglishDescLinker(); }
};

#endif


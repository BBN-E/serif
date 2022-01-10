#ifndef MENTION_CONFIDENCE_H
#define MENTION_CONFIDENCE_H

#include "Generic/common/Attribute.h"
#include "Generic/theories/Mention.h"

class SentenceTheory;
class DocTheory;
class MentionSet;
class Proposition;
class PropositionSet;
class Entity;
class EntitySet;
class TokenSequence;
class SynNode;

class MentionConfidence {
public:
	/* MRF 
		The following functions borrow heavily from PreLinker.cpp and En_PreLinker.cpp. They may provide useful information about
		how much we should trust pronoun/descriptor co-reference
		Warning: Aspects may be English specific
	*/
	
	static MentionConfidenceAttribute determineMentionConfidence(const DocTheory *docTheory, const SentenceTheory *sentTheory, const Mention *ment, const std::set<Symbol>& ambiguousLastNames=std::set<Symbol>());

private:

	static bool enIsNominalPremod(const SynNode *node);
	static const Mention* nameFromTitleDescriptor(const Mention* desc_mention, const Entity *ent, const MentionSet* mentionSet);
	static bool isTitleDescriptor(const Mention* desc_mention, const Entity *ent, const MentionSet* mentionSet);
	static const Mention* nameFromCopulaDescriptor(const Mention* ment, const Entity* ent, const PropositionSet* prop_set, const MentionSet* mentionSet);
	static bool isCopulaDescriptor(const Mention* ment, const Entity* ent, const PropositionSet* prop_set, const MentionSet* mentionSet);
	static const Mention * nameFromApposDescriptor(const Mention* ment, const Entity* ent);
	static bool isApposDescriptor(const Mention* ment, const Entity* ent);
	static const Mention* getWHQLink(const Mention *currMention, const MentionSet *mentionSet);
	static const Mention* nameFromPronNameandPos(const Mention *ment, const Entity *ent, const MentionSet * mentionSet, const EntitySet* ent_set, const TokenSequence *toks );
	static bool isPronNameandPos(const Mention *ment, const Entity *ent, const MentionSet * mentionSet, const EntitySet* ent_set, const TokenSequence *toks);
	static const Mention* nameFromDoubleSubjectPersonPron(const Mention *ment, const Entity *ent, const MentionSet * mentionSet, const EntitySet* ent_set, const PropositionSet * propSet);
	static std::string getDebugSentenceString(const DocTheory *doc_theory, int sent_no);
	static std::string getDebugTextFromMention(const DocTheory *docTheory, const Mention *ment);
	static std::vector<const Mention *> mentionsMTypeEType(const DocTheory *docTheory, const Mention::Type mType, EntityType entType, int sentNo);
	static bool isMentInSentWithArgRole(const Mention *ment, const DocTheory *docTheory, Symbol role, int sent_no);
	static bool isMentPrevSentDoubleSubj(const Mention *ment, const Entity *ent, const EntitySet *entitySet, const DocTheory *docTheory);
	static bool isDoubleSubjectPersonPron(const Mention *ment, const Entity *ent, const MentionSet * mentSet, const EntitySet* ent_set, const PropositionSet * propSet);
	static bool entityNamePrecedesMentSentence(const Mention* ment, const Entity *ent, const EntitySet *ent_set);
	static bool entityNamePrecedesMentInSentence(const Mention* ment, const Entity *ent, const EntitySet *entitySet, const MentionSet* mentionSet);
	static bool isMentOnlyPrecedingTypeMatch(const Mention* ment, EntityType entType, const Entity *ent, const EntitySet *ent_set);
	static bool namePrecedesMentAndIsOnlyPrecedingTypeMatch(const Mention* ment, EntityType entType, const Entity *ent,const EntitySet *entitySet, const MentionSet *mentionSet, const DocTheory *docTheory);



};

#endif

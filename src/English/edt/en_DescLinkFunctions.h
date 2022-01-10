// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_DESCLINKFUNCTIONS_H
#define en_DESCLINKFUNCTIONS_H
#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/theories/EntitySet.h"

class EnglishDescLinkFunctions : public DescLinkFunctions {
private:
	friend class EnglishDescLinkFunctionsFactory;

public:
	static void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize);
private:
	static bool _testHeadWordMatch(Mention *ment, Entity *entity, EntitySet *entitySet);
	static bool _testHeadWordNodeMatch(Mention *ment, Entity *entity, EntitySet *entitySet);

	static Symbol _getUniqueModifierRatio(Mention *ment, Entity *entity, EntitySet *entitySet);
	static Symbol _getDistance(int sent_num, Entity *entity, EntitySet *entitySet);
	static bool _exactStringMatch(Mention *ment, Entity *entity, EntitySet *entitySet);
	static int _getHeadNPWords(const SynNode* node, Symbol results[], int max_results);
	static void _testNumericPremods(OldMaxEntEvent *e, Mention *ment, Entity *ent, EntitySet *entitySet);
	static const SynNode* _getNumericPremod(Mention *ment);
	static bool _hasPremod(Mention* ment);
	static int _getPremods(Mention* ment, Symbol results[], const int max_results);

	// how many entities of this type are in the set?
	static Symbol _getNumEnts(EntityType type, EntitySet *set);

	// premods of the mention
	// for mention and all entity mentions
	static void _addPremodList(Mention* ment, OldMaxEntEvent* e);
	static void _addPremodList(Entity* ent, EntitySet* ents, OldMaxEntEvent* e);
	// head of the parent node (hopefully predicate)
	// for mention and all entity mentions
	static Symbol _getParentHead(Mention* ment);
	static void _addParentHead(Mention* ment, OldMaxEntEvent* e);
	static void _addParentHead(Entity* ent, EntitySet* ents, OldMaxEntEvent* e);
	static bool _hasPremodClash(Mention* ment, Entity* ent, EntitySet* ents);
	static bool _hasPremodMatch(Mention* ment, Entity* ent, EntitySet* ents, Symbol* word);
};

class EnglishDescLinkFunctionsFactory: public DescLinkFunctions::Factory {
	virtual void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize) {  EnglishDescLinkFunctions::getEventContext(entSet, mention, entity, evt, maxSize); }
};


#endif

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_DESCLINKFUNCTIONS_H
#define es_DESCLINKFUNCTIONS_H

#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/SymbolHash.h"

class SpanishDescLinkFunctions : public DescLinkFunctions {
private:
	friend class SpanishDescLinkFunctionsFactory;

public:
	static void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize);
private:
	static bool _testHeadWordMatch(Mention *ment, Entity *entity, EntitySet *entitySet);
	static bool _testHeadWordNodeMatch(Mention *ment, Entity *entity, EntitySet *entitySet);
	static bool _lastHanziMatch(Mention *ment, Entity *entity, EntitySet *entitySet);

	static Symbol _getUniqueHanziRatio(Mention *ment, Entity *entity, EntitySet *entitySet);
	static Symbol _getUniqueModifierRatio(Mention *ment, Entity *entity, EntitySet *entitySet);
	static Symbol _getDistance(int sent_num, Entity *entity, EntitySet *entitySet);
	static bool _exactStringMatch(Mention *ment, Entity *entity, EntitySet *entitySet);
	static int _getHeadNPWords(const SynNode* node, Symbol results[], int max_results);
	static const SynNode* _getNumericPremod(Mention *ment);
	static bool _hasPremodName(Mention* ment, EntitySet* ents);
	static bool _hasPremod(Mention* ment);
		// the following borrowed loosely from es_DescLinker
	static bool _hasPremodNameEntityClash(Mention* ment, Entity* ent, EntitySet* ents);
	static int _getMentionInternalNames(const SynNode* node, const MentionSet* ms, const SynNode* results[], const int max_results);
	static int _getPremodNames(Mention* ment, EntitySet* ents, const SynNode* results[], const int max_results);
	static int _getPremods(Mention* ment, Symbol results[], const int max_results);

	// how many entities of this type are in the set?
	static Symbol _getNumEnts(EntityType type, EntitySet *set);

	// premods of the mention
	// for mention and all entity mentions
	static int _findMentPremods(Mention* ment, Symbol *results, const int max_results);
	static Symbol _findMentHeadWord(Mention *ment);
	static int _findEntHeadWords(Entity* ent, EntitySet* ents, Symbol *results, const int max_results);
	static int _findEntPremods(Entity* ent, EntitySet* ents, Symbol *results, const int max_results);

	// head of the parent node (hopefully predicate)
	// for mention and all entity mentions
	static Symbol _getParentHead(Mention* ment);
	static Symbol _findMentParent(Mention* ment);
	static int _findEntParents(Entity* ent, EntitySet* ents, Symbol *results, const int max_results);
	static Symbol _getCommonParent(Mention* ment, Entity* ent, EntitySet* ents);

	static bool _hasPremodClash(Mention* ment, Entity* ent, EntitySet* ents);
	static bool _hasPremodMatch(Mention* ment, Entity* ent, EntitySet* ents);

	static bool _mentHasNumeric(Mention *ment);
	static bool _entHasNumeric(Entity *ent, EntitySet *entitySet);
	static bool _hasNumericClash(Mention *ment, Entity *ent, EntitySet *entitySet); 
	static bool _hasNumericMatch(Mention *ment, Entity *ent, EntitySet *entitySet); 

	static bool _entityContainsOnlyNames(Entity *entity, EntitySet *entitySet);
	static bool _mentOverlapsEntity(Mention *mention, Entity *entity, EntitySet *entSet);

	static Symbol _getClusterScore(Mention *ment, Entity *ent, EntitySet *entSet); 
	static bool _hasClusterMatch(Mention *ment, Entity *ent, EntitySet *entSet) ;
	static int _findMentClusters(Mention *ment, Symbol *results, const int max_results); 
	static int _findEntClusters(Entity* ent, EntitySet* ents, Symbol *results, const int max_results);
	static int _getClusters(Symbol word, Symbol *results, const int max_results); 


	struct HashKey {
        size_t operator()(const Symbol s) const {
			return s.hash_code();
        }
    };

    struct EqualKey {
        bool operator()(const Symbol s1, const Symbol s2) const {
			return s1 == s2;
		}
    };

	typedef serif::hash_map<Symbol, Symbol*, HashKey, EqualKey> WordClusterTable;
	static WordClusterTable *_clusterTable;
	static void initializeWordClusterTable();


	static SymbolHash *_stopWords;
	static void initializeStopWords();
};

class SpanishDescLinkFunctionsFactory: public DescLinkFunctions::Factory {
	virtual void getEventContext(EntitySet* entSet, Mention* mention, Entity* entity, OldMaxEntEvent* evt, int maxSize) {  SpanishDescLinkFunctions::getEventContext(entSet, mention, entity, evt, maxSize); }
};


#endif

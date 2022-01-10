// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___SNODECONTENT_H___
#define ___SNODECONTENT_H___


#include <float.h>

#include "Generic/SPropTrees/Common.h"

//#include "Generic/distill-generic/utilities/mempool.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
//#include "Generic/wordnet/xx_WordNet.h"
#include "boost/pool/poolfwd.hpp"

class STreeNode;
class Entity;

typedef std::pair<const STreeNode*,EntityType> MentionInfo;
typedef std::map<const Mention*,const Entity*> AltMentions;
typedef std::multimap<const Mention*,MentionInfo> RelevantMentions;

class SNodeContent {
	friend class STreeNode;
public:
	friend class boost::object_pool<SNodeContent>;
	static boost::object_pool<SNodeContent>* _nctPool;
	static void initializeMemoryPool();
	static void clearMemoryPool();
	//static mempool* nodeContentPool;
	//static WordNet* wn;
	enum ContentType {
		EMPTY = 0,
		PHRASE,
		ENTITY,
		PROPOSITION,
		ACE_ENTITY,
		ACE_EVENT
	};
	enum Language {
		ENGLISH,
		ARABIC,
		CHINESE,
		UNKNOWN
	};
protected:
	ContentType _type;
	mutable bool _hasName;
	const Proposition* _theProp;
	const SynNode* _theSynnode;
	const Entity* _theEntity;
	std::wstring _thePhrase; //this is a mimimal-phrase, i.e. for "recent troops that entered Iraq" it will store "troops"
					   //and for propositions it will contain their predicate head
	AltMentions _theMentions;
public:
	static std::wstring dummy;
	bool hasName() const { return _hasName; }
	const AltMentions& getMentions() const { return _theMentions; }
	const SNodeContent::ContentType getType() const { return _type; }
	const Proposition* getProposition() const {
		if ( _type != PROPOSITION ) { /* std::cerr << "Attempt to obtain content proposition from a wrong node!\n"; */ return 0; }
		else return _theProp;
	}
	const Entity* getEntity() const {
		if ( _type != ENTITY ) { /* std::cerr << "Attempt to obtain content mention from a wrong node!\n"; */ return 0; }
		else return _theEntity;
	}
	const SynNode* getSynNode() const { return _theSynnode; }
	//get the first (and in this case supposingly only) phrase in the set
	const std::wstring& getPhrase() const { return _thePhrase; }
};

#endif

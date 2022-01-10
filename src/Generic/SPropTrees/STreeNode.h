// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___STREENODE_H___
#define ___STREENODE_H___

#include <float.h>

#include "Generic/SPropTrees/SNodeContent.h"

#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"

class SPropTree;
class STreeNode;
class Sexp;

typedef std::multimap<std::wstring,STreeNode*> Roles;

//information that is common and accessible for the whole tree
struct CommonInfo {
	const SentenceTheory* senTheory;
	const DocTheory* docTheory;
	SNodeContent::Language _language;
};


class STreeNode {
	friend class SPropTree;

	static long _FIRST_UNUSED_ID;
	long _id;
	bool _isGood;
	int size; //total number of nodes in the subtree that I am parent of (including myself)
	const STreeNode* _parent;
	std::wstring _role;
	CommonInfo* _common;
	Roles _parts;
	SNodeContent* _content;
	bool _debug;
	static bool _ALLOW_EXPANSION;
	STreeNode();
	~STreeNode();
	STreeNode(const Proposition*, const CommonInfo*);
	STreeNode(const Sexp*, const CommonInfo*, const STreeNode*);
	STreeNode(const Proposition*, const STreeNode*, const std::wstring& =L"");
	STreeNode(const Mention*, const STreeNode*, const std::wstring& =L"");
	STreeNode(const SynNode*, const STreeNode*, const std::wstring& =L"");
	STreeNode(const std::wstring&, const STreeNode*, const std::wstring& =L"");
public:
	friend class boost::object_pool<STreeNode>;
	static boost::object_pool<STreeNode>* _tndPool;
	//static mempool* treeNodePool;
	static void initializeMemoryPool();
	static void clearMemoryPool();
	const Mention* findEntityMentionForSynNode(const SynNode* sn);
	long getID() const { return _id; }
	std::wstring toString(bool=true) const;
	//returns number of subnodes in the subtree spanned by this node
	int getSize() const { return size; }
	std::wstring getRole() const { return _role; }
	const STreeNode* getParent() const { return _parent; }
	const SNodeContent* getContent() const { return _content; }
	const SynNode* getSynNode() const { return _content ? _content->getSynNode() : 0; }
	const Entity* getEntity() const { return _content ? _content->getEntity() : 0; }
	int populateChildren();
	const Proposition* getProposition() const { return _content ? _content->getProposition() : 0; }
	int getPropositionID() const { return ( _content && _content->_theProp ) ? _content->_theProp->getID() : -1; }
	int collectMentions();
	//size_t getAllMentions(AltMentions& mns, const STreeNode* tn, bool onlyHead, Mention::Type mtype=Mention::NONE) const;
	size_t getAllEntityMentions(RelevantMentions& rmns, bool unique=true, bool onlyHead=false, const std::wstring& etypeName=L"UNDET", Mention::Type mtype=Mention::NONE) const;
	std::wstring getEntityType() const;
	void serialize(std::wostream&, int) const;
	void serialize(std::wstring&, int) const;
	static void allowExpansion(bool e) { _ALLOW_EXPANSION = e; }
	const Roles& getRoles() const { return _parts; }
	void addMention(const Mention* m) { 
		if ( _content && m && _common && _common->docTheory ) 
			_content->_theMentions[m] = _common->docTheory->getEntitySet()->getEntityByMention(m->getUID()); 
	}
	static std::wstring getSynNodeText(const SynNode*);
	static const SynNode* getAtomicHeadGeneral(const SynNode* sn);
	static const Proposition* getNameOrNounPropositionForMention(const SentenceTheory* st, const Mention* m);
	static const Proposition* getNameOrNounPropositionForSynNode(const SentenceTheory* st, const SynNode* sn);

};

#endif

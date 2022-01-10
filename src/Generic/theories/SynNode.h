// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYN_NODE_H
#define SYN_NODE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/Attribute.h"
#include "Generic/common/version.h"
#include "Generic/theories/TokenSequence.h"
#include <string>
#include <vector>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class MentionSet;
class Mention;

// This is a node in the parse tree (like ParseNode). It has links
// to its parent and children. It has a notion of a head node (a
// special child node), but could be used without using this feature.
// Another optional feature is the _mention_index, which provides
// the index of the mention which is associated with a given node
// (see MentionSet.h).

class SERIF_EXPORTED SynNode : public Theory {

protected:
	SynNode **_children;
	SynNode *_parent;

	Symbol _tag;

	// Note: we intentionally declare all the int fields together,
	// to allow better memory alignment (reduced padding) on 64-bit
	// builds.
	int _ID;	// These IDs are sentence-level
	int _head_index;
	int _n_children;
	// An inclusive range of token numbers in the sentence:
	int _start_token, _end_token;
	int _mention_index;

public:
	// To construct these, use the constructor, and then set each
	// child use setChild() or setChildren(). You should also use
	// setTokenSpan() and setHeadIndex().
	// (Or, if this doesn't suit your needs, you could add a
	// different constructor.)
	SynNode(int ID, SynNode *parent, const Symbol &tag, int n_children);
	SynNode();
	SynNode(const SynNode &other, SynNode *parent);
	~SynNode();

	// accessors for setting stuff
	void setTag(const Symbol &tag) { _tag = tag; }
	void setChild(int i, SynNode *child);
	void setChildren(int n, int head_index, SynNode *children[]);
	void setTokenSpan(int start, int end) { _start_token = start;
	                                        _end_token = end; }
	void setMentionIndex(int mention_index) { _mention_index = mention_index; }
	void setHeadIndex(int head_index) { _head_index = head_index; }
	void setParent(SynNode *parent) { _parent = parent; }

	// accessors for getting stuff
	int getID() const { return _ID; }
	const Symbol &getTag() const { return _tag; }

	const SynNode *getParent() const { return _parent; }
	int getNChildren() const { return _n_children; }
	const SynNode *getChild(int i) const;
	const SynNode *const *getChildren() const { return _children; }
	const SynNode *getHead() const { if (_head_index >= 0 && _head_index < _n_children) return getChild(_head_index); else return 0; }
	int getHeadIndex() const { return _head_index; }

	/** Return true if this SynNode has a parent, and is the head child of its parent. */
	bool isHeadChild() const { return ((_parent != NULL) && (_parent->getHead() == this)); }

	bool isTerminal() const { return _n_children == 0; }
	bool isPreterminal() const;

	/** This one returns a single token covered by a node, even if it's several
	  * levels down. If the node covers more than one token, it returns Symbol();
	  * i.e., a blank Symbol. */
	Symbol getSingleWord() const;
	/** recursively descend heads */
	const SynNode *getHeadPreterm() const;
	virtual const Symbol &getHeadWord() const;
	const SynNode *getHighestHead() const;

	/** counts terminals (tokens) */
	int getNTerminals() const { return _end_token - _start_token + 1; }
	/** This takes a pointer to an array of symbols and the size of that array
	  * and populates it (without overflowing) with the terminal symbols under
	  * the node, returning the number of those symbols. */
	int getTerminalSymbols(Symbol results[], int max_results) const;
	/** Returns a vector of the terminal Symbols under the node. */
	std::vector<Symbol> getTerminalSymbols() const;
	/** This takes a pointer to an array of symbols and the size of that array
	  * and populates it (without overflowing) with the part-of-speech (preterminal) 
	  * symbols under the node, returning the number of those symbols. */
	int getPOSSymbols(Symbol results[], int max_results) const;
	/** This takes a start token offset and an end token offset and returns the
	  * highest node that has the exact offset,  if there is no match, return 0*/
	const SynNode *getNodeByTokenSpan(int starttoken, int endtoken) const;
	/** This takes a start token offset and an end token offset and returns the
	  * lowest node that covers the entire span,  if there is no match, return 0*/
	const SynNode *getCoveringNodeFromTokenSpan(int starttoken, int endtoken) const;
	/** This finds the number of nodes between an ancestor and child.
	  * If node is not descendant of ancestor, return -1
	  */
	int getAncestorDistance(const SynNode* ancestor) const;
	bool isAncestorOf(const SynNode* node) const; 

	void getDescendantMentionIndices(std::vector<int>& mention_indices) const;

	int getStartToken() const { return _start_token; }
	int getEndToken() const { return _end_token; }
	int getMentionIndex() const { return _mention_index; }

	// Functions that deal with siblings
	bool hasNextSibling() const;
	const SynNode* getNextSibling() const;

	/** Return first terminal node in subtree (0 if none) */
	const SynNode *getFirstTerminal() const;
	/** Opposite of above */
	const SynNode *getLastTerminal() const;
	/** Return next terminal node following given node (0 if none) */
	const SynNode *getNextTerminal() const;
	/** Opposite of above */
	const SynNode *getPrevTerminal() const;
	const SynNode *getNthTerminal(int n) const;

	/** Populate the provided array with this node and all its descendants. */
	void getAllNonterminalNodes(std::vector<const SynNode*>& nodes) const;
	void getAllNonterminalNodes( int * tokenStarts, int * tokenEnds, Symbol * tags, int & n_nodes, int max_nodes ) const;
	void getAllNonterminalNodesWithHeads( int * heads, int * tokenStarts, int * tokenEnds, Symbol * tags, int & n_nodes, int max_nodes ) const;
	void getAllTerminalNodes(std::vector<const SynNode*>& nodes) const;

	// Use this to see if this node has a valid mention index:
	bool hasMention() const { return _mention_index != -1; }

	std::string toDebugString(int indent) const ;
	std::wstring toString(int indent) const ;
	// prints out the pretty-printed parse without token offsets, etc.
	std::wstring toPrettyParse(int indent) const ;
	std::wstring toPrettyParseWithMentions(const Mention *ment, Symbol label, int indent) const;
	std::wstring toPrettyParseWithMentions(const Mention *ment1, Symbol label1, const Mention *ment2, Symbol label2, int indent) const;
	// prints out non-pretty printed parse without token offsets, etc.
	std::wstring toFlatString() const ;
	// prints out just the words (with trailing space) 
	// returns an empty string when called on a terminal
	std::wstring toTextString() const ;
	std::wstring toCasedTextString(const TokenSequence* ts) const;  // As above, but with casing and no trailing space
	std::wstring toEnamexString(MentionSet *mentionSet, Symbol headType) const;
	//prints out ( word pos start end) ( word pos start end)....
	std::wstring toPOSString(const TokenSequence *ts) const;

	// prints out training for coref 2005
	std::wstring toAugmentedParseString(TokenSequence *tokenSequence,
										MentionSet *mentionSet, 
										int indent) const;

	std::string toDebugTextString() const ;

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const SynNode &it)
		{ it.dump(out, 0); return out; }


	// For saving state:
	void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	SynNode(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit SynNode(SerifXML::XMLTheoryElement elem, SynNode* parent, int &node_id_counter);
	const wchar_t* XMLIdentifierPrefix() const;

	static bool isNounTag(const Symbol& tag, LanguageAttribute language=SerifVersion::getSerifLanguage());
	static bool isVerbTag(const Symbol& tag, LanguageAttribute language=SerifVersion::getSerifLanguage());
	static bool isSentenceLikeTag(const Symbol& tag, LanguageAttribute language=SerifVersion::getSerifLanguage());
};

#endif

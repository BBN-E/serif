// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"

#include <iostream>


class Mention;
class MentionSet;
class Proposition;
class SynNode;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Argument : public Theory {

public:
	/** The three types of arguments */
	typedef enum {MENTION_ARG,
			      PROPOSITION_ARG,
			      TEXT_ARG}
		Type;

	static const Symbol REF_ROLE;
	static const Symbol SUB_ROLE;
	static const Symbol OBJ_ROLE;
	static const Symbol IOBJ_ROLE;
	static const Symbol POSS_ROLE;
	static const Symbol TEMP_ROLE;
	static const Symbol LOC_ROLE;
	static const Symbol MEMBER_ROLE;
	static const Symbol UNKNOWN_ROLE;


public:
	/** Arguments exist in arrays in Propositions, so the default constructor
	  * is always used. They turn into meaningful objects when one of the
	  * populateWith...() methods below is called.
	  */
	Argument() {}
	void populateWithMention(Symbol roleSym,
							 int mention_id);
	void populateWithProposition(Symbol roleSym,
								 const Proposition *proposition);
	void populateWithText(Symbol roleSym,
						  const SynNode *node);

	Type getType() const { return _type; }

	Symbol getRoleSym() const { return _roleSym; }

	int getMentionIndex() const;
	const Mention *getMention(const MentionSet *mentionSet) const;
	const Proposition *getProposition() const;
	const SynNode *getNode() const;
	//mrf convenince accessors for getting the start and end offsets of an argument
	int getStartToken(const MentionSet *mentionSet) const;
	int getEndToken(const MentionSet *mentionSet) const;


	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const Argument &it)
		{ it.dump(out, 0); return out; }
	void dump(UTF8OutputStream &out, int indent = 0) const;
	friend UTF8OutputStream &operator <<(UTF8OutputStream &out, const Argument &it)
		{ it.dump(out, 0); return out; }

	std::wstring toString(int indent = 0);
	std::string toDebugString(int indent = 0) const;


	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void loadState(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	void loadXML(SerifXML::XMLTheoryElement elem);
	void resolvePointers(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
	virtual bool hasXMLId() const { return false; }

private:
	Type _type;

	Symbol _roleSym;

	union {
		int mention_index;			/// for _type == MENTION_ARG
		const Proposition *prop;	/// for _type == PROPOSITION_ARG
		const SynNode *node;		/// for _type == TEXT_ARG
	} _arg;
};

#endif

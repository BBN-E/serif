// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FACT_ARGUMENT_H
#define FACT_ARGUMENT_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/ValueMention.h"

class DocTheory;

class FactArgument : public Theory {
public:

	FactArgument(Symbol role);
	FactArgument(const FactArgument &other);
	~FactArgument();

	Symbol getRole() { return _role; }
	void setRole(Symbol role) { _role = role; }

	// State file serialization (not currently implemented)
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	virtual void resolvePointers(StateLoader * stateLoader);

	FactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const { return L"factargument"; }



private:
	Symbol _role;
};
typedef boost::shared_ptr<FactArgument> FactArgument_ptr;


class MentionFactArgument : public FactArgument {

public:
	MentionFactArgument(Symbol role, MentionUID mention_uid);
	MentionFactArgument(const MentionFactArgument &other);
	MentionFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;

	MentionUID getMentionID() { return _mention_uid; }
	const Mention* getMentionFromMentionSet(const DocTheory *docTheory);

private:
	MentionUID _mention_uid;
};
typedef boost::shared_ptr<MentionFactArgument> MentionFactArgument_ptr;


class ValueMentionFactArgument : public FactArgument {

public:
	ValueMentionFactArgument(Symbol role, const ValueMention *valueMention, bool is_doc_date);
	ValueMentionFactArgument(const ValueMentionFactArgument &other);
	ValueMentionFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;

	const ValueMention* getValueMention() { return _valueMention; }
	bool isDocDate() { return _is_doc_date; }

private:
	const ValueMention* _valueMention;
	bool _is_doc_date;
};
typedef boost::shared_ptr<ValueMentionFactArgument> ValueMentionFactArgument_ptr;


class TextSpanFactArgument : public FactArgument {
public:
	TextSpanFactArgument(Symbol role, int start_sentence, int end_sentence, int start_token, int end_token);
	TextSpanFactArgument(const TextSpanFactArgument &other);
	TextSpanFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	
	int getStartSentence() { return _start_sentence; }
	int getEndSentence() { return _end_sentence; }
	int getStartToken() { return _start_token; }
	int getEndToken() { return _end_token; }

	std::wstring getStringFromDocTheory(const DocTheory *docTheory);

private:
	int _start_sentence;
	int _end_sentence;
	int _start_token;
	int _end_token;
};
typedef boost::shared_ptr<TextSpanFactArgument> TextSpanFactArgument_ptr;

class StringFactArgument : public FactArgument {
public:
	StringFactArgument(Symbol role, std::wstring str);
	StringFactArgument(const StringFactArgument &other);
	StringFactArgument(SerifXML::XMLTheoryElement elem, const DocTheory* theory=0);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;

	std::wstring getString() { return _string; }

private:
	std::wstring _string;
};
typedef boost::shared_ptr<StringFactArgument> StringFactArgument_ptr;

#endif 

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_MENTION_H
#define CORRECT_MENTION_H

#include "Generic/common/Symbol.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/Mention.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"

class CorrectMention {

public:

	CorrectMention();
	void loadFromSexp(Sexp *mentionSexp);
	//void loadFromERE(ERE *mentionERE);

private:

	// once we use the CorrectMention, we'd like to know what tokens it got mapped to
	int start_token;
	int end_token;
	int head_start_token;
	int head_end_token;

	EDTOffset start;
	EDTOffset end;
	EDTOffset head_start;
	EDTOffset head_end;
	Symbol annotationID;

	// used for matching up CorrectMentions with Mentions
	const SynNode *bestHeadSynNode;

	bool printed;

	int sentence_number;

	Mention::Type mentionType;
	
	// this will be one of the types in CASymbolicConstants
	Symbol correctMentionType;
	Symbol role;
	
	NameSpan *nameSpan;
	
	std::vector<Mention::UID> system_mention_ids;

public:

	bool isName() { return (correctMentionType == CASymbolicConstants::NAME_LOWER ||
							correctMentionType == CASymbolicConstants::NAME_UPPER); }
	bool isName_ERE() { return (correctMentionType == CASymbolicConstants::NAME_ERE); }

	bool isNominal() { return (correctMentionType == CASymbolicConstants::NOMINAL_LOWER ||
							   correctMentionType == CASymbolicConstants::NOMINAL_UPPER ||
							   correctMentionType == CASymbolicConstants::NOM_PRE_UPPER); }

	bool isNominal_ERE() { return (correctMentionType == CASymbolicConstants::NOM); }

	bool isNominalPremod() { return (correctMentionType == CASymbolicConstants::NOM_PRE_UPPER); }
	bool isPronoun() { return (correctMentionType == CASymbolicConstants::PRONOUN_LOWER ||
							   correctMentionType == CASymbolicConstants::PRONOUN_UPPER); }

	bool isPronoun_ERE() { return (correctMentionType == CASymbolicConstants::PRO ); }

	bool isDateTime() { return (correctMentionType == CASymbolicConstants::DATETIME_LOWER ||
								correctMentionType == CASymbolicConstants::DATETIME_UPPER); }
	bool isUndefined_ERE(){return (correctMentionType==CASymbolicConstants::NONE_ERE);}
	bool isUndifferentiatedPremod() { 
		return (correctMentionType == CASymbolicConstants::PRE_UPPER); }

	bool setStartAndEndTokens(TokenSequence *tokenSequence);

	void addSystemMentionId(Mention::UID id);
	int getNSystemMentionIds();
	Mention::UID getSystemMentionId(int index);

	int getStartToken() const { return start_token; }
	int getEndToken() const { return end_token; }
	int getHeadStartToken() const { return head_start_token; }
	int getHeadEndToken() const { return head_end_token; }
	
	EDTOffset getStartOffset() const { return start; }
	EDTOffset getEndOffset() const { return end; }
	EDTOffset getHeadStartOffset() const { return head_start; }
	EDTOffset getHeadEndOffset() const { return head_end; }

	Symbol getAnnotationID() const { return annotationID; }

	void setBestHeadSynNode(const SynNode *node) { bestHeadSynNode = node; }
	const SynNode* getBestHeadSynNode() const { return bestHeadSynNode; }

	void setIsPrintedFlag() { printed = true; }
	bool isPrinted() const { return printed; }

	void setSentenceNumber(int sent_no) { sentence_number = sent_no; }
	int getSentenceNumber() { return sentence_number; }

	void setMentionType(Mention::Type type) { mentionType = type; }
	Mention::Type getMentionType() const { return mentionType; }

	Symbol getCorrectMentionType() const { return correctMentionType; }
	Symbol getRole() const { return role; }

	void setNameSpan(NameSpan *span) { nameSpan = span; }
	NameSpan* getNameSpan() const { return nameSpan; }

};


#endif

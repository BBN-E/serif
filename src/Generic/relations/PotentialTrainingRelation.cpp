// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Proposition.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/relations/PotentialTrainingRelation.h"

using namespace std;

Symbol PotentialTrainingRelation::UNDET_RELATION_SYM = Symbol(L"---------");
Symbol PotentialTrainingRelation::REVERSED_SYM = Symbol(L"reversed");

PotentialTrainingRelation::PotentialTrainingRelation() : _sentence(0) {
	_docName = Symbol(L"");
	_type1 = _type2 = EntityType::getUndetType();
	_start1 = _start2 = _end1 = _end2 = -1;
	_relationType = RelationConstants::NONE;
	_leftMention = MentionUID();
	_rightMention = MentionUID();
	_relation_reversed = false;
	_args_reversed = false;
}

PotentialTrainingRelation::PotentialTrainingRelation(const PotentialTrainingRelation &other) 
													: _sentence(0) 
{
	*this = other;
}

PotentialTrainingRelation::PotentialTrainingRelation(UTF8InputStream& in) 
													: _sentence(0)
{
	this->read(in);
}

PotentialTrainingRelation::PotentialTrainingRelation(Argument *first, Argument *second,
													 DocTheory *docTheory, const int sent_no,
													 bool leftMetonymy, bool rightMetonymy) 
													 : _sentence(0)
{
	MentionSet *mentionSet = docTheory->getSentenceTheory(sent_no)->getMentionSet();
	TokenSequence *tokenSequence = docTheory->getSentenceTheory(sent_no)->getTokenSequence();

	if (! first->getMention(mentionSet)->isOfRecognizedType() ||
		! second->getMention(mentionSet)->isOfRecognizedType())
		return;

	_docName = docTheory->getDocument()->getName();
	_sentence = getDisplaySentence(docTheory->getSentence(sent_no)->getString()->toString());

	_relationType = RelationConstants::NONE;
	_relation_reversed = false;
	_args_reversed = false;

	Argument *arg1 = first;
	Argument *arg2 = second;
	// Reverse args if they are not in the order they appear in sentence
	const SynNode *firstHead = getEDTHead(first->getMention(mentionSet), mentionSet);
	const SynNode *secondHead = getEDTHead(second->getMention(mentionSet), mentionSet);
	if (firstHead->getStartToken() > secondHead->getStartToken()) {
		arg1 = second;
		arg2 = first;
		_args_reversed = true;
	}
	else if (firstHead->getStartToken() == secondHead->getStartToken() &&
			 firstHead->getEndToken() < secondHead->getEndToken())
	{
		arg1 = second;
		arg2 = first;
		_args_reversed = true;
	}

	const Mention *ment1 = arg1->getMention(mentionSet);
	const Mention *ment2 = arg2->getMention(mentionSet);

	_leftMention = ment1->getUID();
	_rightMention = ment2->getUID();

	_type1 = ment1->getEntityType();
	_type2 = ment2->getEntityType();

	if ((leftMetonymy && !_args_reversed) || (rightMetonymy && _args_reversed))
		_type1 = ment1->getIntendedType();
	if ((rightMetonymy && !_args_reversed) || (leftMetonymy && _args_reversed))
		_type2 = ment2->getIntendedType();
	
	const SynNode* head1 = getEDTHead(ment1, mentionSet);
	const SynNode* head2 = getEDTHead(ment2, mentionSet);

	populateOffsets(docTheory->getSentence(sent_no)->getString(), tokenSequence, head1, head2);	
}

PotentialTrainingRelation::PotentialTrainingRelation(Mention *first, Mention *second,
													 DocTheory *docTheory, const int sent_no,
													 bool leftMetonymy, bool rightMetonymy) 
													 : _sentence(0)
{
	MentionSet *mentionSet = docTheory->getSentenceTheory(sent_no)->getMentionSet();
	TokenSequence *tokenSequence = docTheory->getSentenceTheory(sent_no)->getTokenSequence();

	// need to check for this is calling method - sometimes we want an OTH entity involved 
	// in a relation for later coercion
	/*if (! first->isOfRecognizedType() ||
		! second->isOfRecognizedType())
		return;
	*/

	_docName = docTheory->getDocument()->getName();
	_sentence = getDisplaySentence(docTheory->getSentence(sent_no)->getString()->toString());

	_relationType = RelationConstants::NONE;
	_relation_reversed = false;
	_args_reversed = false;

	Mention *arg1 = first;
	Mention *arg2 = second;
	// Reverse args if they are not in the order they appear in sentence
	const SynNode *firstHead = getEDTHead(first, mentionSet);
	const SynNode *secondHead = getEDTHead(second, mentionSet);
	if (firstHead->getStartToken() > secondHead->getStartToken()) {
		arg1 = second;
		arg2 = first;
		_args_reversed = true;
	}
	else if (firstHead->getStartToken() == secondHead->getStartToken() &&
			 firstHead->getEndToken() < secondHead->getEndToken())
	{
		arg1 = second;
		arg2 = first;
		_args_reversed = true;
	}

	_leftMention = arg1->getUID();
	_rightMention = arg2->getUID();

	_type1 = arg1->getEntityType();
	_type2 = arg2->getEntityType();

	if ((leftMetonymy && !_args_reversed) || (rightMetonymy && _args_reversed))
		_type1 = arg1->getIntendedType();
	if ((rightMetonymy && !_args_reversed) || (leftMetonymy && _args_reversed))
		_type2 = arg2->getIntendedType();
	
	const SynNode* head1 = getEDTHead(arg1, mentionSet);
	const SynNode* head2 = getEDTHead(arg2, mentionSet);

	populateOffsets(docTheory->getSentence(sent_no)->getString(), tokenSequence, head1, head2);	
}


void PotentialTrainingRelation::dump(std::ostream &out, int indent) const {
	out << toDebugString();
}

void PotentialTrainingRelation::dump(UTF8OutputStream &out, int indent) const {
	out << toString();
}

std::wstring PotentialTrainingRelation::toString(int indent) const {
	std::wstring str = L"";
	const short wsz = 100;
	wchar_t wc[100];
	if (_sentence != NULL) {
		str += _docName.to_string();
		str += L"\n";
		str += _sentence;
		str += L"\n";
		swprintf(wc, wsz, L"%d", _edt_start1.value());
		str += wc;
		str += L"\n";
		swprintf(wc, wsz, L"%d", _edt_end1.value());
		str += wc;
		str += L"\n";
		swprintf(wc, wsz, L"%d", _start1);
		str += wc;
		str += L"\n";
		swprintf(wc, wsz, L"%d", _end1 - _start1 + 1);
		str += wc;
		str += L"\n";
		str += _type1.getName().to_string();
		str += L"\n";
		swprintf(wc, wsz, L"%d", _edt_start2.value());
		str += wc;
		str += L"\n";
		swprintf(wc, wsz, L"%d", _edt_end2.value());
		str += wc;
		str += L"\n";
		swprintf(wc, wsz, L"%d", _start2);
		str += wc;
		str += L"\n";
		swprintf(wc, wsz, L"%d", _end2 - _start2 + 1);
		str += wc;
		str += L"\n";
		str += _type2.getName().to_string();
		str += L"\n";
		if (_relationType == RelationConstants::NONE)
			str += UNDET_RELATION_SYM.to_string();
		else
			str += _relationType.to_string();
		str += L"\n";
		if (_relation_reversed)
			str += REVERSED_SYM.to_string();
		str += L"\n";
	}
	else {
		throw InternalInconsistencyException("PotentialTrainingRelation::toString()",
			"PotentialTrainingRelation is not populated");
	}
	return str;
}

std::string PotentialTrainingRelation::toDebugString(int indent) const {
	std::string str = "";
	char c[100];
	if (_sentence != NULL) {
		str += _docName.to_debug_string();
		str += "\n";
		char buf[100000];
		StringTransliterator::transliterateToEnglish(buf, _sentence, 99999);
		str += buf;
		str += "\n";
		sprintf(c, "%d", _edt_start1.value());
		str += c;
		str += "\n";
		sprintf(c, "%d", _edt_end1.value());
		str += c;
		str += "\n";
		sprintf(c, "%d", _start1);
		str += c;
		str += "\n";
		sprintf(c, "%d", _end1 - _start1 + 1);
		str += c;
		str += "\n";
		str += _type1.getName().to_debug_string();
		str += "\n";
		sprintf(c, "%d", _edt_start2.value());
		str += c;
		str += "\n";
		sprintf(c, "%d", _edt_end2.value());
		str += c;
		str += "\n";
		sprintf(c, "%d", _start2);
		str += c;
		str += "\n";
		sprintf(c, "%d", _end2 - _start2 + 1);
		str += c;
		str += "\n";
		str += _type2.getName().to_debug_string();
		str += "\n";
		if (_relationType == RelationConstants::NONE) 
			str += UNDET_RELATION_SYM.to_debug_string();
		else
			str += _relationType.to_debug_string();
		str += "\n";
		if (_relation_reversed)
			str += REVERSED_SYM.to_debug_string();
		str += "\n";
		if (_args_reversed)
			str += "Args are reversed";
		str += "\n";
	}
	else {
		throw InternalInconsistencyException("PotentialTrainingRelation::toDebugString()",
			"PotentialTrainingRelation is not populated");
	}
	return str;
}

void PotentialTrainingRelation::read(UTF8InputStream &in) {
	wchar_t line[200];
	
	// get rid of old sentence
	if (_sentence != NULL)
			delete [] _sentence;

	_args_reversed = false;

	try {
		// Document Name
		in.getLine(line, 200);
		_docName = Symbol(line);
		// Sentence Text
		wchar_t *sent = _new wchar_t[2000];
		in.getLine(sent, 2000);
		_sentence = sent;

		// first EDT offsets
		in.getLine(line, 200);
		_edt_start1 = EDTOffset(_wtoi(line));
		in.getLine(line, 200);	
		_edt_end1 = EDTOffset(_wtoi(line));
		// first display offsets
		in.getLine(line, 200);
		_start1 = _wtoi(line);
		in.getLine(line, 200);
		_end1 = _wtoi(line) + _start1 - 1;
		// first EDT type
		in.getLine(line, 200);
		_type1 = EntityType(Symbol(line));
		
		// second EDT offsets
		in.getLine(line, 200);
		_edt_start2 = EDTOffset(_wtoi(line));
		in.getLine(line, 200);	
		_edt_end2 = EDTOffset(_wtoi(line));
		// first display offsets
		in.getLine(line, 200);
		_start2 = _wtoi(line);
		in.getLine(line, 200);
		_end2 = _wtoi(line) + _start2 - 1;
		// second EDT type
		in.getLine(line, 200);
		_type2 = EntityType(Symbol(line));

		// relation type
		in.getLine(line, 200);
		if (Symbol(line) == UNDET_RELATION_SYM)
			_relationType = RelationConstants::NONE;
		else
			_relationType = Symbol(line);

		// reversed?
		in.getLine(line, 200);
		if (Symbol(line) == REVERSED_SYM)
			_relation_reversed = true;
		else
			_relation_reversed = false;
	}
	catch (UnexpectedInputException &) {
		char message[100];
		sprintf(message, "Error reading relation from document %s", _docName.to_debug_string());
		throw UnexpectedInputException("PotentialTrainingRealtion::read()",
									   message);
	}

}

// borrowed from APFResultCollector
const SynNode* PotentialTrainingRelation::getEDTHead(const Mention* ment, const MentionSet *mentionSet) {
	if (ment == 0)
		throw InternalInconsistencyException("PotentialTrainingRelation::getEDTHead()",
												 "mention does not exist");

	const SynNode* node = ment->node;
	if (ment->mentionType == Mention::NAME) {
		const Mention* menChild = ment;
		const SynNode* head = menChild->node;
		while((menChild = menChild->getChild()) != 0)
			head = menChild->node;			
		return head;
	}
	if (ment->mentionType == Mention::APPO) {
		const Mention* menChild = ment->getChild();
		if (menChild == 0)
			throw InternalInconsistencyException("PotentialTrainingRelation::getEDTHead()",
												 "appositive has no children");
		return getEDTHead(menChild, mentionSet);
	}
	if (ment->mentionType == Mention::LIST) {
		return ment->getNode();
	}

	// descend until we either come upon another mention or a preterminal
	if (!node->isPreterminal())
		do {
			node = node->getHead();
		} while (!node->isPreterminal() && !node->hasMention());
	if (node->isPreterminal())
		return node;
	return getEDTHead(mentionSet->getMentionByNode(node), mentionSet);
}

void PotentialTrainingRelation::populateOffsets(const LocatedString *sent, TokenSequence *tokenSequence, 
												const SynNode *head1, const SynNode *head2) 
{
	_edt_start1 = tokenSequence->getToken(head1->getStartToken())->getStartEDTOffset();
	_edt_start2 = tokenSequence->getToken(head2->getStartToken())->getStartEDTOffset();
	_edt_end1 = tokenSequence->getToken(head1->getEndToken())->getEndEDTOffset();
	_edt_end2 = tokenSequence->getToken(head2->getEndToken())->getEndEDTOffset();

	for (int i = 0; i < sent->length(); i++) {
		if (sent->start<EDTOffset>(i) == _edt_start1) 
			_start1 = i;
		if (sent->start<EDTOffset>(i) == _edt_start2) 
			_start2 = i;
		if (sent->end<EDTOffset>(i) == _edt_end1) 
			_end1 = i;
		if (sent->end<EDTOffset>(i) == _edt_end2)
			_end2 = i;
	}
}

const wchar_t* PotentialTrainingRelation::getDisplaySentence(const wchar_t *sentence) {
	wchar_t *sent = _new wchar_t[wcslen(sentence)+1];
	wcscpy(sent, sentence);
	wchar_t* match = wcschr(sent, L'\n');
	while (match != NULL) {
		*match = L' ';
		match = wcschr(sent, L'\n');
	}
	return sent;
}

const PotentialTrainingRelation &PotentialTrainingRelation::operator=(const PotentialTrainingRelation &other) {
	
	if ( &other != this) {
		// get rid of old sentence and vector
		if (_sentence != NULL)
			delete [] _sentence;

		// Make a copy of other._sentence
		wchar_t *sent = _new wchar_t[wcslen(other._sentence)+1];
		wcscpy(sent, other._sentence);
		_sentence = sent;
		
		_docName = other._docName;
		_start1 = other._start1;
		_start2 = other._start2;
		_end1 = other._end1;
		_end2 = other._end2;
		_edt_start1 = other._edt_start1;
		_edt_start2 = other._edt_start2;
		_edt_end1 = other._edt_end1;
		_edt_end2 = other._edt_end2;
		_type1 = other._type1;
		_type2 = other._type2;
		_relationType = other._relationType;
		_relation_reversed = other._relation_reversed;
		_args_reversed = other._args_reversed;
	}
	return *this;
}

bool PotentialTrainingRelation::operator==(const PotentialTrainingRelation &other) const {
	return (_docName == other._docName &&
			_edt_start1 == other._edt_start1 &&
			_edt_end1 == other._edt_end1 &&
			//_type1 == other._type1 &&
			_edt_start2 == other._edt_start2 &&
			_edt_end2 == other._edt_end2); 
			//_type2 == other._type2);
}

bool PotentialTrainingRelation::operator!=(const PotentialTrainingRelation &other) const {
	return (_docName != other._docName ||
			_edt_start1 != other._edt_start1 ||
			_edt_end1 != other._edt_end1 ||
			//_type1 != other._type1 ||
			_edt_start2 != other._edt_start2 ||
			_edt_end2 != other._edt_end2);
			//_type2 != other._type2);
}







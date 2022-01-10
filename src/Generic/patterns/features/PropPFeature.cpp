// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"

PropPFeature::PropPFeature(Pattern_ptr pattern, const Proposition* prop, int sent_no, const LanguageVariant_ptr& languageVariant)
: PatternFeature(pattern,languageVariant), _proposition(prop), _sent_no(sent_no), _start_token(-1), _end_token(-1) {}

PropPFeature::PropPFeature(const Proposition* prop, int sent_no, int start_token, int end_token, const LanguageVariant_ptr& languageVariant, float confidence)
: PatternFeature(Pattern_ptr(), languageVariant, confidence), _proposition(prop), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}

PropPFeature::PropPFeature(Pattern_ptr pattern, const Proposition* prop, int sent_no)
: PatternFeature(pattern), _proposition(prop), _sent_no(sent_no), _start_token(-1), _end_token(-1) {}

PropPFeature::PropPFeature(const Proposition* prop, int sent_no, int start_token, int end_token, float confidence)
: PatternFeature(Pattern_ptr(), confidence), _proposition(prop), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}

void PropPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	const DocTheory* docTheory = patternMatcher->getDocTheory();
	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();
	MentionSet *mSet = sentenceTheory->getMentionSet();

	out << L"    <focus type=\"prop\"";
    printOffsetsForSpan(patternMatcher, _sent_no, _start_token, _end_token, out);
	out << L" val" << val_sent_no << "=\"" << _sent_no << L"\"";
	out << L" val" << val_id << "=\"" << _proposition->getID() << L"\"";
	out << L" val" << val_confidence << "=\"" << getConfidence() << L"\"";
	out << L" val" << val_start_token << "=\"" << _start_token << L"\"";
	out << L" val" << val_end_token << "=\"" << _end_token << L"\"";
	if (!_proposition->getPredSymbol().is_null())
		out << L" val" << val_extra << "=\"" << getXMLPrintableString(std::wstring(_proposition->getPredSymbol().to_string())) << L"\"";
	else out << L" val" << val_extra << "=\"NONE\"";
	out << L" />\n";

	// let's not do this for now
	/*for (int t = 0; t < _proposition->getNArgs(); t++){
		out << L"    <focus type=\"" << propArgumentSym << "\"";
		if (_proposition->getArg(t)->getType() == Argument::MENTION_ARG) {
			const Mention *ment = _proposition->getArg(t)->getMention(mSet);
			out << L" ";
			printOffsetsForMention(sentenceTheory, ment, out);
			out << L" val" << val_arg_parent_id << "=\"" << _proposition->getID() << L"\"";
			out << L" val" << val_arg_role << "=\"" << getXMLSafeRole(_proposition->getArg(t)->getRoleSym()) << L"\"";
			out << L" val" << val_arg_id << "=\"" << ment->getUID() << L"\"";
			out << L" val" << val_arg_canonical_ref << "=\"" << (ment, docTheory) << L"\"";
			out << L" />\n";
		}
		else{
			out << L" val" << val_arg_parent_id << "=\"" << _proposition->getID() << L"\"";
			out << L" val" << val_arg_role << "=\"" << getXMLSafeRole(_proposition->getArg(t)->getRoleSym()) << L"\"";
			out << L" val" << val_arg_id << "=\"NON_MENT_ARG\"";
			out << L" />\n";
		}
	}*/
}

void PropPFeature::setCoverage(const PatternMatcher_ptr patternMatcher) {
	setCoverage(patternMatcher->getDocTheory());
}

void PropPFeature::setCoverage(const DocTheory * docTheory) {
	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);
	const SynNode *node = _proposition->getPredHead();
	if (node == 0) {
		// this will happen with name predicates, and this isn't quite right,
		// but it'll have to do for now (EMB, 12/2009)
		node = sentenceTheory->getPrimaryParse()->getRoot();
	}

	while (node->getParent() != 0 && node->getParent()->getHead() == node)
		node = node->getParent();

	_start_token = node->getStartToken();
	_end_token = node->getEndToken();

	for (int a = 0; a < _proposition->getNArgs(); a++) {
		Argument *arg = _proposition->getArg(a);
		if (arg->getType() == Argument::MENTION_ARG) {
			const Mention *ment = arg->getMention(sentenceTheory->getMentionSet());
			_start_token = std::min(_start_token, ment->getNode()->getStartToken());
			_end_token = std::max(_end_token, ment->getNode()->getEndToken());
		}
	}

	//_coveringNode = sentenceTheory->getPrimaryParse()->getRoot()->getCoveringNodeFromTokenSpan(_start_token, _end_token);
}

void PropPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);

	if (!idMap->hasId(_proposition)) 
		throw UnexpectedInputException("PropPFeature::saveXML", "Could not find XML ID for proposition");
	elem.setAttribute(X_proposition_id, idMap->getId(_proposition));
}

PropPFeature::PropPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);

	_proposition = dynamic_cast<const Proposition *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_proposition_id)));
}

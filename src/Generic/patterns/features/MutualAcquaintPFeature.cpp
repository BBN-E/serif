// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/patterns/features/MutualAcquaintPFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SentenceTheory.h"

MutualAcquaintPFeature::MutualAcquaintPFeature(Symbol agentSym, const Mention *acquaintanceMention, const Mention *agentMention, const LanguageVariant_ptr& languageVariant) :
	PseudoPatternFeature(languageVariant), _agentSym(agentSym), _acquaintanceMention(acquaintanceMention), _agentMention(agentMention)
	{}
	
MutualAcquaintPFeature::MutualAcquaintPFeature(Symbol agentSym, const Mention *acquaintanceMention, const Mention *agentMention) :
	PseudoPatternFeature(), _agentSym(agentSym), _acquaintanceMention(acquaintanceMention), _agentMention(agentMention)
	{}	

void MutualAcquaintPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
	
	const DocTheory* docTheory = patternMatcher->getDocTheory();
	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_acquaintanceMention->getSentenceNumber());

	out << L"    <focus type=\"acquaintance_mention\" ";
    printOffsetsForSpan(patternMatcher, _acquaintanceMention->getSentenceNumber(), _acquaintanceMention->getNode()->getStartToken(), _acquaintanceMention->getNode()->getEndToken(), out);
	out << L" val" << val_arg_id << "=\"" << _acquaintanceMention->getUID() << L"\"";
	out << L" val" << val_arg_canonical_ref << "=\"" << getXMLPrintableString(getBestNameForMention(_acquaintanceMention, docTheory)) << L"\"";
	out << L" val" << val_extra << "=\"" << getXMLPrintableString(std::wstring(_agentSym.to_string())) << L"\"";
	out << L" />\n";

	// this is mostly for debugging purposes :)
	out << L"    <focus type=\"agent_mention\" ";
    printOffsetsForSpan(patternMatcher, _agentMention->getSentenceNumber(), _agentMention->getNode()->getStartToken(), _agentMention->getNode()->getEndToken(), out);
	out << L" val" << val_arg_id << "=\"" << _agentMention->getUID() << L"\"";
	out << L" val" << val_arg_canonical_ref << "=\"" << getXMLPrintableString(getBestNameForMention(_agentMention, docTheory)) << L"\"";
	out << L" />\n";

}

bool MutualAcquaintPFeature::equals(PatternFeature_ptr other) {
	boost::shared_ptr<MutualAcquaintPFeature> f =
		boost::dynamic_pointer_cast<MutualAcquaintPFeature>(other);
	return PatternFeature::simpleEquals(other) && f && 
		f->getAcquaintanceMention() == getAcquaintanceMention() &&
		f->getAgentMention() == getAgentMention() &&
		f->getAgentSym() == getAgentSym();
}

void MutualAcquaintPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PseudoPatternFeature::saveXML(elem, idMap);

	if (!_agentSym.is_null())
		elem.setAttribute(X_agent_sym, _agentSym);

	if (!idMap->hasId(_acquaintanceMention))
		throw UnexpectedInputException("MutualAcquaintPFeature::saveXML", "Could not find XML ID for aquaintance Mention");
	elem.setAttribute(X_aquaintance_mention_id, idMap->getId(_acquaintanceMention));

	if (!idMap->hasId(_agentMention))
		throw UnexpectedInputException("MutualAcquaintPFeature::saveXML", "Could not find XML ID for agent Mention");
	elem.setAttribute(X_agent_mention_id, idMap->getId(_agentMention));
}

MutualAcquaintPFeature::MutualAcquaintPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PseudoPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_agent_sym))
		_agentSym = elem.getAttribute<Symbol>(X_agent_sym);
	else 
		_agentSym = Symbol();

	_acquaintanceMention = dynamic_cast<const Mention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_aquaintance_mention_id)));
	_agentMention = dynamic_cast<const Mention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_agent_mention_id)));
}

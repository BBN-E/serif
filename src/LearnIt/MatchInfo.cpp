#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/OStringStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include <Generic/common/UnicodeUtil.h> 
#include "Generic/state/StateSaver.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/ValueMentionPattern.h"
#include "Generic/patterns/ValueMentionPattern.h"
#include "Generic/patterns/PatternSet.h"
#include "LearnIt/MatchInfo.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/Seed.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/SlotConstraints.h"
#include "LearnIt/SlotPairConstraints.h"
#include "LearnIt/LearnItPattern.h"
#include "boost/foreach.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/lexical_cast.hpp"
#include <cassert>
#include <vector>
#include <sstream>

using boost::dynamic_pointer_cast;
using std::vector;
using std::wstring;
using std::wstringstream;

void MatchInfo::sentenceTheoryInfo(learnit::SentenceTheory* sentTheoryOut, const DocTheory* doc,
	const SentenceTheory *sentTheory, int sent_index, bool include_state) 
{
	//std::wostringstream out;
	TokenSequence *tokSeq = sentTheory->getTokenSequence();
	//out << L"  <SentenceTheory docid=\"" << sentTheory->getDocID() << L"\" "
	sentTheoryOut->set_docid(UnicodeUtil::toUTF8StdString(sentTheory->getDocID().to_string()));
	//	<< L"sent_index=\"" << sent_index << L"\">\n";
	sentTheoryOut->set_sentindex(sent_index);
	//out << L"    <TokenSequence>" 
	//	<< MainUtilities::encodeForXML(tokSeq->toString(0, tokSeq->getNTokens()-1))
	//	<< L"</TokenSequence>\n";
	for (int i=0;i<tokSeq->getNTokens();++i) {
		sentTheoryOut->add_token(UnicodeUtil::toUTF8StdString(tokSeq->getToken(i)->getSymbol().to_string()));
	}

	eventsToXML(sentTheoryOut, doc, sent_index);
	namesToXML(sentTheoryOut, doc, sent_index);

	/*if (include_state) {
		std::wstring state_string;
		OStringStream resultStream(state_string);
		StateSaver stateSaver(resultStream);

		// You'd think we could just call _sent_theory->saveState() here; but that just
		// saves the sentences' subtheories as pointers.  We want them to be encoded
		// like they are in DocTheory::saveState.  So we have to do it ourselves.
		// (Really I should just refactor the serif code.)
		stateSaver.beginList(L"SentenceTheory");
		stateSaver.saveSymbol(sentTheory->getPrimaryParseSym());
		stateSaver.saveSymbol(sentTheory->getDocID());

		// subtheories
		stateSaver.beginList(L"SentenceTheory::subtheories");
		int n_subtheories = 0;
		for (int j = 0; j < SentenceTheory::N_SUBTHEORY_TYPES; j++) { 
			if (sentTheory->getSubtheory(j) != 0) 
				n_subtheories++;
		}
		stateSaver.saveInteger(n_subtheories);
		for (int k = 0; k < SentenceTheory::N_SUBTHEORY_TYPES; k++) { 
			if (sentTheory->getSubtheory(k) != 0) {
				stateSaver.saveInteger(sentTheory->getSubtheory(k)->getSubtheoryType());
				sentTheory->getSubtheory(k)->saveState(&stateSaver);
			}
		}
		stateSaver.endList(); // end list of subtheories
		stateSaver.endList(); // end sentence theory
		out << L"    <SentenceState>" << MainUtilities::encodeForXML(state_string)
			<< L"</SentenceState>\n";
	}
	out << L"  </SentenceTheory>\n\n";*/
	//return out.str();
}

void MatchInfo::eventsToXML(learnit::SentenceTheory* sentTheoryOut, const DocTheory* dt, int sent_index) {
	std::set<std::wstring> docEvents, sentEvents;

	const EventSet* events = dt->getEventSet();

	for (int i=0; i<events->getNEvents(); ++i) {
		const Event* event=events->getEvent(i);
	
		Symbol eventSym=event->getType();

		if (eventSym.is_null()) {
			continue;
		}

		docEvents.insert(eventSym.to_string());
		Event::LinkedEventMention* ev_linked_ment = event->getEventMentions();

		while (ev_linked_ment) {
			const EventMention* ev_ment=ev_linked_ment->eventMention;
			if (ev_ment) {
				if (ev_ment->getSentenceNumber() == sent_index)
				{
					sentEvents.insert(eventSym.to_string());
				}
			}
			ev_linked_ment=ev_linked_ment->next;
		}
	}

	//std::wostringstream out;
	//out << L"\t<docEvents>";
	BOOST_FOREACH(const std::wstring& s, docEvents) {
		//out << s << L" ";
		sentTheoryOut->add_docevent(UnicodeUtil::toUTF8StdString(s));
	}
	//out << L"</docEvents>\n\t<sentEvents>";
	BOOST_FOREACH(const std::wstring& s, sentEvents) {
		//out << s << L" ";
		sentTheoryOut->add_sentevent(UnicodeUtil::toUTF8StdString(s));
	}
	//out << L"</sentEvents>\n";

	//return out.str();
}

void MatchInfo::setNameSpanForMention(const Mention* m, const DocTheory* doc, learnit::NameSpan* nameSpan) {
	nameSpan->set_entitytype(UnicodeUtil::toUTF8StdString(m->getEntityType().getName().to_string()));
	nameSpan->set_entitysubtype(UnicodeUtil::toUTF8StdString(m->guessEntitySubtype(doc).getName().to_string()));
	const Entity* e = doc->getEntityByMention(m);
	if (e) {
		for (int i=0; i < e->getNMentions(); i++) {
			const Mention* subm = doc->getMention(e->getMention(i));
			if (subm->getMentionType() == Mention::DESC) {
				TokenSequence* seq = doc->getSentenceTheory(subm->getSentenceNumber())->getTokenSequence();

				nameSpan->add_allmentions(UnicodeUtil::toUTF8StdString(
					subm->getAtomicHead()->toCasedTextString(seq)));
			}
		}
	}
	nameSpan->set_start(m->getNode()->getStartToken());
	nameSpan->set_end(m->getNode()->getEndToken());
}

void MatchInfo::namesToXML(learnit::SentenceTheory* sentTheoryOut, const DocTheory* doc, int sent_index) {
	const MentionSet* ms = doc->getSentenceTheory(sent_index)->getMentionSet();
	//std::wstringstream output;
	
	//output << L"\t<nameSpans>\n";
	if (ms) {
		for (int i=0; i<ms->getNMentions(); ++i) {
			const Mention* m=ms->getMention(i);

			/*output << L"\t\t<span mentionType=\"" << m->getTypeString(m->getMentionType()) << L"\" ";
			output << L"entityType=\"" << m->getEntityType().getName().to_string() << L"\" ";
			//guess subtype for now
			output << L"entitySubtype=\"" << m->guessEntitySubtype(doc).getName().to_string() << L"\" ";
			output << L"start=\"" << m->getNode()->getStartToken() << L"\" ";
			output << L"end=\"" << m->getNode()->getEndToken() << L"\" />\n";*/
			learnit::NameSpan* nameSpan = sentTheoryOut->add_namespan();
			nameSpan->set_mentiontype(Mention::getTypeString(m->getMentionType()));
			setNameSpanForMention(m,doc,nameSpan);

			//if (m && m->getMentionType() == Mention::NAME) {
			//	output << m->getNode()->getStartToken() << L" " <<
			//		m->getNode()->getEndToken() << L" ";
			//}
		}
	}
	//output << L"\t</nameSpans>\n";

	//return output.str();
}

void MatchInfo::printRelEvRoles(const vector<RelEvMatch>& matches,
		const wstring& attributeName, unsigned int slot, learnit::SlotFiller* out)
{
	if (!matches.empty()) {
		//out << attributeName << L"=\"";
		
		//bool first = true;
		BOOST_FOREACH(const RelEvMatch& match, matches) {
			/*if (!first) {
				out << L",";
			} else {
				first = false;
			}*/

			if (slot < match.roles.size()) {
				//out << match.roles[slot];
				learnit::Role* role = out->add_role();
				role->set_type(UnicodeUtil::toUTF8StdString(attributeName));
				role->set_value(UnicodeUtil::toUTF8StdString(match.roles[slot]));
			} else {
				wstringstream err;
				err << L"RelEvMatch has only " << match.roles.size() << L" roles, so "
					<< L"cannot get requested slot " << slot;
				throw UnexpectedInputException("InstanceFinder::printRelEvRoles", err);
			}
		}
		//out << L"\" ";
	}
}

void MatchInfo::slotFillerInfo(learnit::SlotFiller* outSlotFiller, const SentenceTheory *sentTheory,
		Target_ptr target, SlotFiller_ptr slot_filler, unsigned int slot_num, std::wstring seedString)
{
	std::vector<RelEvMatch> relationMatches;
	std::vector<RelEvMatch> eventMatches;

	return slotFillerInfo(outSlotFiller, sentTheory, target, slot_filler, slot_num, seedString,
		relationMatches, eventMatches);
}

void MatchInfo::slotFillerInfo(learnit::SlotFiller* outSlotFiller, const SentenceTheory *sentTheory, 
   Target_ptr target, SlotFiller_ptr slot_filler, unsigned int slot_num, 
   std::wstring seedString, const vector<RelEvMatch>& relationMatches, const vector<RelEvMatch>& eventMatches) {

	//std::wostringstream out;
	//double entity_text_score = 0;
	/*std::cout << L"    <SlotFiller "
			  << L"slot=\"" << slot_num << L"\" "
			  << L"type=\"" << ((slot_filler->getType()==SlotConstraints::MENTION)?"MENTION":"VALUE_MENTION") << L"\" "
			  << L"start_token=\"" << slot_filler->getStartToken() << L"\" "
			  << L"end_token=\"" << slot_filler->getEndToken() << L"\" "
			  << L"head_start_token=\"" << slot_filler->getHeadStartToken(target, slot_num) << L"\" "
			  << L"head_end_token=\"" << slot_filler->getHeadEndToken(target, slot_num) << L"\" "
			  << L"mention_type=\"" << Mention::getTypeString(slot_filler->getMentionType()) << L"\" ";*/
	
	outSlotFiller->set_slot(slot_num);
	outSlotFiller->set_type(((slot_filler->getType()==SlotConstraints::MENTION)?"MENTION":"VALUE_MENTION"));
	outSlotFiller->set_starttoken(slot_filler->getStartToken());
	outSlotFiller->set_endtoken(slot_filler->getEndToken());
	outSlotFiller->set_headstarttoken(slot_filler->getHeadStartToken(target, slot_num));
	outSlotFiller->set_headendtoken(slot_filler->getHeadEndToken(target, slot_num));
	outSlotFiller->set_mentiontype(Mention::getTypeString(slot_filler->getMentionType()));
	outSlotFiller->set_language(UnicodeUtil::toUTF8StdString(slot_filler->getLanguageVariant()->getLanguageString()));
	
	//SessionLogger::info("LEARNIT") << L"Seed found in " << slot_filler->getLanguageVariant()->getLanguageString() << L"\n";

		/*<< L"mention_pointer=\"@" << slot_filler->getSerializedObjectID() << L"\" "
		<< L"entity_pointer=\"@" << slot_filler->getSerializedObjectIDForEntity() << L"\" "
		;*/
	if (!relationMatches.empty()) {
		wstring att = L"relationRoles";
		printRelEvRoles(relationMatches, att, slot_num, outSlotFiller);
	}
	if (!eventMatches.empty()) {
		printRelEvRoles(eventMatches, L"eventRoles", slot_num, outSlotFiller);
	}
	/*
	out	<< L">\n"
		<< L"      <Text>" << MainUtilities::encodeForXML(slot_filler->getText()) << L"</Text>\n"
		<< L"      <BestName confidence=\"" << slot_filler->getBestNameConfidence() 
			<< L"\" mention_type=\"" << Mention::getTypeString(slot_filler->getBestNameMentionType()) << L"\">"
		<< MainUtilities::encodeForXML(slot_filler->getBestName()) << L"</BestName>\n";*/

	outSlotFiller->set_text(UnicodeUtil::toUTF8StdString(slot_filler->getText()));
	learnit::BestName* bestName = outSlotFiller->mutable_bestname();
	bestName->set_confidence(slot_filler->getBestNameConfidence());
	bestName->set_mentiontype(Mention::getTypeString(slot_filler->getBestNameMentionType()));
	bestName->set_text(UnicodeUtil::toUTF8StdString(slot_filler->getBestName()));

	if (!seedString.empty()) {
		//out << L"      <SeedString>" << MainUtilities::encodeForXML(seedString) << "</SeedString>\n";
		outSlotFiller->set_seedstring(UnicodeUtil::toUTF8StdString(seedString));
	}
	//out << L"    </SlotFiller>\n";
	//return out.str();
}

/** Given a proposition and a slot filler, return a list of all paths that connect
  * that proposition to that slot filler.  A path is a list of <proposition, arg-role>
  * pairs.  The returned paths will all have 'prop' as the proposition for the
  * first element of the path. */
void MatchInfo::findPropPaths(const SentenceTheory *sentTheory, const Proposition *prop, 
							  SlotFiller_ptr slotFiller, const PropPath &pathToHere,
							  std::vector<PropPath> &result)
{
	// Check for cycles: infinite loops are not a good thing.
	BOOST_FOREACH(PropPathLink link, pathToHere) {
		if (link.prop == prop) {
			SessionLogger::info("LEARNIT") << "Cycle detected: ";
			BOOST_FOREACH(PropPathLink l, pathToHere) {
				SessionLogger::info("LEARNIT") << l.prop->getPredHead()->toTextString()
					<< L" " << l.role << L" -> ";
			}
			SessionLogger::info("LEARNIT") << prop->getPredHead()->toTextString() << std::endl;
			return;
		}
	}

	// If the proposition has an appropriate type, then (recursively) 
	// check all its arguments to see if they contain the slot filler.
	Proposition::PredType propType = prop->getPredType();
	if ((propType == Proposition::NOUN_PRED) || (propType == Proposition::COPULA_PRED) ||
		(propType == Proposition::VERB_PRED) || (propType == Proposition::MODIFIER_PRED)) {
		for (int a = 0; a < prop->getNArgs(); a++) {
			Argument *arg = prop->getArg(a);
			PropPath pathFromHere(pathToHere);
			pathFromHere.push_back(PropPathLink(prop, arg->getRoleSym().to_string(), false));
			findPropPathsViaArg(sentTheory, prop, arg, slotFiller, pathFromHere, result);
		}
	}
	// If the argument contains a 'SET' or 'COMP' proposition, then recurse to all of
	// of its children, but do *not* add a link to the path.  This has the effect of
	// transparently bypassing SET and COMP predicates.
	if ((propType == Proposition::SET_PRED) || (propType == Proposition::COMP_PRED))
	{
		for (int a = 0; a < prop->getNArgs(); a++) {
			Argument *arg = prop->getArg(a);
			findPropPathsViaArg(sentTheory, prop, arg, slotFiller, pathToHere, result);
		}
	}
}

/** Helper function for findPropPaths: Return a list of all paths that connect a 
  * proposition to a given slot filler, via a specified argument. */
void MatchInfo::findPropPathsViaArg(const SentenceTheory *sentTheory, 
									const Proposition *prop, Argument *arg, 
									SlotFiller_ptr slotFiller, const PropPath &pathToHere,
									std::vector<PropPath> &result)
{
	assert(arg->getRoleSym().to_string() != NULL);

	// Check if the argument is the slot-filler we're looking for.  If so, then
	// return a single empty path -- the caller will add the prop/arg pair to it.
	// Return immediately (don't look inside the arg), because we don't care 
	// about slot fillers that occur inside themselves.
	if (arg->getType() == Argument::MENTION_ARG) {
		if(slotFiller->matchesArgument(arg)){
			PropPath path(pathToHere);
			path.push_back(PropPathLink(prop, L"", true));
			result.push_back(path);
			return;
		}
	}

	// Otherwise, check if the argument contains a proposition.  This can happen
	// either (1) if the arg is a proposition itself; or (2) if the arg is a 
	// mention that corresponds to a proposition.
	Proposition const *argProp = NULL;
	if (arg->getType() == Argument::PROPOSITION_ARG) {
		argProp = arg->getProposition();
	} else if (arg->getType() == Argument::MENTION_ARG) {
		argProp = sentTheory->getPropositionSet()->getDefinition(arg->getMentionIndex());
	}

	// If the argument does contain a proposition (and it's not a self-loop), 
	// then recurse to find any paths through that argument's arguments.
	if (argProp && (argProp != prop)) {
		findPropPaths(sentTheory, argProp, slotFiller, pathToHere, result);
	}
}

/** The only slot constrain enforced right now is that slots which say
  * that they must corefer in fact do */
bool MatchInfo::_satisfiesSlotPairConstraints(
	const std::vector<SlotFiller_ptr>& slot_fillers,
	const std::vector<SlotPairConstraints_ptr>& constraints)
{
	typedef std::pair<int, int> IntPair;

	BOOST_FOREACH(const SlotPairConstraints_ptr& constraint, constraints) {
		if (!constraint->mustCorefer() && !constraint->mustNotCorefer()) continue;
		if (std::max(constraint->first(), constraint->second()) 
			> slot_fillers.size()) 
		{
			throw UnexpectedInputException("MatchInfo::_satisfiesSlotPairConstraints",
                                "Invalid slot number in constraint");
		}
        
		const Entity* e1=slot_fillers[constraint->first()]->getMentionEntity();
		const Entity* e2=slot_fillers[constraint->second()]->getMentionEntity();
		
		if (constraint->mustCorefer() && !(e1 && e2 && e1==e2)) {
			return false;
		}
		else if (constraint->mustNotCorefer() && (e1 && e2 && e1==e2)) {
			return false;
		}
	}

	return true;
}

// ======================================================================
// The following functions were pulled from PatternMatchFinder to support recall analysis

/** Search for any matches of a given pattern in a given sentence.  Return a
  * vector of PatternMatch's, where each PatternMatch is a vector of SlotFillers.
  * For RELATION targets, each of these vectors will have two elements (for
  * slotx and sloty).  For CONCEPT targets, each of these vectors will have one
  * element (for slotx).
  */
MatchInfo::PatternMatches MatchInfo::findPatternInSentence(const DocTheory* dt, 
			const SentenceTheory *sent_theory, LearnItPattern_ptr pattern) 
{
	AlignedDocSet_ptr doc_set = boost::shared_ptr<AlignedDocSet>();
	doc_set->loadDocTheory(LanguageVariant::getLanguageVariant(),dt);
	return findPatternInSentence(doc_set,sent_theory,pattern);
}

MatchInfo::PatternMatches MatchInfo::findPatternInSentence(const AlignedDocSet_ptr doc_set, 
			const SentenceTheory *sent_theory, LearnItPattern_ptr pattern) 
{
	PatternMatches result;

	Target_ptr target = pattern->getTarget();

	// Find any places where the pattern matches this sentence.
	int iter = 0;
	BOOST_FOREACH(PatternFeatureSet_ptr sfs, pattern->applyToSentence(doc_set, *sent_theory)) {
		iter++;
		// We found a match; get the slot features.
		std::map<int, ReturnPatternFeature_ptr> slotFeatures;
		int start_token=-1, end_token=-1;
		for (size_t feat_index = 0; feat_index < sfs->getNFeatures(); feat_index++) {
			PatternFeature_ptr sf = sfs->getFeature(feat_index);
			if (ReturnPatternFeature_ptr srf = dynamic_pointer_cast<ReturnPatternFeature>(sf)) {
				std::wstring returnLabel = srf->getReturnLabel().to_string();

				int slotnum = -1;
				assert(returnLabel.substr(0,4) == L"SLOT");
				slotnum = boost::lexical_cast<int>(returnLabel.substr(4));
				slotFeatures[slotnum] = srf;
			}
			else if (TokenSpanPFeature_ptr tokenSpanFeature = dynamic_pointer_cast<TokenSpanPFeature>(sf)) {
/*			else if (sf->getFeatureType() == SnippetFeature::TOKEN_SPAN) {
				SnippetTokenSpanFeature *tokenSpanFeature = dynamic_cast<SnippetTokenSpanFeature*>(sf);*/
				start_token = tokenSpanFeature->getStartToken();
				end_token = tokenSpanFeature->getEndToken();
			}
			else if (PropPFeature_ptr propFeature = dynamic_pointer_cast<PropPFeature>(sf)) {
				const Proposition* prop = propFeature->getProp();
/*			else if (sf->getFeatureType() == SnippetFeature::PROP) {
				const Proposition *prop = dynamic_cast<SnippetPropFeature*>(sf)->getProp();*/\
				SentenceTheory* stheory = doc_set->getDocTheory(propFeature->getLanguageVariant())->getSentenceTheory(sent_theory->getSentNumber());
				start_token = stheory->getTokenSequence()->getNTokens()+1;
				end_token = -1;
				prop->getStartEndTokenProposition(stheory->getMentionSet(), start_token, end_token);
			}
		}
		// Add the match to our results list.
		std::vector<SlotFiller_ptr> slot_fillers;
		slot_fillers.clear();
		for (int slot_num=0; slot_num<target->getNumSlots(); ++slot_num) {
			if (slotFeatures.count(slot_num)) {
			
				SlotFiller_ptr slotFiller = SlotFiller::fromPatternFeature(
					slotFeatures[slot_num], doc_set->getDocTheory(slotFeatures[slot_num]->getLanguageVariant()), 
						target->getSlotConstraints(slot_num));

				if (slotFiller == NULL) {
					// Add a null SlotFiller (indicates some error).
					SessionLogger::info("LEARNIT") << "error, adding null slotfiller" << std::endl;
					slot_fillers.push_back(slotFiller);
				}
				if (slotFiller->satisfiesSlotConstraints(target->getSlotConstraints(slot_num))) {
					slot_fillers.push_back(slotFiller);
				} else {
					// If it doesn't satisfy the constraints, then act like it's not filled in.
					// E.g., if we're looking for a YYMMDD date, and we find "tuesday", then this
					// will leave the SlotFiller empty with unmet constraints.
					//SessionLogger::info("LEARNIT") << "slot " << slot_num << " constraints not met: expected " << DistillUtilities::toUTF8StdString(target->getSlotConstraints(slot_num)->getBrandyConstraints()) << ", got " << DistillUtilities::toUTF8StdString(slotFiller->getBestName()) << std::endl;
					//SessionLogger::info("LEARNIT") << L"Unmet constraints\n";
					//slot_fillers.push_back(boost::make_shared<SlotFiller>(SlotConstraints::UNMET_CONSTRAINTS));
				}
			} else {
				// Add an empty optional SlotFiller.
				//slot_fillers.push_back(boost::make_shared<SlotFiller>(SlotConstraints::UNFILLED_OPTIONAL));
				//SessionLogger::info("LEARNIT") << "optional arg slot_num: " << slot_num << std::endl;
			}
		}
		
		if (slot_fillers.size() < 2) {
			continue;
		}

		if (!_satisfiesSlotPairConstraints(slot_fillers, 
			target->getSlotPairConstraints())) 
		{
			continue;
		}

		result.push_back(PatternMatch(slot_fillers, start_token, end_token, 0.0, pattern->getName()));
	}

	return result;
}

class NoPredHeadException : public std::runtime_error {
public:
	NoPredHeadException();
};

NoPredHeadException::NoPredHeadException() :
std::runtime_error("No PredHead found") {}

std::wstring MatchInfo::getLexValue(const Proposition *prop) {
	const SynNode *node = prop->getPredHead();
	if( node ){
		return node->getHeadWord().to_string();
	}else{
		throw NoPredHeadException();
	}
}

void MatchInfo::lexicalizeCombinations(const SentenceTheory *sentTheory, std::vector<SlotPathMap> &thePaths) {
	typedef std::pair<int, PropPath> SlotPath;
	std::vector<SlotPathMap> pathMapsToAdd;
	BOOST_FOREACH(SlotPathMap currSlotPathMap, thePaths){
		const Proposition *currProp = currSlotPathMap.begin()->second[0].prop;
		std::set<std::wstring> usedArgs;
		BOOST_FOREACH( SlotPath slotPath, currSlotPathMap ) {
			usedArgs.insert(slotPath.second[0].role);
		}
		for( int a = 0; a < currProp->getNArgs(); ++a ){
			Argument *arg = currProp->getArg(a);
			std::wstring currRole = arg->getRoleSym().to_string();
			if( currRole == L"<ref>" ){
				//Skip all references
				continue;
			}
			if( usedArgs.find( currRole ) == usedArgs.end()){
				SlotPathMap newMap(currSlotPathMap);
				PropPath lexPath(1, PropPathLink(currProp, currRole, false));
				//const Proposition *prop = NULL;
				try{
					std::wstring prop = L"";
					if (arg->getType() == Argument::MENTION_ARG){
						const Proposition* tprop = sentTheory->getPropositionSet()->getDefinition(arg->getMentionIndex());
						if( !tprop )
						{
							//TODO: This may be inappropriate if not having a definition *doesn't* mean to skip.
							continue;
						}
						prop = getLexValue(tprop);
					} else if (arg->getType() == Argument::PROPOSITION_ARG){
						const Proposition* tprop = arg->getProposition();
						if( !tprop )
						{
							//TODO: I'm pretty sure this should *definitely* never happen, but check.
							SessionLogger::info("LEARNIT") << "MatchInfo::lexicalizeCombinations(...): Argument of type PROPOSITION returned NULL\n" <<
								"                                        when getProposition() called" << std::endl;
							throw UnrecoverableException("MatchInfo::lexicalizeCombinations", "Bad Proposition!");
						}
						prop = getLexValue(tprop);
					} else {
						//TODO: what do do with TEXT args
						
						//SessionLogger::info("LEARNIT") << "MatchInfo::lexicalizeCombinations(...): Argument of type TEXT was lexicalized\n" <<
						//	"                                        code does not handle this situation." << std::endl;
						prop = arg->getNode()->toCasedTextString(sentTheory->getTokenSequence());
						//throw UnrecoverableException("MatchInfo::lexicalizeCombinations", "TEXT argument found.");
					}
					
					lexPath.push_back(PropPathLink(currProp, prop, true)); 
				} catch(NoPredHeadException){
					continue;
				}
				newMap[-1] = lexPath;
				pathMapsToAdd.push_back(newMap);
			}
		}
	}
	for( size_t i = 0; i < pathMapsToAdd.size(); ++i ){
		thePaths.push_back(pathMapsToAdd[i]);
	}
}

void MatchInfo::propInfo(learnit::SeedInstanceMatch* out, const SentenceTheory *sentTheory, Target_ptr target,
								 std::map<int, SlotFiller_ptr> slotFillers, bool lexicalize) {
	// Fill the propset definitions array.  This will allow us to map from mentions to
	// corresponding propositions.
	PropositionSet *propSet = sentTheory->getPropositionSet();
	propSet->fillDefinitionsArray();

	// For each proposition, find all paths from that proposition to each slot filler.
	// If there's at least one path to each slot filler, then take all combinations
	// of paths to the slot fillers, and add an XML record for each one.
	//std::wstring result;
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		Proposition *prop = propSet->getProposition(p);
		// We only care about the proposition types that can be matched by mprop/vprop/nprop
		// Brandy patterns:
		Proposition::PredType propType = prop->getPredType();
		if ((propType == Proposition::NOUN_PRED) || (propType == Proposition::COPULA_PRED) ||
			(propType == Proposition::VERB_PRED) || (propType == Proposition::MODIFIER_PRED)) 
		{
			if (ParamReader::isParamTrue("no_names_in_predicates")) {
				if (prop->getPredHead()->hasMention() && sentTheory->getMentionSet()->getMention(
					prop->getPredHead()->getMentionIndex())->getMentionType() == Mention::NAME)
				{
						continue;
				}
			}
			propInfo(out, sentTheory, prop, target, slotFillers, lexicalize);
		}
	}
	//return result;
}

// Get information about a specific proposition.
void MatchInfo::propInfo(learnit::SeedInstanceMatch* out, const SentenceTheory *sentTheory, 
								 Proposition *prop, Target_ptr target,
								 std::map<int, SlotFiller_ptr> slotFillers, bool lexicalize) {
	// For each slot, get a vector containing all paths to the corresponding slot filler.
	// If there are no paths to any single slot filler, then give up (return "").
	std::map<int, std::vector<PropPath> > pathsToSlots;
	typedef std::pair<int, SlotFiller_ptr> SlotFillerPair; 
	BOOST_FOREACH(SlotFillerPair slotFillerPair, slotFillers) {
		int slot_num = slotFillerPair.first;
		SlotFiller_ptr slotFiller = slotFillerPair.second;
		findPropPaths(sentTheory, prop, slotFiller, PropPath(), pathsToSlots[slot_num]);
		if (pathsToSlots[slot_num].size() == 0) {
			return;
		}
	}

	// Some typedefs used for finding combinations of paths:
	typedef std::pair<int, std::vector<PropPath> > SlotAndPropPaths;

	// Find all combinations of the paths.  Each combination is encoded as a slot->path
	// map.  Thus, the list of all combinations is a vector of SlotPathMap.  To find
	// all combinations, we begin with an empty SlotPathMap.  Then, for each slot, we
	// form all combinations of that slot with the combinations we've already found.
	std::vector<SlotPathMap> slotPathCombinations;
	slotPathCombinations.push_back(std::map<int, PropPath>());
	BOOST_FOREACH(SlotAndPropPaths slotAndPropPaths, pathsToSlots) {
		int slot_num = slotAndPropPaths.first;
		std::vector<PropPath> pathsToSlot = slotAndPropPaths.second;

		std::vector<SlotPathMap> newCombinations;
		BOOST_FOREACH(SlotPathMap partialSlotPaths, slotPathCombinations) {
			BOOST_FOREACH(PropPath pathToSlot, pathsToSlot) {
				SlotPathMap slotPaths(partialSlotPaths); // Make a copy.
				slotPaths[slot_num] = pathToSlot;
				newCombinations.push_back(slotPaths);
			}
		}
		// Swap in the new value for slotPathCombinations.
		slotPathCombinations.swap(newCombinations);
	}

	//If instructed to lexicalize go through the 
	if( lexicalize ) {
		lexicalizeCombinations(sentTheory, slotPathCombinations);
	}
	
	// Now, generate an XML record for each combination of paths to slot fillers.SE
	// This XML record should merge paths that share links.
	//std::wstringstream out;
	BOOST_FOREACH(SlotPathMap slotPaths, slotPathCombinations) {
		writePropInfoFromSlotPaths(slotPaths, out);
	}
	//return out.str();
}

/* Given a mapping from slot labels to proposition paths, return an XML string describing
 * the proposition tree that follows those paths.  Paths that share links should be 
 * 'collapsed' together.  
 * 
 * Preconditions: slotPaths is non-empty; and all paths in slotPaths must start with the
 * same proposition.
 */
int MatchInfo::writePropInfoFromSlotPaths(const std::map<int, PropPath> &slotPaths, 
										   learnit::SeedInstanceMatch* out, size_t depth) {
	// Check our precondition; and get the root of the proposition tree.
	const Proposition* prop = (*slotPaths.begin()).second[depth].prop;
	typedef std::pair<int, PropPath> SlotAndPath;
	typedef std::pair<const Proposition*, std::wstring> PropAndRole;	
	BOOST_FOREACH(SlotAndPath slotAndPropPath, slotPaths) {
		assert(slotAndPropPath.second[depth].prop == prop);
	}

	// Write an XML tag for the root.
	learnit::Prop* propOut = out->add_prop();
	int propIndex = out->prop_size()-1;
	getPropStartTag(prop,propOut);

	// Divide up the slots, according to which child proposition they
	// appear with.
	std::map<PropAndRole, std::map<int, PropPath> > pathsViaChild;
	std::map<PropAndRole, std::vector<int> > slotsAtChild;
	std::map<PropAndRole, std::wstring> lexAtChild;
	std::set<PropAndRole> children;
	BOOST_FOREACH(SlotAndPath slotAndPath, slotPaths) {
		int slot_num = slotAndPath.first;
		PropPath path = slotAndPath.second;
		if (path.size() > (depth+1)) {
			PropAndRole child = make_pair(path[depth+1].prop, path[depth].role);
			if (path[depth+1].end) {
				if (path[depth+1].role.empty()){
					slotsAtChild[child].push_back(slot_num);
				} else {
					lexAtChild[child] = path[depth+1].role;
				}
			} else {
				pathsViaChild[child][slot_num] = path;
			}
			children.insert(child);
		}
	}

	BOOST_FOREACH(PropAndRole child, children) {
		std::wstring role = child.second;
		//out << std::wstring(depth*4+2, L' '); // indentation
		//out << "<Arg role=\"" 
		//	<< MainUtilities::encodeForXML(child.second) << "\"";
		learnit::Arg* arg = propOut->add_arg();
		arg->set_role(UnicodeUtil::toUTF8StdString(role));

		if (slotsAtChild.count(child)) {
			// For multiple slots, this gives 'slots="6 3 4 "'
			//out << L" slot=\"";
			BOOST_FOREACH(int slot_num, slotsAtChild[child]) {
				//out << slot_num << L" ";
				arg->add_slot(slot_num);
			}
			//out << L"\"";
		}
		if (lexAtChild.count(child)){
			//out << L" lex=\"";
			//out << MainUtilities::encodeForXML(lexAtChild[child]);
			//out << L"\"";
			arg->set_lex(UnicodeUtil::toUTF8StdString(lexAtChild[child]));
		}
		if (pathsViaChild[child].empty()) {
			//out << "/>\n";
		} else {
			//out << ">\n";
			arg->add_propchildid(writePropInfoFromSlotPaths(pathsViaChild[child], out, depth+1));
			//out << std::wstring(depth*4+2, L' '); // indentation
			//out << "</Arg>\n";
		}

	}
	//out << std::wstring(depth*4, L' ') << "</Prop>\n";
	return propIndex;
}

void MatchInfo::getPropStartTag(const Proposition *prop, learnit::Prop *out) 
{	
	std::wstring return_value = L"<Prop";

	// Specify the kind of proposition pattern we should use.
	if (prop->getPredType() == Proposition::NOUN_PRED) {
		//return_value += L" kind=\"nprop\"";
		out->set_kind("nprop");
	} else if (prop->getPredType() == Proposition::COPULA_PRED || prop->getPredType() == Proposition::VERB_PRED) {
		//return_value += L" kind=\"vprop\"";
		out->set_kind("vprop");
	} else if (prop->getPredType() == Proposition::MODIFIER_PRED) {
		//return_value += L" kind=\"mprop\"";
		out->set_kind("mprop");
	} else {
		return;
	}

	// Include the predicate (if there is one)
	if (prop->getPredHead() != 0) {
		//return_value += L" predicate=\"";
		//return_value += MainUtilities::encodeForXML(prop->getPredHead()->getHeadWord().to_string());
		//return_value += L"\"";
		out->set_predicate(UnicodeUtil::toUTF8StdString(prop->getPredHead()->getHeadWord().to_string()));
	}
}

RelEvMatch::RelEvMatch(const std::wstring& type, 
	const std::vector<std::wstring>& roles, const wstring& anchor)
	: type(type), roles(roles), anchor(anchor) {}


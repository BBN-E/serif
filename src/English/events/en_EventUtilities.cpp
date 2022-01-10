// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/wordnet/xx_WordNet.h"
#include "English/events/en_EventUtilities.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/events/EventFinder.h"
#include "Generic/events/patterns/DeprecatedPatternEventFinder.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EventMention.h"
#include "English/common/en_WordConstants.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "English/parse/en_STags.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/TokenSequence.h"
#include "English/descriptors/en_TemporalIdentifier.h"
#include "Generic/events/patterns/SurfaceLevelSentence.h"
#include "Generic/theories/EntitySet.h"

// events
static Symbol ACCUSING_SYM(L"Accusing");
static Symbol ARRESTING_SYM(L"ArrestingSomeone");
static Symbol ARRIVING_SYM(L"ArrivingAtAPlace");
static Symbol ATTACK_SYM(L"AttackOnTangible");
static Symbol COERCING_SYM(L"CoercingAnAgent");
static Symbol DYING_SYM(L"Dying");
static Symbol HIRING_SYM(L"EmployeeHiring");
static Symbol HARMING_SYM(L"HarmingAnAgent");
static Symbol KIDNAPPING_SYM(L"KidnappingSomebody");
static Symbol LEAVING_SYM(L"LeavingAPlace");
static Symbol PROMISE_SYM(L"MakingAPromise");
static Symbol AGREEMENT_SYM(L"MakingAnAgreement");
static Symbol MEOUR_SYM(L"MonetaryExchangeOfUserRights");
static Symbol MURDER_SYM(L"Murder");
static Symbol PAYING_SYM(L"Paying");

static Symbol ATTEMPT_SYM(L"AttemptedMurder");

//pseudo events from the LDC sentiment data
static Symbol NEG_FROM_SYM(L"neg-from");
static Symbol POS_FROM_SYM(L"pos-from");
static Symbol NEG_TOWARDS_SYM(L"neg-towards");
static Symbol POS_TOWARDS_SYM(L"pos-towards");

// event relations
static Symbol performedBySym(L"performedBy");
static Symbol sellerSym(L"seller");
static Symbol payerSym(L"payer");

static Symbol objectActedOnSym(L"objectActedOn");
static Symbol agentHiredSym(L"agentHired");
static Symbol buyerSym(L"buyer");
static Symbol toPossessorSym(L"toPossessor");
static Symbol victimSym(L"victim");
static Symbol recipientOfInfoSym(L"recipientOfInfo");

static Symbol deviceUsedSym(L"deviceUsed");
static Symbol objectPaidForSym(L"objectPaidFor");
static Symbol moneyTransferredSym(L"moneyTransferred");

static Symbol eventOccursAtSym(L"eventOccursAt");
static Symbol eventOccursNearSym(L"eventOccursNear");
static Symbol toLocationSym(L"toLocation");
static Symbol fromLocationSym(L"fromLocation");

static Symbol dateOfEventSym(L"dateOfEvent");
static Symbol durationSym(L"duration");
static Symbol timeWithinSym(L"Time-Within");


static Symbol victimAndPerformedBy(L"victimAndPerformedBy");

// entities
static Symbol transportationDeviceSym(L"TransportationDevice");
static Symbol monetaryValueSym(L"MonetaryValue");
static Symbol weaponSym(L"Weapon");
static Symbol dateSym(L"Date");
static Symbol tqSym(L"Time-Quantity");

// times
static Symbol sym_hour(L"hour");
static Symbol sym_hours(L"hours");
static Symbol sym_minute(L"minute");
static Symbol sym_minutes(L"minutes");
static Symbol sym_morning(L"morning");
static Symbol sym_mornings(L"mornings");
static Symbol sym_afternoon(L"afternoon");
static Symbol sym_afternoons(L"afternoons");
static Symbol sym_night(L"night");
static Symbol sym_nights(L"nights");
static Symbol sym_day(L"day");
static Symbol sym_days(L"days");
static Symbol sym_yesterday(L"yesterday");
static Symbol sym_today(L"today");
static Symbol sym_tomorrow(L"tomorrow");
static Symbol sym_year(L"year");
static Symbol sym_years(L"years");
static Symbol sym_week(L"week");
static Symbol sym_weeks(L"weeks");
static Symbol sym_month(L"month");
static Symbol sym_months(L"months");

// english words
static Symbol deadSym(L"dead");
static Symbol overSym(L"over");
static Symbol forSym(L"for");
static Symbol inSym(L"in");
static Symbol onSym(L"on");
static Symbol atSym(L"at");
static Symbol toSym(L"to");
static Symbol bySym(L"by");
static Symbol intoSym(L"into");
static Symbol nearSym(L"near");
static Symbol outsideSym(L"outside");
static Symbol fromSym(L"from");
static Symbol outofSym(L"out_of");
static Symbol ofSym(L"of");

SymbolHash * EnglishEventUtilities::_nonAssertedIndicators;
static Symbol allegedly_sym(L"allegedly_sym");

Symbol EnglishEventUtilities::getStemmedNounPredicate(const Proposition *prop) {
	return WordNet::getInstance()->stem_noun(prop->getPredSymbol());
}
Symbol EnglishEventUtilities::getStemmedVerbPredicate(const Proposition *prop) {
	return WordNet::getInstance()->stem_verb(prop->getPredSymbol());
}

int EnglishEventUtilities::compareEventMentions(EventMention *mention1, EventMention *mention2) {
	// to delete mention1, return 1; to delete mention 2, return 2
	if (mention1->getAnchorProp() == mention2->getAnchorProp()) {
		if (mention1->getEventType() == DeprecatedPatternEventFinder::BLOCK_SYM) {
			return 2;
		} else if (mention2->getEventType() == DeprecatedPatternEventFinder::BLOCK_SYM) {
			return 1;
		} else if (mention1->getEventType() == MURDER_SYM &&
			(mention2->getEventType() == ATTACK_SYM ||
			mention2->getEventType() == DYING_SYM)) 
		{
			return 2;
		} else if (mention2->getEventType() == MURDER_SYM &&
			(mention1->getEventType() == ATTACK_SYM ||
			mention1->getEventType() == DYING_SYM)) 
		{
			return 1;
		} else if (mention1->getEventType() == ATTACK_SYM &&
				   mention2->getEventType() == HARMING_SYM)
		{
			return 2;
		} else if (mention2->getEventType() == ATTACK_SYM &&
				   mention1->getEventType() == HARMING_SYM)
		{
			return 1;
		}  else if (mention1->getEventType() == MURDER_SYM &&
				   mention2->getEventType() == ATTEMPT_SYM)
		{
			return 1;
		} else if (mention2->getEventType() == MURDER_SYM &&
				   mention1->getEventType() == ATTEMPT_SYM)
		{
			return 2;
		} else if (mention1->getNArgs() < mention2->getNArgs()) {
			return 1;
		} else if (mention1->getNArgs() > mention2->getNArgs()) {
			return 2;
		} else {
			// don't know which to remove... test for which has named entity
			// as slot, perhaps? for now, pick randomly...
			return 2;
		}
	}

	if (ParamReader::isParamTrue("use_correct_answers")) {
		return 0;
	}

	const Proposition *prop1 = mention1->getAnchorProp();
	const Proposition *prop2 = mention2->getAnchorProp();
	if (mention1->getEventType() == mention2->getEventType()) 
	{
		int index1 = -1;
		int index2 = -1;
		if ((prop1->getPredType() == Proposition::NOUN_PRED ||
			 prop1->getPredType() == Proposition::MODIFIER_PRED) &&
			 prop1->getArg(0)->getType() == Argument::MENTION_ARG)
		{
			index1 = prop1->getArg(0)->getMentionIndex();
		}
		if ((prop2->getPredType() == Proposition::NOUN_PRED ||
			 prop2->getPredType() == Proposition::MODIFIER_PRED) &&
			 prop2->getArg(0)->getType() == Argument::MENTION_ARG)
		{
			index2 = prop2->getArg(0)->getMentionIndex();
		}

		for (int i = 0; i < prop1->getNArgs(); i++) {
			Argument *arg = prop1->getArg(i);
			if ((arg->getType() == Argument::PROPOSITION_ARG &&
				 arg->getProposition() == prop2) ||
			    (arg->getType() == Argument::MENTION_ARG &&
				 arg->getMentionIndex() == index2))
				return 2;
		}

		for (int j = 0; j < prop2->getNArgs(); j++) {
			Argument *arg = prop2->getArg(j);
			if ((arg->getType() == Argument::PROPOSITION_ARG &&
				 arg->getProposition() == prop1) ||
			    (arg->getType() == Argument::MENTION_ARG &&
				 arg->getMentionIndex() == index1))
				return 1;
		}

	}

	return 0;
}

void EnglishEventUtilities::postProcessEventMention(EventMention *mention,
											 MentionSet *mentionSet)
{
	for (int i = 0; i < mention->getNArgs(); i++) {
		if (mention->getNthArgRole(i) == victimAndPerformedBy) {
			mention->changeNthRole(i, performedBySym);
			mention->addArgument(victimSym, mention->getNthArgMention(i));
		} else if (mention->getNthArgRole(i) == dateOfEventSym) {
			const SynNode *node = mention->getNthArgMention(i)->getNode();
			if (node->getParent() != 0 && node->getParent()->getTag() == EnglishSTags::PP) {
				Symbol ppSym = node->getParent()->getHeadWord();
				if (ppSym == overSym || ppSym == forSym) {
					mention->changeNthRole(i, durationSym);
				}
			}
		}
	}
	if (mention->getEventType() == ATTEMPT_SYM) {
		mention->setEventType(ATTACK_SYM);
	}

	if (ParamReader::isParamTrue("do_last_ditch_date_finding"))
	{
		runLastDitchDateFinder(mention, mentionSet);
	}
}

/** deprecated -- pre-Values */
void EnglishEventUtilities::runLastDitchDateFinder(EventMention *evMention,
											MentionSet *mentionSet)
{
	if (evMention->getEventType() != MURDER_SYM &&
		evMention->getEventType() != ATTACK_SYM &&
		evMention->getEventType() != ATTEMPT_SYM &&
		evMention->getEventType() != DYING_SYM &&
		evMention->getEventType() != HARMING_SYM &&
		evMention->getEventType() != KIDNAPPING_SYM &&
		evMention->getEventType() != ARRIVING_SYM &&
		evMention->getEventType() != LEAVING_SYM)
	{
		return;
	}

	// if event already has date (or duration), return
	for (int i = 0; i < evMention->getNArgs(); i++) {
		if (evMention->getNthArgRole(i) == dateOfEventSym ||
			evMention->getNthArgRole(i) == durationSym)
		{
			return;
		}
	}

	// now see if if this sentence mentions exactly one date
	Mention *tempMention = 0;
	for (int j = 0; j < mentionSet->getNMentions(); j++) {
		Mention *mention = mentionSet->getMention(j);

		if (TemporalIdentifier::looksLikeTemporal(mention->getNode())) {
			// make sure that this isn't just the head
			// of another date mention
			if (mention->getNode()->getParent() != 0 &&
				mention->getNode()->hasMention() &&
				mention->getNode()->getHead() == mention->getNode())
			{
				continue;
			}

			// so far so good, but make sure this doesn't look more like
			// a duration than a date
			Symbol mentionHead = mention->getNode()->getHeadWord();
			if (mentionHead == sym_hour ||
				mentionHead == sym_hours ||
				mentionHead == sym_minute ||
				mentionHead == sym_minutes ||
				mentionHead == sym_morning ||
				mentionHead == sym_mornings ||
				mentionHead == sym_afternoon ||
				mentionHead == sym_afternoons ||
				mentionHead == sym_night ||
				mentionHead == sym_nights ||
				mentionHead == sym_day ||
				mentionHead == sym_days ||
				mentionHead == sym_yesterday ||
				mentionHead == sym_today ||
				mentionHead == sym_tomorrow ||
				mentionHead == sym_year ||
				mentionHead == sym_years ||
				mentionHead == sym_week ||
				mentionHead == sym_weeks ||
				mentionHead == sym_month ||
				mentionHead == sym_months)
			{
				continue;
			}

			if (tempMention != 0) {
				// we're looking at the second temp mention, so it's not the
				// only one in the sentence -- bail out
				tempMention = 0;
				break;
			}
			else {
				tempMention = mention;
			}
		}
	}

	if (tempMention != 0) {

		int anchor_depth = 0;
		const SynNode *root = evMention->getAnchorNode();
		while (root->getParent() != 0) {
			root = root->getParent();
			anchor_depth++;
		}

		//SessionLogger::info("add_date_0")
		//	<< "In: '" << root->toTextString() << "'\n"
		//	<< "Adding date '" << tempMention->getNode()->toTextString() 
		//	<< "' to event: \n" << evMention->toString() << "\n"
		//	<< "Anchor depth = " << anchor_depth << "\n"
		//	<< "___________________________________________\n\n";

		evMention->addArgument(dateOfEventSym, tempMention);
	}
}

void EnglishEventUtilities::runLastDitchDateFinder(EventMention *evMention,
											const TokenSequence *tokens, 
											ValueMentionSet *valueMentionSet,											
											PropositionSet *props)
{
	// if event already has timex value, return
	for (int i = 0; i < evMention->getNValueArgs(); i++) {
		if (evMention->getNthArgValueMention(i)->isTimexValue())
			return;
	}

	// now see if if this sentence mentions exactly one date
	ValueMention *onlyTimeMention = 0;
	for (int j = 0; j < valueMentionSet->getNValueMentions(); j++) {
		ValueMention *val = valueMentionSet->getValueMention(j);
		if (!val->isTimexValue())
			continue;

		// so far so good, but make sure this doesn't look more like
		// a duration than a date
		bool duration_like = false;
		for (int tok = val->getStartToken(); tok <= val->getEndToken(); tok++) {
			Symbol token = SymbolUtilities::lowercaseSymbol(tokens->getToken(tok)->getSymbol());
			if (token == sym_hour ||
				token == sym_hours ||
				token == sym_minute ||
				token == sym_minutes ||
				token == sym_morning ||
				token == sym_mornings ||
				token == sym_afternoon ||
				token == sym_afternoons ||
				token == sym_night ||
				token == sym_nights ||
				token == sym_day ||
				token == sym_days ||
				token == sym_yesterday ||
				token == sym_today ||
				token == sym_tomorrow ||
				token == sym_year ||
				token == sym_years ||
				token == sym_week ||
				token == sym_weeks ||
				token == sym_month ||
				token == sym_months)
			{
				duration_like = true;
				break;
			}
		}

		if (duration_like)
			continue;

		bool contains_reporting = false;
		for (int p = 0; p < props->getNPropositions(); p++) {
			Proposition *prop = props->getProposition(p);
			for (int a = 0; a < prop->getNArgs(); a++) {
				if (prop->getArg(a)->getRoleSym() == Argument::TEMP_ROLE) {
					Symbol predWord = prop->getPredSymbol();
					if (predWord == Symbol(L"said") ||
						predWord == Symbol(L"told") ||
						predWord == Symbol(L"reported") ||
						predWord == Symbol(L"announced")||
						predWord == Symbol(L"described") ||
						predWord == Symbol(L"declared") ||
						predWord == Symbol(L"informed") ||
						predWord == Symbol(L"stated"))
					{
						contains_reporting = true;
						break;
					}
				}
			}
		}

		if (val->getStartToken() - 1 >= 0) {
			Symbol prevWord = tokens->getToken(val->getStartToken() - 1)->getSymbol();
			if (prevWord == Symbol(L"said") ||
				prevWord == Symbol(L"told") ||
				prevWord == Symbol(L"reported") ||
				prevWord == Symbol(L"announced") ||
				prevWord == Symbol(L"described") ||
				prevWord == Symbol(L"declared") ||
				prevWord == Symbol(L"informed") ||
				prevWord == Symbol(L"stated"))
			{
				contains_reporting = true;
			}
		}
		if (val->getStartToken() - 2 >= 0) {
			Symbol prevWord = tokens->getToken(val->getStartToken() - 2)->getSymbol();
			if (prevWord == Symbol(L"said") ||
				prevWord == Symbol(L"told") ||
				prevWord == Symbol(L"reported") ||
				prevWord == Symbol(L"announced")||
				prevWord == Symbol(L"described") ||
				prevWord == Symbol(L"declared") ||
				prevWord == Symbol(L"informed") ||
				prevWord == Symbol(L"stated"))
			{
				contains_reporting = true;
			}
		}

		if (contains_reporting){
			onlyTimeMention = 0;
			break;
		}

		if (onlyTimeMention != 0) {
			// we're looking at the second time mention, so it's not the
			// only one in the sentence -- bail out
			onlyTimeMention = 0;
			break;
		} else onlyTimeMention = val;
	}

	if (onlyTimeMention != 0) {
		
		int anchor_depth = 0;
		const SynNode *root = evMention->getAnchorNode();
		while (root->getParent() != 0) {
			root = root->getParent();
			anchor_depth++;
		}

		const SynNode *trigger = evMention->getAnchorNode();
		const SynNode *time = root->getCoveringNodeFromTokenSpan(onlyTimeMention->getStartToken(), 
			onlyTimeMention->getEndToken());

		SessionLogger::info("SERIF") << "\nIn: '" << root->toDebugTextString() << "'\n"
			<< "Adding date '" << onlyTimeMention->toDebugString(tokens)
			<< "' to event: \n" << evMention->toDebugString() << "\n";
				
		int distance = -1;
		if (trigger->isAncestorOf(time)) {
			distance = time->getAncestorDistance(trigger);
			SessionLogger::info("SERIF") << "Trigger is ancestor of time, distance " << distance << "\n";
		} else if (time->isAncestorOf(trigger)) {
			distance = trigger->getAncestorDistance(time);
			SessionLogger::info("SERIF") << "Time is (strangely) ancestor of trigger, distance " << distance << "\n";
		} else {
			const SynNode *ancestor = trigger->getParent();
			while (ancestor != 0) {
				if (ancestor->isAncestorOf(time)) {
					SessionLogger::info("SERIF") << "Ancestor = " << ancestor->toDebugTextString() << "\n";
					int trigger_distance = trigger->getAncestorDistance(ancestor);
					SessionLogger::info("SERIF") << "Distance from trigger = " << trigger_distance << "\n";
					int time_distance = time->getAncestorDistance(ancestor);
					SessionLogger::info("SERIF") << "Distance from time = " << time_distance << "\n";
					distance = trigger_distance + time_distance;
					break;
				}
				if (ancestor->getTag() == EnglishSTags::S) {
					distance = -1;
					break;
				}
				ancestor = ancestor->getParent();
			}
		}
		
		if (distance >= 0)		
			evMention->addValueArgument(timeWithinSym, onlyTimeMention, 0);
	}
}



bool EnglishEventUtilities::isInvalidEvent(EventMention *mention) {
	for (int n = 0; n < mention->getNArgs(); n++) {
		if (mention->getNthArgMention(n)->getNode()->getHeadWord() == EnglishWordConstants::THE)
			return true;
	}
	if (mention->getEventType() == DYING_SYM &&
		mention->getAnchorNode()->getHeadWord() == deadSym) 
	{
		const SynNode* parent = mention->getAnchorNode()->getParent();
		if (parent != 0 &&
			parent->getTag() == EnglishSTags::NPA)
		{
			return true;
		}
	}
	return false;
}

void EnglishEventUtilities::fixEventType(EventMention *mention, Symbol correctType) {

	static Symbol subjectSlots[] = {performedBySym,
									buyerSym,
									payerSym,
									Symbol()};
	static Symbol objectSlots[] = {objectActedOnSym,
								   agentHiredSym,
								   sellerSym,
								   toPossessorSym,
								   victimSym,
								   Symbol()};
	static Symbol locationSlots[] = {eventOccursAtSym,
									 eventOccursNearSym,
									 fromLocationSym,
									 toLocationSym,
									 Symbol()};

	SessionLogger::info("chg_event_type_0") << "Changing event type from: "
		<< mention->getEventType().to_string() << "\n" << "                      to: "
		<< correctType.to_string() << "\n";

	mention->setEventType(correctType);

	for (int i = 0; i < mention->getNArgs(); i++) {
		Symbol slotName = mention->getNthArgRole(i);

		if (correctType == DYING_SYM) {
			if (isPerMention(mention->getNthArgMention(i))) {
				if (isOnSlotList(slotName, subjectSlots))
					mention->changeNthRole(i, victimSym);
				else if (isOnSlotList(slotName, objectSlots))
					mention->changeNthRole(i, victimSym);
			}
		} else if (correctType == MEOUR_SYM) {
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, buyerSym);
			else if (isOnSlotList(slotName, objectSlots))
				mention->changeNthRole(i, sellerSym);
			if ((isOnSlotList(slotName, subjectSlots) ||
				 isOnSlotList(slotName, objectSlots)) &&
				 !isAgentMention(mention->getNthArgMention(i)))
			{
				mention->changeNthRole(i, eventOccursAtSym);
			}
		}
		else if (correctType == PAYING_SYM) {
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, payerSym);
			else if (isOnSlotList(slotName, objectSlots))
				mention->changeNthRole(i, toPossessorSym);
			if ((isOnSlotList(slotName, subjectSlots) ||
				 isOnSlotList(slotName, objectSlots)) &&
				 !isAgentMention(mention->getNthArgMention(i)))
			{
				mention->changeNthRole(i, eventOccursAtSym);
			}
		}
		else if (correctType == HIRING_SYM) {
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, performedBySym);
			else if (isOnSlotList(slotName, objectSlots))
				mention->changeNthRole(i, agentHiredSym);
			if ((isOnSlotList(slotName, subjectSlots) ||
				 isOnSlotList(slotName, objectSlots)) &&
				 !isAgentMention(mention->getNthArgMention(i)))
			{
				mention->changeNthRole(i, eventOccursAtSym);
			}
		}
		else if (correctType == HARMING_SYM ||
				 correctType == KIDNAPPING_SYM ||
				 correctType == MURDER_SYM)
		{
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, performedBySym);
			if (isPerMention(mention->getNthArgMention(i))) {
				if (isOnSlotList(slotName, objectSlots))
					mention->changeNthRole(i, victimSym);
			}
			if ((isOnSlotList(slotName, subjectSlots) ||
				 isOnSlotList(slotName, objectSlots)) &&
				 !isAgentMention(mention->getNthArgMention(i)))
			{
				mention->changeNthRole(i, eventOccursAtSym);
			}
		}
		else if (correctType == ATTACK_SYM) {
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, performedBySym);
			else if (isOnSlotList(slotName, objectSlots)) {
				if (mention->getNthArgMention(i)->getEntityType().matchesPER())
					mention->changeNthRole(i, victimSym);
				else
					mention->changeNthRole(i, objectActedOnSym);
			}
			if ((isOnSlotList(slotName, subjectSlots)) &&
				 !isAgentMention(mention->getNthArgMention(i)))
			{
				mention->changeNthRole(i, eventOccursAtSym);
			}
		}
		else if (correctType == ARRIVING_SYM) {
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, performedBySym);
			/*else if (isOnSlotList(slotName, locationSlots))
				mention->changeNthRole(i, toLocationSym);
			if (isOnSlotList(slotName, subjectSlots) &&
				!isAgentMention(mention->getNthSlotMention(i)))
			{
				mention->changeNthRole(i, toLocationSym);
			}*/
		}
		else if (correctType == LEAVING_SYM) {
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, performedBySym);
			/*else if (isOnSlotList(slotName, locationSlots))
				mention->changeNthRole(i, fromLocationSym);
			if (isOnSlotList(slotName, subjectSlots) &&
				!isAgentMention(mention->getNthSlotMention(i)))
			{
				mention->changeNthRole(i, fromLocationSym);
			}*/
		}
		else {
			if (isOnSlotList(slotName, subjectSlots))
				mention->changeNthRole(i, performedBySym);
			else if (isOnSlotList(slotName, objectSlots))
				mention->changeNthRole(i, objectActedOnSym);
			if (isOnSlotList(slotName, subjectSlots) &&
				!isAgentMention(mention->getNthArgMention(i)))
			{
				mention->changeNthRole(i, eventOccursAtSym);
			}
		}

		if (slotName != mention->getNthArgRole(i)) {
			SessionLogger::info("slot_chg_0")
				<< "Changed slot " << slotName.to_string()
				<< " to " << mention->getNthArgRole(i).to_string() << ":\n"
				<< mention->toString() << "\n";
		}
	}
}

bool EnglishEventUtilities::isOnSlotList(Symbol slotName, Symbol *list) {
	if ((*list).is_null())
		return false;
	else
		if (slotName == *list)
			return true;
		else
			return isOnSlotList(slotName, list+1);
}


void EnglishEventUtilities::populateForcedEvent(EventMention *mention, const Proposition *prop, 
										 Symbol correctType, const MentionSet *mentionSet)
{
	mention->setEventType(correctType);
	mention->setAnchor(prop);

	const Mention *subject = 0;
	const Mention *object = 0;
	const Mention *locPrep = 0;
	const Mention *toPrep = 0;
	const Mention *fromPrep = 0;
	const Mention *outofPrep = 0;
	const Mention *nearPrep = 0;
	const Mention *money = 0;
	const Mention *weapon = 0;

	for (int i = 0; i < prop->getNArgs(); i++) {
		Argument *arg = prop->getArg(i);
		if (arg->getType() == Argument::MENTION_ARG &&
			arg->getMention(mentionSet)->getEntityType().isRecognized()) 
		{
			if (arg->getMention(mentionSet)->getEntityType().getName() == monetaryValueSym) {
				money = arg->getMention(mentionSet);
			} else if (arg->getMention(mentionSet)->getEntityType().getName() == weaponSym) {
				weapon = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == Argument::SUB_ROLE) {
				subject = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == Argument::OBJ_ROLE) {
				object = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == toSym ||
					   arg->getRoleSym() == intoSym) 
			{
				toPrep = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == fromSym) 
			{
				fromPrep = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == outofSym) 
			{
				outofPrep = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == nearSym ||
					   arg->getRoleSym() == outsideSym) 
			{
				nearPrep = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == atSym ||
					   arg->getRoleSym() == inSym ||
					   arg->getRoleSym() == onSym ||
					   arg->getRoleSym() == Argument::LOC_ROLE)
			{
				locPrep = arg->getMention(mentionSet);
			} else if (arg->getRoleSym() == Argument::TEMP_ROLE &&
					   arg->getMention(mentionSet)->getEntityType().isTemp()) 
			{
				mention->addArgument(dateOfEventSym, arg->getMention(mentionSet));
			} 
		}
	}

	if (correctType != ARRIVING_SYM &&
		correctType != LEAVING_SYM) 
	{ 
		if (isPlaceMention(locPrep)) {
			mention->addArgument(eventOccursAtSym, locPrep);
		}
		if (isPlaceMention(nearPrep))
			mention->addArgument(eventOccursNearSym, nearPrep);
	} 

	if (correctType == MURDER_SYM ||
		correctType == KIDNAPPING_SYM) 
	{
		if (isOrgOrPerMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isPerMention(object))
			mention->addArgument(victimSym, object);
		if (weapon)
			mention->addArgument(deviceUsedSym, weapon);
	} else if (correctType == ATTACK_SYM) 
	{
		if (isOrgOrPerMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isPerMention(object))
			mention->addArgument(victimSym, object);
		if (isInanimateMention(object))
			mention->addArgument(objectActedOnSym, object);
		if (weapon)
			mention->addArgument(deviceUsedSym, weapon);
	} else if (correctType == ARRIVING_SYM) { 
		if (isOrgOrPerMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isPlaceMention(object))
			mention->addArgument(toLocationSym, object);
		else if (isPlaceMention(locPrep))
			mention->addArgument(toLocationSym, locPrep);
		else if (isPlaceMention(toPrep))
			mention->addArgument(toLocationSym, toPrep);
		if (isPlaceMention(fromPrep))
			mention->addArgument(fromLocationSym, fromPrep);
		else if (isPlaceMention(outofPrep))
			mention->addArgument(fromLocationSym, outofPrep);
	} else if (correctType == LEAVING_SYM) { 
		if (isOrgOrPerMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isPlaceMention(object))
			mention->addArgument(fromLocationSym, object);
		else if (isPlaceMention(locPrep))
			mention->addArgument(fromLocationSym, locPrep);
		else if (isPlaceMention(fromPrep))
			mention->addArgument(fromLocationSym, fromPrep);
		else if (isPlaceMention(outofPrep))
			mention->addArgument(fromLocationSym, outofPrep);
		if (isPlaceMention(toPrep))
			mention->addArgument(toLocationSym, toPrep);
	} else if (correctType == DYING_SYM) { 
		if (isPerMention(subject))
			mention->addArgument(victimSym, subject);
		else if (isPerMention(object))
			mention->addArgument(victimSym, subject);
	} else if (correctType == HARMING_SYM) {
		if (isPerMention(object))
			mention->addArgument(victimSym, object);
		else if (isPerMention(subject))
			mention->addArgument(victimSym, subject);
		if (weapon)
			mention->addArgument(deviceUsedSym, weapon);
	} else if (correctType == COERCING_SYM) { 
		if (isOrgOrPerMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isOrgOrPerMention(object))
			mention->addArgument(objectActedOnSym, object);
	} else if (correctType == ACCUSING_SYM) { 
		if (isAgentMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isOrgOrPerMention(object))
			mention->addArgument(objectActedOnSym, object);
	} else if (correctType == ARRESTING_SYM) { 
		if (isAgentMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isOrgOrPerMention(object))
			mention->addArgument(objectActedOnSym, object);
	} else if (correctType == HIRING_SYM) { 
		if (isAgentMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isOrgOrPerMention(object))
			mention->addArgument(agentHiredSym, object);
	} else if (correctType == MEOUR_SYM) { 
		// from? to?
		if (isAgentMention(subject))
			mention->addArgument(buyerSym, subject);
		else if (isAgentMention(toPrep))
			mention->addArgument(buyerSym, toPrep);
		if (isAgentMention(object))
			mention->addArgument(sellerSym, object);
		else if (isAgentMention(fromPrep))
			mention->addArgument(buyerSym, fromPrep);
		if (money)
			mention->addArgument(moneyTransferredSym, money);
	} else if (correctType == PAYING_SYM) { 
		// from? to?
		if (isAgentMention(subject))
			mention->addArgument(payerSym, subject);
		if (isAgentMention(object))
			mention->addArgument(toPossessorSym, object);
		else if (isOrgOrPerMention(toPrep))
			mention->addArgument(toPossessorSym, toPrep);
		if (money)
			mention->addArgument(moneyTransferredSym, money);
	} else if (correctType == PROMISE_SYM) { 
		if (isAgentMention(subject))
			mention->addArgument(performedBySym, subject);
		if (isAgentMention(object))
			mention->addArgument(recipientOfInfoSym, object);
		else if (isOrgOrPerMention(toPrep))
			mention->addArgument(recipientOfInfoSym, toPrep);
	} else if (correctType == AGREEMENT_SYM) { 
		if (isAgentMention(subject))
			mention->addArgument(performedBySym, subject);
	} 


	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getType() == Argument::MENTION_ARG) {
			const Mention *argMent = arg->getMention(mentionSet);
			if (argMent->getEntityType().isRecognized()) {
				bool found = false;
				for (int k = 0; k < mention->getNArgs(); k++) {
					if (mention->getNthArgMention(k) == argMent) {
						found = true;
						break;
					}
				}
				if (!found) {
					if (EventFinder::DEBUG) {
						EventFinder::_debugStream << L"unattached argument:";
						EventFinder::_debugStream << argMent->getNode()->toTextString();
						EventFinder::_debugStream << L"\n";
					}
					Symbol reltype = getDefaultType(mention->getEventType(),
						arg->getRoleSym(), argMent);
					if (!reltype.is_null()) {
						mention->addArgument(reltype, argMent);
					} else if (isPlaceMention(argMent)) {
						mention->addArgument(eventOccursAtSym, argMent);
					} else mention->addArgument(performedBySym, argMent);

				}
			}
		}
	}
}

bool EnglishEventUtilities::isOrgOrPerMention(const Mention *ment) {
	if (ment == 0) return false;
	return (ment->getEntityType().matchesORG() || ment->getEntityType().matchesPER());
}

bool EnglishEventUtilities::isAgentMention(const Mention *ment) {
	if (ment == 0) return false;
	return (ment->getEntityType().matchesORG() || ment->getEntityType().matchesPER() ||
		ment->getEntityType().matchesGPE());
}

bool EnglishEventUtilities::isPerMention(const Mention *ment) {
	if (ment == 0) return false;
	return ment->getEntityType().matchesPER();
}

bool EnglishEventUtilities::isPlaceMention(const Mention *ment) {
	if (ment == 0) return false;
	return (ment->getEntityType().matchesFAC() || ment->getEntityType().matchesGPE() ||
		ment->getEntityType().matchesLOC() ||
		ment->getEntityType().getName() == transportationDeviceSym);
}
bool EnglishEventUtilities::isInanimateMention(const Mention *ment) {
	if (ment == 0) return false;
	return (ment->getEntityType().matchesFAC() || ment->getEntityType().matchesORG() ||
		ment->getEntityType().matchesLOC() ||
		ment->getEntityType().getName() == transportationDeviceSym);
}

void EnglishEventUtilities::addNearbyEntities(EventMention *mention,
									   SurfaceLevelSentence *sentence, 
									   EntitySet *entitySet) 
{
	int token_index = mention->getAnchorNode()->getStartToken();
	int event_location = -1;

	for (int i = 0; i < sentence->getLength(); i++) {
		if (sentence->getTokenIndex(i) == token_index) {
			event_location = i;
			break;
		}
	}

	if (event_location < 0)
		return;

	// weapons, money...
	for (int j = event_location - 3; j < event_location + 3; j++) {
		addWeapon(mention, sentence, j, entitySet);
		addMoney(mention, sentence, j, entitySet);
	}

	// before the word
	int placeholder = event_location;
	
	if (addDate(mention, sentence, placeholder - 1, entitySet))
		placeholder--;
	if (addAgent(mention, sentence, placeholder - 1, Argument::SUB_ROLE, entitySet))
		placeholder--;
	if (addDate(mention, sentence, placeholder - 1, entitySet))
		placeholder--;
	
	
	// after the word
	placeholder = event_location;
	if (addAgent(mention, sentence, placeholder + 1, Argument::OBJ_ROLE, entitySet))
		placeholder++;

	if (addDate(mention, sentence, placeholder + 1, entitySet))
		placeholder++;
	else if (placeholder + 2 < sentence->getLength() &&
			 !sentence->getWord(placeholder + 1).is_null())
	{
		if (addDate(mention, sentence, placeholder + 2, entitySet))
			placeholder += 2;
	}

	if (addPrep(mention, sentence, placeholder + 1, entitySet)) {
		placeholder += 2;

		if (!addDate(mention, sentence, placeholder + 1, entitySet) &&
			placeholder + 2 < sentence->getLength() &&
			!sentence->getWord(placeholder + 1).is_null())
		{
			addDate(mention, sentence, placeholder + 2, entitySet);
		}
	}

}

bool EnglishEventUtilities::addWeapon(EventMention *mention,
							SurfaceLevelSentence *sentence,
							int index, EntitySet *entitySet) 
{
	if (mention->getFirstMentionForSlot(deviceUsedSym) != 0)
		return false;

	if (mention->getEventType() != MURDER_SYM &&
		mention->getEventType() != ATTACK_SYM)
		return false;

	if (index >= 0 && index < sentence->getLength()) {
		const Mention *nextMention = sentence->getMention(index);
		if (nextMention != 0 && nextMention->getEntityType().getName() == weaponSym) {
			Entity *entity = entitySet->getEntityByMention(nextMention->getUID());
			for (int k = 0; k < mention->getNArgs(); k++) {
				Entity *old_entity = entitySet->getEntityByMention(mention->getNthArgMention(k)->getUID());
				if (old_entity == entity)
					return false;
			}
			mention->addArgument(deviceUsedSym, nextMention);
			if (EventFinder::DEBUG) {
				EventFinder::_debugStream << L"surface argument added:";
				EventFinder::_debugStream << nextMention->getNode()->toTextString();
				EventFinder::_debugStream << L"\n";
			}
			return true;
		}
	}

	return false;
}

bool EnglishEventUtilities::addMoney(EventMention *mention,
							SurfaceLevelSentence *sentence,
							int index, EntitySet *entitySet) 
{
	if (mention->getFirstMentionForSlot(moneyTransferredSym) != 0)
		return false;

	if (mention->getEventType() != MEOUR_SYM &&
		mention->getEventType() != PAYING_SYM)
		return false;

	if (index >= 0 && index < sentence->getLength()) {
		const Mention *nextMention = sentence->getMention(index);
		if (nextMention != 0 && nextMention->getEntityType().getName() == monetaryValueSym) {
			Entity *entity = entitySet->getEntityByMention(nextMention->getUID());
			for (int k = 0; k < mention->getNArgs(); k++) {
				Entity *old_entity = entitySet->getEntityByMention(mention->getNthArgMention(k)->getUID());
				if (old_entity == entity)
					return false;
			}
			mention->addArgument(moneyTransferredSym, nextMention);
			if (EventFinder::DEBUG) {
				EventFinder::_debugStream << L"surface argument added:";
				EventFinder::_debugStream << nextMention->getNode()->toTextString();
				EventFinder::_debugStream << L"\n";
			}
			return true;
		}
	}

	return false;
}


bool EnglishEventUtilities::addDate(EventMention *mention,
							SurfaceLevelSentence *sentence,
							int index, EntitySet *entitySet) 
{
	if (mention->getFirstMentionForSlot(dateOfEventSym) != 0 ||
		mention->getFirstMentionForSlot(durationSym) != 0)
		return false;

	if (index >= 0 && index < sentence->getLength()) {
		const Mention *nextMention = sentence->getMention(index);
		if (nextMention != 0 && nextMention->getEntityType().isTemp()) {
			Entity *entity = entitySet->getEntityByMention(nextMention->getUID());
			for (int k = 0; k < mention->getNArgs(); k++) {
				Entity *old_entity = entitySet->getEntityByMention(mention->getNthArgMention(k)->getUID());
				if (old_entity == entity)
					return false;
			}
			if (nextMention->getEntityType().getName() == dateSym)
				mention->addArgument(dateOfEventSym, nextMention);
			else mention->addArgument(durationSym, nextMention);
			if (EventFinder::DEBUG) {
				EventFinder::_debugStream << L"surface argument added:";
				EventFinder::_debugStream << nextMention->getNode()->toTextString();
				EventFinder::_debugStream << L"\n";
			}
			return true;
		}
	}

	return false;
}

bool EnglishEventUtilities::addAgent(EventMention *mention,
							SurfaceLevelSentence *sentence,
							int index,
							Symbol role, EntitySet *entitySet) 
{
	if (index >= 0 && index < sentence->getLength()) {
		const Mention *nextMention = sentence->getMention(index);
		if (nextMention != 0) {
			Entity *entity = entitySet->getEntityByMention(nextMention->getUID());
			for (int k = 0; k < mention->getNArgs(); k++) {
				Entity *old_entity = entitySet->getEntityByMention(mention->getNthArgMention(k)->getUID());
				if (old_entity == entity)
					return false;
			}
			Symbol reltype = getDefaultType(mention->getEventType(), role, nextMention);
			if (!reltype.is_null() &&
				mention->getFirstMentionForSlot(reltype) == 0) 
			{
				mention->addArgument(reltype, nextMention);	
				if (EventFinder::DEBUG) {
					EventFinder::_debugStream << L"surface argument added:";
					EventFinder::_debugStream << nextMention->getNode()->toTextString();
					EventFinder::_debugStream << L"\n";
				}
				return true;
			}
		}
	}

	return false;
}


bool EnglishEventUtilities::addPrep(EventMention *mention,
							SurfaceLevelSentence *sentence,
							int index, EntitySet *entitySet) 
{
	if (index >= 0 && index + 1 < sentence->getLength()) {
		Symbol next_word = sentence->getWord(index);
		const Mention *nextMention = sentence->getMention(index + 1);
		if (nextMention != 0) {
			Entity *entity = entitySet->getEntityByMention(nextMention->getUID());
			for (int k = 0; k < mention->getNArgs(); k++) {
				Entity *old_entity = entitySet->getEntityByMention(mention->getNthArgMention(k)->getUID());
				if (old_entity == entity)
					return false;
			}
			Symbol reltype = getDefaultType(mention->getEventType(), next_word, nextMention);
			if (!reltype.is_null() && mention->getFirstMentionForSlot(reltype) == 0) {
				mention->addArgument(reltype, nextMention);	
				if (EventFinder::DEBUG) {
					EventFinder::_debugStream << L"surface argument added:";
					EventFinder::_debugStream << nextMention->getNode()->toTextString();
					EventFinder::_debugStream << L"\n";
				}
				return true;
			}
		}
	}

	return false;
}


Symbol EnglishEventUtilities::getDefaultType(Symbol eventType, Symbol role, 
									  const Mention *mention) 
{
	if (mention == 0)
		return Symbol();

	if (role == inSym ||
		role == intoSym) 
	{
		if (isPlaceMention(mention)) {
			if (eventType == ARRIVING_SYM || eventType == LEAVING_SYM)
				return toLocationSym;
			else return eventOccursAtSym;
		}
		if (isInanimateMention(mention)) {
			if (eventType == ATTACK_SYM)
				return objectActedOnSym;
		}
	} if (role == atSym ||
		role == onSym) 
	{
		if (isPlaceMention(mention)) {
			if (eventType == ARRIVING_SYM || eventType == LEAVING_SYM)
				return toLocationSym;
			else return eventOccursAtSym;
		}
		if (isInanimateMention(mention)) {
			if (eventType == ATTACK_SYM)
				return objectActedOnSym;
		}
		if (isPerMention(mention)) {
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM ||
				eventType == DYING_SYM || 
				eventType == HARMING_SYM)
				return victimSym;
		}
	} else if (role == toSym) 
	{
		if (isPlaceMention(mention)) {
			if (eventType == ARRIVING_SYM || eventType == LEAVING_SYM)
				return toLocationSym;
		} 
		
		if (isPerMention(mention)) {
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM ||
				eventType == DYING_SYM || 
				eventType == HARMING_SYM)
				return victimSym;
		}

		if (isAgentMention(mention)) {
			if (eventType == MEOUR_SYM) 
				return buyerSym;
			if (eventType == PAYING_SYM) 
				return toPossessorSym;
			if (eventType == PROMISE_SYM) 
				return recipientOfInfoSym;
		}
	} if (role == fromSym) 
	{
		if (isPlaceMention(mention)) {
			if (eventType == ARRIVING_SYM || eventType == LEAVING_SYM)
				return fromLocationSym;
			else return eventOccursAtSym;
		} 
		
		if (isAgentMention(mention)) {
			if (eventType == MEOUR_SYM) 
				return sellerSym;
			else if (eventType == PAYING_SYM) 
				return payerSym;
		}

	} if (role == outofSym) 
	{
		if (isPlaceMention(mention)) {
			if (eventType == ARRIVING_SYM || eventType == LEAVING_SYM)
				return fromLocationSym;
			else return eventOccursAtSym;
		}

	} else if (role == nearSym ||
			   role == outsideSym) 
	{
		if (isPlaceMention(mention)) 
			return eventOccursNearSym;

	} if (role == ofSym) {
		if (isPerMention(mention)) {
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM ||
				eventType == DYING_SYM || 
				eventType == HARMING_SYM)
				return victimSym;
		} 
		if (isOrgOrPerMention(mention)) {
			if (eventType == ARRIVING_SYM ||
				 eventType == LEAVING_SYM)
				return performedBySym;
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM)
				return performedBySym;
		}

		if (isAgentMention(mention)) {
			if (eventType == AGREEMENT_SYM ||
				eventType == PROMISE_SYM)
				return performedBySym;
			else if (eventType == COERCING_SYM ||
				eventType == ACCUSING_SYM ||
				eventType == ARRESTING_SYM)
				return objectActedOnSym;
			else if (eventType == HIRING_SYM)
				return agentHiredSym;
			else if (eventType == MEOUR_SYM)
				return buyerSym;
			else if (eventType == PAYING_SYM)
				return toPossessorSym;
		} 
		
		if (isInanimateMention(mention)) {
			if (eventType == MEOUR_SYM)
				return objectPaidForSym;
			if (eventType == ATTACK_SYM)
				return objectActedOnSym;
		}

		if (mention->getEntityType().getName() == weaponSym) {
			if (eventType == MEOUR_SYM)
				return objectPaidForSym;
		}
	} if (role == bySym) {
		if (mention->getEntityType().getName() == transportationDeviceSym) {
			if (eventType == ARRIVING_SYM ||
				 eventType == LEAVING_SYM)
				return deviceUsedSym;
		}

		if (isPerMention(mention)) {
			if (eventType == DYING_SYM ||
				eventType == HARMING_SYM)
				return victimSym;
		} 

		if (isOrgOrPerMention(mention)) {
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM)
				return performedBySym;
		} 
		
		if (isAgentMention(mention)) {
			if (eventType == AGREEMENT_SYM ||
				eventType == PROMISE_SYM ||
				eventType == COERCING_SYM ||
				eventType == ACCUSING_SYM ||
				eventType == ARRESTING_SYM ||
				eventType == HIRING_SYM)
				return performedBySym;
			else if (eventType == MEOUR_SYM)
				return buyerSym;
			else if (eventType == PAYING_SYM)
				return toPossessorSym;
		} 
		
	} if (role == Argument::SUB_ROLE) {
		if (isPerMention(mention)) {
			if (eventType == DYING_SYM || eventType == HARMING_SYM)
				return victimSym;
		} 
		
		if (isOrgOrPerMention(mention)) {
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM ||
				eventType == ARRIVING_SYM ||
				eventType == LEAVING_SYM ||
				eventType == COERCING_SYM ||
				eventType == ACCUSING_SYM ||
				eventType == ARRESTING_SYM ||
				eventType == HIRING_SYM ||
				eventType == PROMISE_SYM ||
				eventType == AGREEMENT_SYM) 
				return performedBySym;
		}
		if (isAgentMention(mention)) {
			if (eventType == COERCING_SYM ||
				eventType == ACCUSING_SYM ||
				eventType == ARRESTING_SYM ||
				eventType == HIRING_SYM ||
				eventType == PROMISE_SYM ||
				eventType == AGREEMENT_SYM) 
				return performedBySym;
		}
		if (isInanimateMention(mention)) {
			if (eventType == ATTACK_SYM)
				return objectActedOnSym;
			else if (eventType == MEOUR_SYM)
				return objectPaidForSym;
		}
	} if (role == Argument::OBJ_ROLE) {
		if (isPerMention(mention)) {
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM ||
				eventType == DYING_SYM || 
				eventType == HARMING_SYM)
				return victimSym;
		} 
		if (isOrgOrPerMention(mention)) {
			if (eventType == MURDER_SYM ||
				eventType == ATTACK_SYM ||
				eventType == KIDNAPPING_SYM ||
				eventType == HARMING_SYM)
				return performedBySym;
		} 
		if (isPlaceMention(mention)) {
			if (eventType == ARRIVING_SYM)
				return toLocationSym;
			if (eventType == LEAVING_SYM)
				return fromLocationSym;
		}
		if (isAgentMention(mention)) {
			if (eventType == AGREEMENT_SYM)
				return performedBySym;
			else if (eventType == COERCING_SYM ||
				eventType == ACCUSING_SYM ||
				eventType == ARRESTING_SYM)
				return objectActedOnSym;
			else if (eventType == HIRING_SYM)
				return agentHiredSym;
			else if (eventType == PROMISE_SYM)			
				return recipientOfInfoSym;
			else if (eventType == MEOUR_SYM)
				return sellerSym;
			else if (eventType == PAYING_SYM)
				return toPossessorSym;
		}
		if (isInanimateMention(mention)) {
			if (eventType == MEOUR_SYM)
				return objectPaidForSym;
			if (eventType == ATTACK_SYM)
				return objectActedOnSym;
		}
		if (mention->getEntityType().getName() == weaponSym) {
			if (eventType == MEOUR_SYM)
				return objectPaidForSym;
		}
	} 

	return Symbol();
		
}

bool EnglishEventUtilities::isHelperVerb(SurfaceLevelSentence *sentence, int index) {
	if (index >= 0 && index < sentence->getLength()) {
		Symbol word = sentence->getWord(index);
		if (word.is_null())
			return false;
		if (word == EnglishWordConstants::WAS ||
			word == EnglishWordConstants::IS ||
			word == EnglishWordConstants::ARE ||
			word == EnglishWordConstants::WERE ||
			word == EnglishWordConstants::WILL ||
			word == EnglishWordConstants::WOULD ||
			word == EnglishWordConstants::COULD ||
			word == EnglishWordConstants::SHOULD ||
			word == EnglishWordConstants::MIGHT ||
			word == EnglishWordConstants::HAS ||
			word == EnglishWordConstants::HAD ||
			word == EnglishWordConstants::HAVE)
			return true;
	}
	return false;
}

// This function was initially lifted from RelationUtilities, but has been modified
void EnglishEventUtilities::identifyNonAssertedProps(const PropositionSet *propSet, 
													 const MentionSet *mentionSet, 
													 bool *isNonAsserted)

{

	static bool init = false;
	if (!init) {
		init = true;
		std::string filename = ParamReader::getParam("non_asserted_event_indicators");
		if (!filename.empty()) {
			_nonAssertedIndicators = new SymbolHash(filename.c_str());
		} else _nonAssertedIndicators = new SymbolHash(5);
	}
	
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		isNonAsserted[i] = false;
	}

	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);

		// allegedly
		if (prop->getAdverb() && prop->getAdverb()->getHeadWord() == allegedly_sym)
			isNonAsserted[prop->getIndex()] = true;

		// should, could, might, may
		const SynNode *modal = prop->getModal();
		if (modal != 0) {
			Symbol modalWord = modal->getHeadWord();
			if (modalWord == EnglishWordConstants::SHOULD ||
				modalWord == EnglishWordConstants::COULD ||
				modalWord == EnglishWordConstants::MIGHT ||
				modalWord == EnglishWordConstants::MAY)
				isNonAsserted[prop->getIndex()] = true;
		}

		// if/whether: p#
		// not ALWAYS accurate, but close enough		
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getType() == Argument::PROPOSITION_ARG &&
				(arg->getRoleSym() == EnglishWordConstants::IF || 
				arg->getRoleSym() == EnglishWordConstants::WHETHER))
			{
				isNonAsserted[arg->getProposition()->getIndex()] = true;
			}
		}

		if (prop->getPredType() == Proposition::VERB_PRED) {
			// get "if..." unrepresented in propositions
			const SynNode* node = prop->getPredHead();
			while (node != 0) {
				if (node->getTag() != EnglishSTags::VP &&
					node->getTag() != EnglishSTags::S &&
					node->getTag() != EnglishSTags::SBAR &&
					!node->isPreterminal())
					break;
				if (node->getTag() == EnglishSTags::SBAR &&
					(node->getHeadWord() == EnglishWordConstants::IF ||
					(node->getHeadIndex() == 0 &&
					node->getNChildren() > 1 &&
					node->getChild(1)->getTag() == EnglishSTags::IN &&
					node->getChild(1)->getHeadWord() == EnglishWordConstants::IF)))
				{
					isNonAsserted[prop->getIndex()] = true;
					break;
				}
				node = node->getParent();								
			}
		}
	}

	for (int l = 0; l < propSet->getNPropositions(); l++) {
		Proposition *prop = propSet->getProposition(l);
		
		Symbol predicate = prop->getPredSymbol();
		if (predicate.is_null())
			continue;
		Symbol stemmed_predicate = WordNet::getInstance()->stem_verb(predicate);

		// [non-asserted prop] that: p# -- e.g. he might plan an attack
		// ... but only allow verbs, no modifiers, e.g. he might be happy that...
		// OR
		// [non-asserted word] that: p# -- he guessed that X

		if (_nonAssertedIndicators->lookup(stemmed_predicate) ||
			(isNonAsserted[prop->getIndex()] && prop->getPredType() == Proposition::VERB_PRED))
		{
			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getType() == Argument::PROPOSITION_ARG &&
					(arg->getRoleSym() == Argument::OBJ_ROLE ||
					 arg->getRoleSym() == EnglishWordConstants::THAT ||
					 arg->getRoleSym() == Argument::IOBJ_ROLE))					  
				{
					isNonAsserted[arg->getProposition()->getIndex()] = true;
				}
			}
		}
	}
}

bool EnglishEventUtilities::includeInConnectingString(Symbol tag, Symbol next_tag) {
	/*if (tag == EnglishSTags::DATE_NNP || 
		tag == EnglishSTags::DATE_NNPS ||
		tag == EnglishSTags::RB)
		return false;
	if (tag == EnglishSTags::IN && (next_tag == EnglishSTags::DATE_NNP || next_tag == EnglishSTags::DATE_NNPS))
		return false;*/
	return true;
}

bool EnglishEventUtilities::includeInAbbreviatedConnectingString(Symbol tag, Symbol next_tag) {
	if (!includeInConnectingString(tag, next_tag))
		return false;
	return (LanguageSpecificFunctions::isVerbPOSLabel(tag) ||
		LanguageSpecificFunctions::isPrepPOSLabel(tag));
}

static Symbol beBornSym(L"Life.Be-Born");
static Symbol marrySym(L"Life.Marry");
static Symbol divorceSym(L"Life.Divorce");
static Symbol injureSym(L"Life.Injure");
static Symbol dieSym(L"Life.Die");
static Symbol movementSym(L"Movement.Transport");
static Symbol transferOwnershipSym(L"Transaction.Transfer-Ownership");
static Symbol transferMoneySym(L"Transaction.Transfer-Money");
static Symbol startOrgSym(L"Business.Start-Org");
static Symbol endOrgSym(L"Business.End-Org");
static Symbol mergeOrgSym(L"Business.Merge-Org");
static Symbol bankruptcySym(L"Business.Declare-Bankruptcy");
static Symbol attackSym(L"Conflict.Attack");
static Symbol demonstrateSym(L"Conflict.Demonstrate");
static Symbol meetingSym(L"Contact.Meet");
static Symbol phoneSym(L"Contact.Phone-Write");
static Symbol startPosSym(L"Personnel.Start-Position");
static Symbol endPosSym(L"Personnel.End-Position");
static Symbol nominateSym(L"Personnel.Nominate");
static Symbol electSym(L"Personnel.Elect");
static Symbol acquitSym(L"Justice.Acquit");
static Symbol appealSym(L"Justice.Appeal");
static Symbol arrestSym(L"Justice.Arrest-Jail");
static Symbol chargeSym(L"Justice.Charge-Indict");
static Symbol convictSym(L"Justice.Convict");
static Symbol executeSym(L"Justice.Execute");
static Symbol extraditeSym(L"Justice.Extradite");
static Symbol fineSym(L"Justice.Fine");
static Symbol pardonSym(L"Justice.Pardon");
static Symbol releaseSym(L"Justice.Release-Parole");
static Symbol sentenceSym(L"Justice.Sentence");
static Symbol sueSym(L"Justice.Sue");
static Symbol trialSym(L"Justice.Trial-Hearing");


Symbol EnglishEventUtilities::getReduced2005EventType(Symbol sym) {

	if (sym == beBornSym || sym == marrySym || sym == divorceSym) {
		return Symbol(L"LIFE");
	} else if (sym == injureSym || sym == dieSym) {
		return Symbol(L"INJURE-DIE");
	} else if (sym == startOrgSym || sym == endOrgSym || sym == mergeOrgSym || sym == bankruptcySym) { 
		return Symbol(L"BUSINESS");
	} else if (sym == meetingSym || sym == phoneSym) {
		return Symbol(L"CONTACT");
	} else if (sym == startPosSym || sym == endPosSym || sym == nominateSym || sym == electSym) {
		return Symbol(L"CONTACT");
	} else if (sym == arrestSym || sym == releaseSym || sym == extraditeSym || sym == executeSym) {
		return Symbol(L"CUSTODY");
	} else if (sym == trialSym || sym == chargeSym || sym == convictSym || sym == sentenceSym ||
		sym == acquitSym || sym == appealSym || sym == pardonSym) 
	{
		return Symbol(L"TRIAL");
	} 

	return sym;

}
bool EnglishEventUtilities::isMoneyWord(Symbol sym) {
	return (sym == Symbol(L"money") ||
		sym == Symbol(L"cash") ||
		sym == Symbol(L"taxes") ||
		sym == Symbol(L"funds") ||
		sym == Symbol(L"stipend") ||
		sym == Symbol(L"honorarium") ||
		sym == Symbol(L"allowance") ||
		sym == Symbol(L"alimony") ||
		sym == Symbol(L"commission") ||
		sym == Symbol(L"subsidy") ||
		sym == Symbol(L"pension") ||
		sym == Symbol(L"wages") ||
		sym == Symbol(L"salary") ||
		sym == Symbol(L"$"));
}

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_RESULT_COLLECTION_UTILITIES_H
#define en_RESULT_COLLECTION_UTILITIES_H

#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "English/parse/en_STags.h"
#include "Generic/results/ResultCollectionUtilities.h"

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

// let's try to fix these parses, but for now...
static Symbol sym_reports(L"reports");

static Symbol sym_navy(L"navy");
static Symbol sym_army(L"army");
static Symbol sym_parliament(L"parliament");
static Symbol sym_wall(L"wall"); 
static Symbol sym_street(L"street"); 
static Symbol sym_secret(L"secret");
static Symbol sym_service(L"service");
static Symbol sym_electoral(L"electoral");
static Symbol sym_college(L"college");
static Symbol sym_city(L"city");
static Symbol sym_hall(L"hall");


class EnglishResultCollectionUtilities : public ResultCollectionUtilities {
private:
	friend class EnglishResultCollectionUtilitiesFactory;

public:
	static bool isPREmention(const EntitySet *entitySet, Mention *ment) { 

		if (ment->getMentionType() == Mention::DESC) {
			return NodeInfo::isNominalPremod(ment->getNode());
		}

		if (ment->getMentionType() != Mention::NAME)
			return false;

		if (ment->getChild() != 0)
			return false;

		const SynNode *parent = ment->getNode()->getParent();
		if (parent != 0 && 
			parent->hasMention() &&
			parent->getHead() != ment->getNode())
		{
			const Mention *parentMent = 
				entitySet->getMentionSet(ment->getSentenceNumber())->getMentionByNode(parent);

			// "as (Bob reports) from Jerusalem
			if (parent->getHeadWord() == sym_reports)
				return false;

			// try to make nested dateline stuff NAMEs -- probably not needed w/ above test
			if (parent->getParent() == 0)
				return false;
			
			if (isCityState(entitySet, parentMent))
				return false;
			if (isTime(parentMent))
				return false;
			if (parentMent->getMentionType() != Mention::LIST &&
				parentMent->getMentionType() != Mention::APPO)
			{
				return true;
			}

		}

		return false;
	}

	static bool isTime(const Mention *ment) {
		Symbol headword = ment->getNode()->getHeadWord();
		if (headword == sym_hour ||
			headword == sym_hours ||
			headword == sym_minute ||
			headword == sym_minutes ||
			headword == sym_morning ||
			headword == sym_mornings ||
			headword == sym_afternoon ||
			headword == sym_afternoons ||
			headword == sym_night ||
			headword == sym_nights ||
			headword == sym_day ||
			headword == sym_days ||
			headword == sym_yesterday ||
			headword == sym_today ||
			headword == sym_tomorrow ||
			headword == sym_year ||
			headword == sym_years ||
			headword == sym_week ||
			headword == sym_weeks ||
			headword == sym_month ||
			headword == sym_months)
			return true;
		else return false;
	}

	static bool isCityState(const EntitySet *entitySet, const Mention *ment) {
		const SynNode *node = ment->getNode();
		const MentionSet *mentSet = entitySet->getMentionSet(ment->getSentenceNumber());
		int n_commas = 0;
		const Mention *city = 0;
		const Mention *state = 0;
		for (int i = 0; i < node->getNChildren(); i++) {
			const SynNode *child = node->getChild(i);
			if (child->getTag() == EnglishSTags::CC || child->getTag() == EnglishSTags::CONJP)
				return false;
			else if (child->getTag() == EnglishSTags::COMMA)
				n_commas++;
			else if (child->hasMention()) {
				if (child->getTag() != EnglishSTags::NPP)
					return false;
				if (city == 0)
					city = mentSet->getMentionByNode(child);
				else if (state == 0)
					state = mentSet->getMentionByNode(child);
				else return false;
			}
		}

		if (city == 0 || state == 0 || n_commas == 0)
			return false;

		if (city->getMentionType() == Mention::NONE &&
			city->getEntityType().matchesGPE() &&
			state->getMentionType() == Mention::NAME &&
			state->getEntityType().matchesGPE())
		{
			return true;
		}


		return false;

	}
};

class EnglishResultCollectionUtilitiesFactory: public ResultCollectionUtilities::Factory {
	virtual bool isPREmention(const EntitySet *entitySet, Mention *ment) {  return EnglishResultCollectionUtilities::isPREmention(entitySet, ment); }
};


#endif

#include "common/leak_detection.h"
#include "SeedIRDocConverter.h"

#include "common/ParamReader.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string/erase.hpp>

#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Value.h"
#include "Generic/common/TimexUtils.h"
#include "LearnIt/BestName.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/SlotConstraints.h"

using namespace std;
using namespace boost;

wstring SeedIRDocConverter::ENTITY_LIST=L"<e>"; 
wstring SeedIRDocConverter::_ENTITY_LIST=L"</e>";

// needs to be kept in sync with SlotFiller::matchesSeedSlot ~ RMG
void SeedIRDocConverter::processSentence(const DocTheory* dt,
												 int sn, wostream& out) 
{
	SentenceTheory* st=dt->getSentenceTheory(sn);

	set<wstring> slotFillerStrings;
	getMentionSlotFillerStrings(dt, st, slotFillerStrings);
	getValueSlotFillerStrings(dt, st, slotFillerStrings);

	if (!slotFillerStrings.empty()) {
		LearnItDocConverter::indent(3, out);
		out << ENTITY_LIST;

		BOOST_FOREACH(const wstring& s, slotFillerStrings) {
			out << s << L" ";
		}

		out << _ENTITY_LIST << L"\n";
	}
}

void SeedIRDocConverter::getMentionSlotFillerStrings(const DocTheory* dt,
									  const SentenceTheory* st,
									  set<wstring>& slotFillerStrings) const
{
	const EntitySet* entities=dt->getEntitySet();
	const MentionSet* mentions=st->getMentionSet();
	const int n_mentions=mentions->getNMentions();

	for (int i=0; i<mentions->getNMentions(); ++i) {
		const Mention* ment=mentions->getMention(i);
		slotFillerStrings.insert(
			BestName::calcBestNameForMention(ment, st, dt, false, true).bestName());			

		const Entity* entity=
			entities->getEntityByMentionWithoutType(ment->getUID());
		if (entity && !ParamReader::isParamTrue("no_coref")) {
			for (int j=0; j<entity->getNMentions(); ++j) {
				const Mention* otherMention=
					entities->getMention(entity->getMention(j));
				if (otherMention 
						&& (otherMention->getMentionType()==Mention::NAME)) 
				{
					slotFillerStrings.insert(SlotFiller::corefName(dt, 
											otherMention));
				}
			}
		}
	}	
}

// see SlotFiller::matchesYYMMDDSeedSlot
// if our date is more specific than the one in the seed, we still want to
// match, so we need to generate less specific versions of our dates
void SeedIRDocConverter::addBackedOffDates(const wstring& timexString,
											set<wstring>& slotFillerStrings) const
{
	vector<wstring> dateParts=TimexUtils::parseYYYYMMDD(timexString);
	if (!dateParts.empty()) {
		if (dateParts.size() == 3) {
			const wstring& year=dateParts[0];
			// month and day are optional in the regular expression
			// if they're not present, treat them as unspecified
			const wstring month=(L""==dateParts[1]?L"xx":dateParts[1]);
			const wstring day=(L""==dateParts[2]?L"xx":dateParts[2]);

			// if this doesn't hold, our regexp is broken
			assert(year.length()==2 || year.length()==4);
			assert(month.length()==2);
			assert(day.length()==2);

			vector<wstring> yearVals, monthVals, dayVals;
			
			// xes indicate that part of the data is unspecified...
			yearVals.push_back(L"xxxx");
			yearVals.push_back(L"xx");
			monthVals.push_back(L"xx");
			dayVals.push_back(L"xx");

			if (year.length()==4 && year!=L"xxxx") {
				yearVals.push_back(year);
				// also accept two digit years
				yearVals.push_back(year.substr(2,2));
			} else if (year.length()==2 && year!=L"xx") {
				yearVals.push_back(year);
			}

			if (month!=L"xx") {
				monthVals.push_back(month);
			}

			if (day!=L"xx") {
				dayVals.push_back(day);
			}

			BOOST_FOREACH(const wstring& y, yearVals) {
				BOOST_FOREACH(const wstring& m, monthVals) {
					BOOST_FOREACH(const wstring& d, dayVals) {
						wstring toInsert=y+m+d;
						// require *something* to be specified...
						if (toInsert!=L"xxxxxx" 
							&& toInsert!=L"xxxxxxxx") 
						{
							slotFillerStrings.insert(toInsert);
						}
					}
				}
			}
		} else if (dateParts.size()==2) { //year month
			const wstring& year=dateParts[0];
			// month and day are optional in the regular expression
			// if they're not present, treat them as unspecified
			const wstring month=(L""==dateParts[1]?L"xx":dateParts[1]);

			// if this doesn't hold, our regexp is broken
			assert(year.length()==2 || year.length()==4);
			assert(month.length()==2);

			vector<wstring> yearVals, monthVals;
			
			// xes indicate that part of the data is unspecified...
			yearVals.push_back(L"xxxx");
			yearVals.push_back(L"xx");
			monthVals.push_back(L"xx");

			if (year.length()==4 && year!=L"xxxx") {
				yearVals.push_back(year);
				// also accept two digit years
				yearVals.push_back(year.substr(2,2));
			} else if (year.length()==2 && year!=L"xx") {
				yearVals.push_back(year);
			}

			if (month!=L"xx") {
				monthVals.push_back(month);
			}

			BOOST_FOREACH(const wstring& y, yearVals) {
				BOOST_FOREACH(const wstring& m, monthVals) {
					wstring toInsert=y+m;
					// require *something* to be specified...
					if (toInsert!=L"xxxx" 
						&& toInsert!=L"xxxxxx") 
					{
						slotFillerStrings.insert(toInsert);
					}
				}
			}
		} else if (dateParts.size()==1) { //year
			const wstring& year=dateParts[0];
	
			// if this doesn't hold, our regexp is broken
			assert(year.length()==2 || year.length()==4);

			vector<wstring> yearVals;
			
			// xes indicate that part of the data is unspecified...
			yearVals.push_back(L"xxxx");
			yearVals.push_back(L"xx");

			if (year.length()==4 && year!=L"xxxx") {
				yearVals.push_back(year);
				// also accept two digit years
				yearVals.push_back(year.substr(2,2));
			} else if (year.length()==2 && year!=L"xx") {
				yearVals.push_back(year);
			}

			BOOST_FOREACH(const wstring& y, yearVals) {
				wstring toInsert=y;
				// require *something* to be specified...
				if (toInsert!=L"xx" 
					&& toInsert!=L"xxxx") 
				{
					slotFillerStrings.insert(toInsert);
				}
			}
		}
	}
}

void SeedIRDocConverter::getValueSlotFillerStrings(const DocTheory* dt,
													const SentenceTheory* st,
									  set<wstring>& slotFillerStrings) const
{
	const int sn=st->getSentNumber();
	const ValueMentionSet* vms=st->getValueMentionSet();

	for (int i=0; i<vms->getNValueMentions(); ++i) {
		const ValueMention* vm=vms->getValueMention(i);
		if (vm && sn==vm->getSentenceNumber()) {
			// getting timesString copied from SlotFiller::_findBestName
			// we can't easily refactor to avoid the repetition because
			// that function assumes we know what seed we're searching for~ RMG
			slotFillerStrings.insert(vm->toCasedTextString(st->getTokenSequence()));

			if (vm->isTimexValue()) {
				const Value * docVal = vm->getDocValue();
				const Symbol timexVal = docVal->getTimexVal();
				if (timexVal != Symbol()){
					wstring timexString=timexVal.to_string();
					// we also do looser matching for dates
					// see second conditional in SlotFiller::matchesSeedSlot
					addBackedOffDates(timexString, slotFillerStrings);
					// INDRI tokenizes on -s, so we can't leave them in
					erase_all(timexString, L"-"); 
					slotFillerStrings.insert(timexString);
				}
			}
		} 
	}
}


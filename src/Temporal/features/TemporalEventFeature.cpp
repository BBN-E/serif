#include "Generic/common/leak_detection.h"
#include "TemporalEventFeature.h"
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EntitySet.h"
#include "ActiveLearning/EventUtilities.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"
#include "Temporal/features/TemporalRelationFeature.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"

using std::wstring;
using std::wstringstream;
using boost::dynamic_pointer_cast;

TemporalEventFeature::TemporalEventFeature(const Symbol& eventType, const Symbol& temporalRole,
	const Symbol& relation, const Symbol& anchor)
: TemporalFeature(L"EventTime", relation), _anchor(anchor), _eventType(eventType),
	_temporalRole(temporalRole)
{ }


TemporalEventFeature_ptr TemporalEventFeature::create(
		const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (!(parts.size() == 2 || parts.size() == 3)) {
		throw UnexpectedInputException("TemporalEventFeature::create",
			"Cannot parse TemporalEventFeature");
	}

	Symbol eventType = parts[0];
	Symbol temporalRole = parts[1];
	Symbol anchor = Symbol();

	if (parts.size() == 3) {
		anchor = parts[2];
	}

	return boost::make_shared<TemporalEventFeature>(eventType, temporalRole,
		relation, anchor);
}

std::wstring TemporalEventFeature::pretty() const {
	wstringstream str;

	str << type () << L"(" << _eventType.to_string()
		<< L", " << _temporalRole;

	if (!_anchor.is_null()) {
		str << L", " << _anchor.to_string();
	}

	if (!relation().is_null()) {
		str << L", " << relation().to_string();
	}

	str << L")";

	return str.str();
}

std::wstring TemporalEventFeature::dump() const {
	return pretty();
}

std::wstring TemporalEventFeature::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << _eventType << L"\t" << _temporalRole;

	if (!_anchor.is_null()) {
		str << L"\t" << _anchor.to_string();
	}
	return str.str();
}

bool TemporalEventFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (TemporalEventFeature_ptr tef = 
				dynamic_pointer_cast<TemporalEventFeature>(other)) {
			return tef->_eventType == _eventType
				&& tef->_temporalRole == _temporalRole
				&& tef->_anchor == _anchor;
		}
	}
	return false;
}

size_t TemporalEventFeature::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _eventType.to_string());
	boost::hash_combine(ret, _temporalRole.to_string());

	if (!_anchor.is_null()) {
		boost::hash_combine(ret, _anchor.to_string());
	} else {
		boost::hash_combine(ret, 0);
	}
	return ret;
}
			
bool TemporalEventFeature::matches(TemporalInstance_ptr inst, const DocTheory* dt, 
		unsigned int sn) const 
{
	if (!relationNameMatches(inst)) {
		return false;
	}

	const EventMentionSet* events = dt->getSentenceTheory(sn)->getEventMentionSet();
	const EntitySet* es = dt->getEntitySet();

	for (int i =0; i < events->getNEventMentions(); ++i) {
		const EventMention* event = events->getEventMention(i);

		if (matchesEvent(event, *inst, es)) {
			if (matchesEventFeatureConstraints(event, *inst)) {
				return true;
			}
		}
	}

	return false;
}

bool TemporalEventFeature::matchesEventFeatureConstraints(
		const EventMention* event, const TemporalInstance& inst) const 
{
	Symbol temporalRoleName = getTemporalRoleName(event, inst);
	Symbol anchArg = ALEventUtilities::anchorArg(event);

	return temporalRoleName == _temporalRole && anchArg == _anchor;
}

bool TemporalEventFeature::matchesEvent(const EventMention* event, 
	const TemporalInstance& inst, const EntitySet* es)
{
	if (!getTemporalRoleName(event, inst).is_null()) {
		if ((size_t)event->getNArgs() == inst.relation()->get_args().size()) {
			unsigned int num_matches = 0;
			for (int i=0; i<event->getNArgs(); ++i) {
				const Mention* m = event->getNthArgMention(i);
				BOOST_FOREACH(ElfRelationArg_ptr arg, inst.relation()->get_args()) {
					if (TemporalRelationFeature::relArgMatchesMention(arg, m, es)) {
						++num_matches;
						break;
					}
				}
			}
			return num_matches == (size_t)event->getNArgs();
		}
	}

	return false;
}

Symbol TemporalEventFeature::getTemporalRoleName(const EventMention* event,
	const TemporalInstance& inst) 
{
	for (int i = 0; i < event->getNValueArgs(); ++i) {
		if (event->getNthArgValueMention(i) == inst.attribute()->valueMention()
				&& inst.attribute()->valueMention())
		{
			return event->getNthArgValueRole(i);
		}
	}
	return Symbol();
}


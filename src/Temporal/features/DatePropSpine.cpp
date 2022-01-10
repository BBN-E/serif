#include "Generic/common/leak_detection.h"
#include "Temporal/features/DatePropSpine.h"

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/DocTheory.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"

using std::wstring;
using std::wstringstream;
using std::vector;
using boost::dynamic_pointer_cast;
using boost::make_shared;

DatePropSpineFeature::DatePropSpineFeature(const Spine& spineElements,
		const Symbol& relation)
	: TemporalFeature(L"PropSpine", relation), _spineElements(spineElements)
{ }

DatePropSpineFeature_ptr DatePropSpineFeature::create(
		const std::vector<std::wstring>& parts, const Symbol& relation) 
{
	if (parts.empty()) {
		throw UnexpectedInputException("DatePropSpineFeature::create",
				"No metadata provided.");
	}
	std::vector<Symbol> elements;

	BOOST_FOREACH(const wstring& part, parts) {
		elements.push_back(part);
	}
	return make_shared<DatePropSpineFeature>(elements, relation);
}

std::wstring DatePropSpineFeature::pretty() const {
	wstringstream str;
	str << L"PropSpine(";

	BOOST_FOREACH(const Symbol& element, _spineElements) {
		str << element.to_string() << L" ";
	}

	if (!relation().is_null()) {
		str << L", " << relation().to_string();
	}

	str << L")";

	return str.str();
}

std::wstring DatePropSpineFeature::dump() const {
	return pretty();
}

std::wstring DatePropSpineFeature::metadata() const {
	wstringstream str;

	TemporalFeature::metadata(str);

	BOOST_FOREACH(const Symbol& element, _spineElements) {
		str << L"\t" << element.to_string();
	}

	return str.str();
}

bool DatePropSpineFeature::equals(const TemporalFeature_ptr& other) const {
	if (!topEquals(other)) {
		return false;
	}

	if (DatePropSpineFeature_ptr dpsf =
			dynamic_pointer_cast<DatePropSpineFeature>(other)) {
		return _spineElements == dpsf->_spineElements;
	}
	return false;
}

size_t DatePropSpineFeature::calcHash() const {
	size_t ret = topHash();
	spineHash(ret, _spineElements);	
	return ret;
}


void DatePropSpineFeature::spineHash(size_t& start, 
		const Spine& spineElements)
{
	BOOST_FOREACH(const Symbol& element, spineElements) {
		boost::hash_combine(start, element.to_string());
	}
}


std::vector<DatePropSpineFeature::Spine> DatePropSpineFeature::spinesFromDate(
		const ValueMention* vm, const SentenceTheory* st)
{
	std::vector<Spine> ret;
	// we need to find any mentions which would match this value mention
	// by the algorithm in Generic/patterns/ValueMentionPattern.cpp's ::matchesArgument
	const MentionSet* ms = st->getMentionSet();

	for (int i=0; i<ms->getNMentions(); ++i) {
		const Mention* m = ms->getMention(i);

		if (m->getNode()->getStartToken() == vm->getStartToken() &&
				m->getNode()->getEndToken() == vm->getEndToken())
		{
			spinesFromMention(ret, m, st, false);
		} else if (m->getNode()->getStartToken() <= vm->getStartToken() &&
				m->getNode()->getEndToken() >= vm->getEndToken())
		{
			spinesFromMention(ret, m, st, true);
		}
	}
	return ret;
}

void DatePropSpineFeature::spinesFromMention(
		std::vector<Spine>& ret, const Mention* m,
		const SentenceTheory* st, bool require_temp_role)
{
	const PropositionSet* ps = st->getPropositionSet();
	std::vector<Symbol> path;

	for (int i=0; i<ps->getNPropositions(); ++i) {
		searchForPropMentionPath(ret, path, m, ps->getProposition(i),
				st->getMentionSet(), require_temp_role);
	}
}

void DatePropSpineFeature::searchForPropMentionPath(
		std::vector<Spine>& ret, 
		std::vector<Symbol>& path, const Mention* m, 
		const Proposition* p, const MentionSet * ms, bool require_temp_role) 
{
	if (!containsMentionSpan(p, m, ms)) {
		return;
	}

	if (p->getPredHead()) {
		path.push_back(p->getPredHead()->getHeadWord());
	} else {
		path.push_back(L"<NULL>");
	}

	for (int i=0; i<p->getNArgs(); ++i) {
		const Argument* arg = p->getArg(i);

		if (arg->getType() == Argument::MENTION_ARG && arg->getMention(ms) == m 
				&& (!require_temp_role || arg->getRoleSym() == Argument::TEMP_ROLE))
		{
			ret.push_back(path);
		} else {
			if (arg->getType() == Argument::PROPOSITION_ARG) {
				const Proposition* q = arg->getProposition();
				searchForPropMentionPath(ret, path, m, q, ms, require_temp_role);
			}
		}
	}

	path.pop_back();
}

bool DatePropSpineFeature::containsMentionSpan(const Proposition* p,
		const Mention* m, const MentionSet* ms)
{
	int startTok = 1000000;
	int endTok = -1;
	p->getStartEndTokenProposition(ms, startTok, endTok);

	return startTok <= m->getNode()->getStartToken() 
		&& endTok >= m->getNode()->getEndToken();
}

bool DatePropSpineFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!relationNameMatches(inst)) {
		return false;
	}

	const ValueMention* vm = inst->attribute()->valueMention();

	if (vm) {
		vector<Spine> spines = spinesFromDate(vm, dt->getSentenceTheory(sn));

		BOOST_FOREACH(const Spine& spine, spines) {
			if (_spineElements == spine) {
				return true;
			}
		}
	}

	return false;
}

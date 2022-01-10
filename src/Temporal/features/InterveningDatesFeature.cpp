# pragma warning(disable:4996)

#include "Generic/common/leak_detection.h"
#include "Temporal/features/InterveningDatesFeature.h"

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Offset.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"

using std::wstring;
using std::wstringstream;
using std::vector;
using std::make_pair;
using boost::dynamic_pointer_cast;
using boost::make_shared;
using boost::algorithm::split;
using boost::is_any_of;

const int InterveningDatesFeature::MAX_DATES = 3;

InterveningDatesFeature::InterveningDatesFeature(int n_dates,
		const Symbol& relation)
	: TemporalFeature(L"InterveningDates", relation), _n_dates(n_dates)
{ 
	if (_n_dates < 0) {
		throw UnexpectedInputException("InterveningDatesFeature::InterveningDatesFeature",
				"Cannot have a negative number of dates");
	}

	if (_n_dates > MAX_DATES) {
		SessionLogger::warn("more_than_max_dates") << L"Attempted to create a "
			<< L"InterveningDates feature with more than MAX_DATEs; coercing "
			<< L"down to MAX_DATES";
		_n_dates = MAX_DATES;
	}
}

InterveningDatesFeature_ptr InterveningDatesFeature::create(
		const std::vector<std::wstring>& parts, const Symbol& relation) 
{
	if (parts.empty()) {
		throw UnexpectedInputException("InterveningDatesFeature::create",
				"No metadata provided.");
	}

	if (parts.size() != 1) {
		throw UnexpectedInputException("InterveningDatesFeature::create",
				"Expected exactly one field of metadata");
	}

	int n_dates;

	try {
		n_dates = boost::lexical_cast<int>(parts[0]);
	} catch (boost::bad_lexical_cast&) {
		throw UnexpectedInputException("InterveningDatesFeature::create",
				"Single metadata field must be an integer");
	}

	return make_shared<InterveningDatesFeature>(n_dates, relation);
}

std::wstring InterveningDatesFeature::pretty() const {
	wstringstream str;
	str << L"InterveningDates(";
	
	if (!relation().is_null()) {
		str << relation().to_string() << L", ";
	} 

	str << _n_dates;
	
	if (_n_dates == MAX_DATES) {
		str << L"+";
	}
	str << L")";

	return str.str();
}

std::wstring InterveningDatesFeature::dump() const {
	return pretty();
}

std::wstring InterveningDatesFeature::metadata() const {
	wstringstream str;

	TemporalFeature::metadata(str);
	str << L"\t" << _n_dates;
	return str.str();
}

bool InterveningDatesFeature::equals(const TemporalFeature_ptr& other) const {
	if (!topEquals(other)) {
		return false;
	}

	if (InterveningDatesFeature_ptr idf =
			dynamic_pointer_cast<InterveningDatesFeature>(other)) {
		return _n_dates == idf->_n_dates;
	}
	return false;
}

size_t InterveningDatesFeature::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _n_dates);	
	return ret;
}

bool InterveningDatesFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!relationNameMatches(inst)) {
		return false;
	}
	return _n_dates == numInterveningDates(*inst, dt->getSentenceTheory(sn));
}

void InterveningDatesFeature::addFeaturesToInstance(const TemporalInstance& inst,
		const SentenceTheory* st, std::vector<TemporalFeature_ptr>& fv)
{
	int n_dates = numInterveningDates(inst, st);

	if (n_dates >= 0) {
		fv.push_back(make_shared<InterveningDatesFeature>(n_dates));
		fv.push_back(make_shared<InterveningDatesFeature>(n_dates, 
					inst.relation()->get_name()));
	}
}

std::pair<EDTOffset, EDTOffset> InterveningDatesFeature::constrainedRelationSpan(
		ElfRelation_ptr rel) 
{
	if (rel->get_args().empty()) {
		return make_pair(rel->get_start(), rel->get_end());
	}

	EDTOffset upperBound = rel->get_start();
	EDTOffset lowerBound = rel->get_end();

	BOOST_FOREACH(ElfRelationArg_ptr arg, rel->get_args()) {
		if (arg->get_start() < lowerBound) {
			lowerBound = arg->get_start();
		}
		if (arg->get_end() > upperBound) {
			upperBound = arg->get_end();
		}
	}

	return make_pair(lowerBound, upperBound);
}

int InterveningDatesFeature::numInterveningDates(const TemporalInstance& inst,
		const SentenceTheory* st)
{
	// find the relation bounds
	std::pair<EDTOffset, EDTOffset> relSpan = 
		constrainedRelationSpan(inst.relation());
	EDTOffset relStartOffset = relSpan.first;
	EDTOffset relEndOffset = relSpan.second;
	//SessionLogger::info("foo") << "Interv called";

	if (relStartOffset.is_defined() && relEndOffset.is_defined()) {
	//SessionLogger::info("foo") << "rel offsets defined";
		const TokenSequence* ts = st->getTokenSequence();
		const ValueMention* vm = inst.attribute()->valueMention();
		if (vm) {
			EDTOffset vmStartOffset = 
				ts->getToken(vm->getStartToken())->getStartEDTOffset();
			EDTOffset vmEndOffset = 
				ts->getToken(vm->getEndToken())->getEndEDTOffset();
	//		SessionLogger::info("foo") << "Interv 1";

			// first, check if ValueMention overlaps relation
			if (((relStartOffset <= vmStartOffset) && (relEndOffset >= vmStartOffset))
				|| ((relStartOffset<=vmEndOffset) && (relEndOffset >= vmEndOffset))) 
			{
	/*			SessionLogger::info("foo") << "OVERLAP: Rel(" << relStartOffset
					<< ", " << relEndOffset << "), VM(" << vmStartOffset << ", "
					<< vmEndOffset << ")";*/
				return 0; // overlapping = no intervening dates
			} 
			//SessionLogger::info("foo") << "Interv 2";
		
			EDTOffset lowerBound, upperBound;

			if (vmStartOffset > relEndOffset) {
				// date follows relation
				lowerBound = relEndOffset;
				upperBound = vmStartOffset;
			} else if (vmEndOffset < relStartOffset) {
				// date precedes relation
				lowerBound = vmEndOffset;
				upperBound = relStartOffset;
			} else {
				throw UnexpectedInputException("InterveningDatesFeature::numInterveningDates",
						"Inconsistent token offsets? This should not be possible to reach.");
			}

			const ValueMentionSet* vms = st->getValueMentionSet();
			int num_intervening = 0;

			for (int i=0; i<vms->getNValueMentions(); ++i) {
				const ValueMention* other_vm = vms->getValueMention(i);
			//	SessionLogger::info("foo") << "Interv 3";

				if (vm != other_vm && vm->isTimexValue()) {
					EDTOffset otherVMStartOffset = 
						ts->getToken(other_vm->getStartToken())->getStartEDTOffset();
					EDTOffset otherVMEndOffset = 
						ts->getToken(other_vm->getEndToken())->getEndEDTOffset();

					if (otherVMStartOffset > lowerBound 
							&& otherVMEndOffset < upperBound)
					{
			//		SessionLogger::info("foo") << "Interv 5";
						++num_intervening;
					}
				}
			}
			return (std::min)(MAX_DATES, num_intervening);
		}
	}

	return -1;	
}

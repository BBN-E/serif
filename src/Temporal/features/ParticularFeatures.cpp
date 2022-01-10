#include "Generic/common/leak_detection.h"
#include "ParticularFeatures.h"
#include <boost/shared_ptr.hpp>
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/Value.h"
#include "Generic/common/TimexUtils.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"

using std::wstring;
using std::wstringstream;
using boost::dynamic_pointer_cast;

std::wstring ParticularXFeature::pretty() const {
	wstringstream pretty;

	pretty << type() << L"(";

	if (!relation().is_null()) {
		pretty << relation();
	}

	pretty << L")";

	return pretty.str();
}

std::wstring ParticularXFeature::dump() const {
	return pretty();
}

std::wstring ParticularXFeature::metadata() const {
	wstringstream meta;

	TemporalFeature::metadata(meta);
	return meta.str();
}

bool ParticularXFeature::equals(const TemporalFeature_ptr& other) const {
	return topEquals(other);
}

size_t ParticularXFeature::calcHash() const {
	return topHash();
}

ParticularXFeature::ParticularXFeature(const Symbol& type, const Symbol& relation)
	: TemporalFeature(type, relation) {}

ParticularYearFeature::ParticularYearFeature(const Symbol& relation)
: ParticularXFeature(L"ParticularYearFeature", relation) {}

ParticularMonthFeature::ParticularMonthFeature(const Symbol& relation)
: ParticularXFeature(L"ParticularMonthFeature", relation) {}

ParticularDayFeature::ParticularDayFeature(const Symbol& relation)
: ParticularXFeature(L"ParticularDayFeature", relation) {}

// should refactor this with
// TemporalFeatureVectorGenerator::addTimexShapeFeatures

bool ParticularDayFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const {
	if (TemporalFeature::relationNameMatches(inst)) {
		const ValueMention* vm = inst->attribute()->valueMention();

		if (vm && vm->isTimexValue()) {
			Symbol date = vm->getDocValue()->getTimexVal();
			if (!date.is_null()) {
				wstring dateStr = date.to_string();

				if (TimexUtils::isParticularDay(dateStr)) {
					return true;
				}
			}
		}
	}

	return false;
}
			
bool ParticularMonthFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const {
	if (TemporalFeature::relationNameMatches(inst)) {
		const ValueMention* vm = inst->attribute()->valueMention();

		if (vm && vm->isTimexValue()) {
			Symbol date = vm->getDocValue()->getTimexVal();
			if (!date.is_null()) {
				wstring dateStr = date.to_string();

				if (TimexUtils::isParticularMonth(dateStr)) {
					return true;
				}
			}
		}
	}

	return false;
}
			
bool ParticularYearFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const {
	if (TemporalFeature::relationNameMatches(inst)) {
		const ValueMention* vm = inst->attribute()->valueMention();

		if (vm && vm->isTimexValue()) {
			Symbol date = vm->getDocValue()->getTimexVal();
			if (!date.is_null()) {
				wstring dateStr = date.to_string();

				if (TimexUtils::isParticularYear(dateStr)) {
					return true;
				}
			}
		}
	}

	return false;
}
			
bool ParticularYearFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (ParticularYearFeature_ptr f = 
				dynamic_pointer_cast<ParticularYearFeature>(other)) {
			return true;
		}
	}
	return false;
}

bool ParticularMonthFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (ParticularMonthFeature_ptr f = 
				dynamic_pointer_cast<ParticularMonthFeature>(other)) {
			return true;
		}
	}
	return false;
}

bool ParticularDayFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (ParticularDayFeature_ptr f = 
				dynamic_pointer_cast<ParticularDayFeature>(other)) {
			return true;
		}
	}
	return false;
}

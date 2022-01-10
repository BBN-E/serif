#ifndef _INTERVENING_DATES_FEATURE_
#define _INTERVENING_DATES_FEATURE_

#include <vector>
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Offset.h"
#include "Temporal/features/TemporalFeature.h"

BSP_DECLARE(ElfRelation)
BSP_DECLARE(ElfRelationArg)
BSP_DECLARE(InterveningDatesFeature)
class SentenceTheory;

class InterveningDatesFeature : public TemporalFeature {
public:
	InterveningDatesFeature(int n_dates, const Symbol& relation = Symbol());
	static InterveningDatesFeature_ptr create(const std::vector<std::wstring>& parts,
			const Symbol& relation);

	std::wstring pretty() const;
	std::wstring dump() const;
	std::wstring metadata() const;
	bool equals(const TemporalFeature_ptr& other) const;

	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;

	static void addFeaturesToInstance(const TemporalInstance& inst,
		const SentenceTheory* st, std::vector<TemporalFeature_ptr>& fv);
protected:
	size_t calcHash() const;
private:
	int _n_dates;
	static const int MAX_DATES;

	static int numInterveningDates(const TemporalInstance& inst,
		const SentenceTheory* st);
	static std::pair<EDTOffset, EDTOffset> constrainedRelationSpan(
			ElfRelation_ptr rel);
};

#endif


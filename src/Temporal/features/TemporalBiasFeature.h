#ifndef _TEMPORAL_BIAS_FEATURE_H_
#define _TEMPORAL_BIAS_FEATURE_H_

#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

BSP_DECLARE(TemporalBiasFeature)
BSP_DECLARE(TemporalInstance)

class TemporalBiasFeature : public TemporalFeature {
public:
	TemporalBiasFeature(unsigned int attributeType, const Symbol& relation=Symbol());
	static TemporalBiasFeature_ptr create(const std::vector<std::wstring>& parts, 
		const Symbol& relation);

	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;

	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;

protected:
	size_t calcHash() const;
private:
	unsigned int _attributeType;
};
#endif


#ifndef _PARTICULAR_EVENT_FEATURE_H_
#define _PARTICULAR_EVENT_FEATURE_H_

#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

BSP_DECLARE(TemporalEventFeature)
BSP_DECLARE(TemporalInstance)
class DocTheory;
class EventMention;
class EntitySet;

class TemporalEventFeature : public TemporalFeature {
public:
	TemporalEventFeature(const Symbol& eventType, const Symbol& temporalRole,
		const Symbol& relation, const Symbol& anchor = Symbol());
	static TemporalEventFeature_ptr create(const std::vector<std::wstring>& parts, 
		const Symbol& relation);

	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;

	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;

	static Symbol getTemporalRoleName(const EventMention* event,
		const TemporalInstance& inst);
	static bool matchesEvent(const EventMention* event, 
			const TemporalInstance& inst, const EntitySet* es);
protected:
	size_t calcHash() const;
private:
	Symbol _eventType;
	Symbol _temporalRole;
	Symbol _anchor;

	bool matchesEventFeatureConstraints(const EventMention* event,
			const TemporalInstance& inst) const;
};
#endif


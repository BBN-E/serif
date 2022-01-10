#ifndef _PARTICULAR_RELATION_FEATURE_H_
#define _PARTICULAR_RELATION_FEATURE_H_

#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

BSP_DECLARE(TemporalRelationFeature)
BSP_DECLARE(ElfRelation)
BSP_DECLARE(ElfRelationArg)
class RelMention;
class EntiySet;
class Mention;
class EntitySet;

class TemporalRelationFeature : public TemporalFeature {
public:
	TemporalRelationFeature(const Symbol& relationType, 
		const Symbol& relation = Symbol());
	static TemporalRelationFeature_ptr create(const std::vector<std::wstring>& parts, 
		const Symbol& relation);
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;

	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;

	static bool relMatchesRel(const RelMention* serifRel, 
			ElfRelation_ptr elfRel, const EntitySet* es);
	static bool relArgMatchesMention(ElfRelationArg_ptr arg, const Mention* ment, 
			const EntitySet* es);
protected:
	size_t calcHash() const;
private:
	Symbol _relationType;

};
#endif

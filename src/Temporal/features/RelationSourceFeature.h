/*#ifndef _RELATION_SOURCE_FEATURE_
#define _RELATION_SOURCE_FEATURE_

#include <vector>
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

BSP_DECLARE(ElfRelation)
BSP_DECLARE(RelationSourceFeature)
class RelationSourceFeature : public TemporalFeature {
public:
	RelationSourceFeature(const std::wstring& relationSourceComponent,
			const Symbol& relation = Symbol());
	static RelationSourceFeature_ptr create(const std::vector<std::wstring>& parts,
			const Symbol& relation);

	std::wstring pretty() const;
	std::wstring dump() const;
	std::wstring metadata() const;
	bool equals(const TemporalFeature_ptr& other) const;

	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;

	static void addFeaturesToInstance(const TemporalInstance& inst,
		std::vector<TemporalFeature_ptr>& fv);
protected:
	size_t calcHash() const;
private:
	std::wstring _sourceComponent;

};

#endif
*/

#ifndef _DATE_PROP_SPINE_H_
#define _DATE_PROP_SPINE_H_

#include <vector>
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

class ValueMention;
class Mention;
class Proposition;
class SentenceTheory;
class MentionSet;
BSP_DECLARE(TemporalInstance);
class DocTheory;

BSP_DECLARE(DatePropSpineFeature)
class DatePropSpineFeature : public TemporalFeature {
public:
	typedef std::vector<Symbol> Spine;
	DatePropSpineFeature(const Spine& spineElements,
			const Symbol& relation = Symbol());
	static DatePropSpineFeature_ptr create(const std::vector<std::wstring>& parts,
			const Symbol& relation);

	std::wstring pretty() const;
	std::wstring dump() const;
	std::wstring metadata() const;
	const std::vector<Symbol>& spine() const;
	bool equals(const TemporalFeature_ptr& other) const;
	static void spineHash(size_t& start, const Spine& spineElements);

	static std::vector<Spine> spinesFromDate(const ValueMention* vm, 
			const SentenceTheory* st);

	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
protected:
	size_t calcHash() const;
private:
	Spine _spineElements;

	static void spinesFromMention(std::vector<Spine>& ret,
			const Mention* m, const SentenceTheory* st, bool require_temp_role);
	static void searchForPropMentionPath(std::vector<Spine>& ret,
			std::vector<Symbol>& path, const Mention* m, const Proposition* p, 
			const MentionSet* ms, bool require_temp_role);
	static bool containsMentionSpan(const Proposition* p, const Mention* m,
			const MentionSet* ms);
};

#endif


#ifndef _TEMPORAL_ADJACENT_WORDS_FEATURE_H_
#define _TEMPORAL_ADJACENT_WORDS_FEATURE_H_

#include <map>
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

class SentenceTheory;
class DocTheory;
class ValueMention;
BSP_DECLARE(TemporalInstance)
BSP_DECLARE(TemporalAdjacentWordsFeature)

class TemporalAdjacentWordsFeature : public TemporalFeature {
public:
	TemporalAdjacentWordsFeature(const std::vector<Symbol>& syms,  bool preceding,
			const Symbol& relation = Symbol());
	static TemporalAdjacentWordsFeature_ptr create(
		const std::vector<std::wstring>& parts, const Symbol& relation);
	TemporalAdjacentWordsFeature_ptr copyWithRelation(const Symbol& symbol);
	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	size_t wordsHash(size_t start = 0) const;
protected:
	size_t calcHash() const;
private:
	std::vector<Symbol> _words;
	bool _preceding;

};

class TemporalAdjacentWordsFeatureProposer {
public:
	TemporalAdjacentWordsFeatureProposer () {}
	//void observe(const SentenceTheory* st);
	void addApplicableFeatures(const TemporalInstance& inst, 
			const SentenceTheory* st, std::vector<TemporalFeature_ptr>& fv) const;
	static std::vector<TemporalAdjacentWordsFeature_ptr> generate(
			const ValueMention* vm, const SentenceTheory* st);
private:
/*	typedef std::map<size_t, unsigned int> FeatCounts;
	FeatCounts _featureCounts;*/
	static const int PROPOSAL_LIMIT = 2;
};

#endif


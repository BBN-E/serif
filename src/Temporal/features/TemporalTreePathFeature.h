#ifndef _TEMPORAL_TREE_PATH_FEATURE_H_
#define _TEMPORAL_TREE_PATH_FEATURE_H_

#include <map>
#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

class SentenceTheory;
class DocTheory;
class ValueMention;
class SynNode;
BSP_DECLARE(TemporalInstance)
BSP_DECLARE(TemporalTreePathFeature)
BSP_DECLARE(ElfRelationArg)

class TemporalTreePathFeature : public TemporalFeature {
public:
	TemporalTreePathFeature(const Symbol& slot, const std::vector<Symbol>& upSpine,
			const std::vector<Symbol>& downSpine,
			const Symbol& relation);
	static TemporalTreePathFeature_ptr create(
		const std::vector<std::wstring>& parts, const Symbol& relation);
	TemporalTreePathFeature_ptr copyWithRelation(const Symbol& symbol);
	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	size_t pathHash(size_t start = 0) const;
protected:
	size_t calcHash() const;
private:
	Symbol _slot;
	std::vector<Symbol> _upPath;
	std::vector<Symbol> _downPath;
};

class TemporalTreePathFeatureProposer {
public:
	TemporalTreePathFeatureProposer () {}
	void observe(const TemporalInstance& inst, const SentenceTheory* st);
	void addApplicableFeatures(const TemporalInstance& inst, 
			const SentenceTheory* st, std::vector<TemporalFeature_ptr>& fv) const;
	static std::vector<TemporalTreePathFeature_ptr> generate(
			const TemporalInstance& inst, const SentenceTheory* st);
	static TemporalTreePathFeature_ptr generateForArg(const SentenceTheory* st,
			const Symbol& role, ElfRelationArg_ptr arg, const ValueMention* vm,
			const Symbol& relation);
	static std::vector<const SynNode*> pathToRoot(const SynNode* node);
private:
	static const SynNode* synNodeForArg(const SentenceTheory* st, ElfRelationArg_ptr arg);
	static const SynNode* synNodeForVM(const SentenceTheory* st, const ValueMention* vm);
	static const SynNode* synNodeForVM(const SynNode* node, int start_tok, int end_tok);
/*	typedef std::map<size_t, unsigned int> FeatCounts;
	FeatCounts _featureCounts;*/
};

#endif



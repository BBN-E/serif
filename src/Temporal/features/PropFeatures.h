#ifndef _TEMPORAL_TREE_JOIN_FEATURE_H_
#define _TEMPORAL_TREE_JOIN_FEATURE_H_

#include <map>
#include <vector>
#include "Generic/common/bsp_declare.h"
#include "Generic/PropTree/PropNode.h"
#include "Temporal/features/TemporalFeature.h"

class SentenceTheory;
class DocTheory;
class ValueMention;
class SynNode;
BSP_DECLARE(TemporalInstance)
//BSP_DECLARE(PropJoinWord)
BSP_DECLARE(PropAttWord)
BSP_DECLARE(ElfRelationArg)
BSP_DECLARE(DocPropForest)

/*
class PropJoinWord : public TemporalFeature {
public:
	PropJoinWord(const Symbol& word, const Symbol& relation);
	static PropJoinWord_ptr create(
		const std::vector<std::wstring>& parts, const Symbol& relation);
	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	static PropJoinWord_ptr generate(const TemporalInstance& inst,
			const SentenceTheory* st);
	static void addFeatures(const TemporalInstance& inst, const SentenceTheory* st,
		std::vector<TemporalFeature_ptr>& features);
protected:
	size_t calcHash() const;
private:
	Symbol _word;
};
*/

class PropAttWord : public TemporalFeature {
public:
	PropAttWord(const Symbol& role, const Symbol& word, const Symbol& relation);
	static PropAttWord_ptr create(
		const std::vector<std::wstring>& parts, const Symbol& relation);
	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	static void addToFV(const TemporalInstance& inst,
			const DocTheory* dt, int sn, std::vector<TemporalFeature_ptr>& fv);
protected:
	size_t calcHash() const;
private:
	Symbol _word;
	Symbol _role;

	static PropNode::ptr_t nodeForMention(DocPropForest_ptr forest, int sn, 
			const Mention* m);
	static PropNode::ptr_t findMentionUnderNode(PropNode_ptr node, const Mention* m);
	static void addToFVForMention(const Mention* m, DocPropForest_ptr forest, 
			int sn, const std::wstring& role, const std::wstring& relationName,
			std::vector<TemporalFeature_ptr>& fv);
};
#endif


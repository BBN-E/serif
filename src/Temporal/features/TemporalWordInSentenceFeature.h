#ifndef _TEMPORAL_WORD_IN_SENTENCE_FEATURE_H_
#define _TEMPORAL_WORD_IN_SENTENCE_FEATURE_H_

#include "Generic/common/bsp_declare.h"
#include "Temporal/features/TemporalFeature.h"

class SynNode;
class DocTheory;
BSP_DECLARE(TemporalInstance)
BSP_DECLARE(TemporalWordInSentenceFeature)

class TemporalWordInSentenceFeature : public TemporalFeature {
public:
	TemporalWordInSentenceFeature(const Symbol& word, 
			const Symbol& relation = Symbol());
	static TemporalWordInSentenceFeature_ptr create(
		const std::vector<std::wstring>& parts, const Symbol& relation);
	virtual std::wstring pretty() const;
	virtual std::wstring dump() const;
	virtual std::wstring metadata() const;
	virtual bool equals(const TemporalFeature_ptr& other) const;
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	static bool isWordFeatureTriggerPOS(const Symbol& pos);
protected:
	size_t calcHash() const;
private:
	Symbol _word;

	bool matches(const SynNode* node) const; 
};

#endif


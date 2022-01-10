#ifndef _MANUAL_TEMPORAL_
#define _MANUAL_TEMPORAL_

#include <vector>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"
#include "TemporalInstance.h"
#include "TemporalInstanceGenerator.h"

class SentenceTheory;
class Mention;
class ValueMention;
class SynNode;
BSP_DECLARE(TemporalTypeTable)
BSP_DECLARE(ElfRelation)

class ManualTemporalInstanceGenerator : public TemporalInstanceGenerator {
public:
	ManualTemporalInstanceGenerator(TemporalTypeTable_ptr typeTable);
	static bool mentionsFromRelation(const SentenceTheory* st, 
			ElfRelation_ptr relation, std::vector<const Mention*>& mentions);
protected:
	virtual void instancesInternal(const Symbol& docid, const SentenceTheory* st, 
			ElfRelation_ptr relation, TemporalInstances& instances);
private:
	static bool isCandidateVM(const ValueMention* vm);
	static bool isGoodDateAttachment(const SentenceTheory* st, 
			ElfRelation_ptr relation, const std::vector<const Mention*>&  mentions, 
			const ValueMention* vm, std::wstring& proposalSource);
	static bool onlyOneVerbInSentence(const SentenceTheory* st);
	static int countVPs(const SynNode* node);
	static bool dateAdjacentToArgument(const ValueMention* vm, const Mention* m);
	static bool dateWithinArgument(const ValueMention* vm, const Mention* m);
	static bool dateInAdjacentPrepositionalPhrase(const SentenceTheory* st,
			const ValueMention* vm, const std::vector<const Mention*>& mentions);
	static bool directlyConnectedToJoiningTreePath(const SentenceTheory* st,
			const ValueMention* vm, const std::vector<const Mention*>& mentions);
	static const SynNode* synNodeForVM(const ValueMention* vm, const SentenceTheory* st);
	static const Mention* mentionForVM(const ValueMention* vm, const SentenceTheory* st);
	static const SynNode* ppContainingDateNode(const SynNode* node);
	static bool adjacent(const SynNode* node, const Mention* m);
	static bool immediatelyDominatesTag(const SynNode* node, const Symbol& tag);
	static const SynNode* mentionForVMSyntaxOnly(const ValueMention* vm,
			const SentenceTheory* st);
	static const SynNode* mentionForVMSyntaxOnly(const ValueMention* vm,
			const SynNode* node);
	static bool dominatesValueMention(const Mention* m, const ValueMention* vm);
	static bool dominatesValueMention(const SynNode* m, const ValueMention* vm);
	static int numDominatedVMs(const Mention* m, const SentenceTheory* st);
	static bool onlyDateCase(ElfRelation_ptr relation, const ValueMention* vm,
			const SentenceTheory* st);
};
	
#endif


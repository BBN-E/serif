#include "Generic/common/leak_detection.h"
#include "ManualTemporalInstanceGenerator.h"

#include <limits>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/patterns/QueryDate.h"
#include "English/parse/en_STags.h"
#include "PredFinder/elf/ElfRelation.h"
#include "LearnIt/MainUtilities.h"
#include "TemporalAttribute.h"
#include "TemporalTypeTable.h"

using std::wstring;
using boost::make_shared;

ManualTemporalInstanceGenerator::ManualTemporalInstanceGenerator(
	    TemporalTypeTable_ptr typeTable) : TemporalInstanceGenerator(typeTable) {}

void ManualTemporalInstanceGenerator::instancesInternal(
		const Symbol& docid, const SentenceTheory* st, 
		ElfRelation_ptr relation, TemporalInstances& instances)
{
	std::vector<const Mention*> mentions;
	if (mentionsFromRelation(st, relation, mentions)) {
		const ValueMentionSet* vms = st->getValueMentionSet();
		for (int i = 0; i< vms->getNValueMentions(); ++i) {
			const ValueMention* vm = vms->getValueMention(i);
			if (vm->getSentenceNumber() == st->getSentNumber()) {
				if (isCandidateVM(vm)) {
					std::wstring proposalSource;
					if (isGoodDateAttachment(st, relation, mentions, vm, proposalSource)) {
						BOOST_FOREACH(const unsigned int attributeType, typeTable()->ids()) {
							TemporalInstance_ptr inst = 
								make_shared<TemporalInstance>(docid, 
										st->getSentNumber(), relation,
										make_shared<TemporalAttribute>(attributeType, vm),
										makePreviewString(st, relation, attributeType, vm),
										proposalSource);
							instances.push_back(inst);
						}
					}
				}
			}
		}
	}
}

bool ManualTemporalInstanceGenerator::isCandidateVM(const ValueMention* vm) {
	// use only specific dates, not things like "weekly"
	return vm && vm->isTimexValue() && QueryDate::isSpecificDate(vm->getDocValue());
}


bool ManualTemporalInstanceGenerator::mentionsFromRelation(
		const SentenceTheory* st, ElfRelation_ptr relation,
		std::vector<const Mention*>& mentions) 
{
	const MentionSet* ms = st->getMentionSet();
	bool all_mentions = true;

	BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
		const Mention* m = 0;
		ElfIndividual_ptr indiv = arg->get_individual();

		if (indiv) {
			if (indiv->has_mention_uid()) {
				mentions.push_back(ms->getMention(indiv->get_mention_uid()));
			} else if (indiv->has_value_mention_id()) {
				// value mentions do not count against
				// all-mentionessossity, but neither do they get
				// added to the mention vector
			} else {
				all_mentions = false;
			}
		}
	}

	return all_mentions;
}

bool ManualTemporalInstanceGenerator::isGoodDateAttachment(
		const SentenceTheory* st, ElfRelation_ptr relation, 
		const std::vector<const Mention*>& mentions, const ValueMention* vm,
		std::wstring& proposalSource)
{
	proposalSource.clear();
	BOOST_FOREACH(const Mention* m, mentions) {
		if (dateWithinArgument(vm, m) 
				|| dateAdjacentToArgument(vm, m)) 
		{
			proposalSource = L"AdjacentOrWithin";
			return true;
		}
	}

	if (dateInAdjacentPrepositionalPhrase(st, vm, mentions)) {
		proposalSource = L"AdjacentPreposition";
		return true;
	}

	if (onlyOneVerbInSentence(st)) {
		proposalSource = L"onlyVerb";
		return true;
	}

	if (directlyConnectedToJoiningTreePath(st, vm, mentions)) {
		proposalSource = L"directConnectTreePath";
		return true;
	}

	if (onlyDateCase(relation, vm, st)) {
		proposalSource = L"onlyDateForEmployOrMember";
		return true;
	}

	return false;
}

bool ManualTemporalInstanceGenerator::onlyDateCase(ElfRelation_ptr relation,
		const ValueMention* vm, const SentenceTheory* st)
{
	// many times a person's role or employment will be mentioned in a 
	// sentence where an event is happening or another relation is being
	// expressed.  Usually we can then assert the employment relation to
	// be true by implication as well. We only do this if there is one
	// date in the sentence, though, to prevent confusion.
	
	const wstring relName = relation->get_name();
	if (relName == L"eru:HasEmployer" || relName == L"eru:PersonTitleInOrganization"
			|| relName == L"eru:PersonInOrganization" 
			|| relName == L"eru:HasTopMemberOrEmployee")
	{
		const ValueMentionSet* vms = st->getValueMentionSet();
		for (int i=0; i<vms->getNValueMentions(); ++i) {
			const ValueMention* otherVM = vms->getValueMention(i);
			if (otherVM != vm && isCandidateVM(otherVM)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool ManualTemporalInstanceGenerator::onlyOneVerbInSentence(const SentenceTheory* st) {
	return countVPs(st->getPrimaryParse()->getRoot()) == 1;
}

int ManualTemporalInstanceGenerator::countVPs(const SynNode* node) {
	int ret = 0;
	// count how many VPs which are not head-children of other VPs are in the tree
	// this may be off for conjoined VPs...
	if (node->getTag() == EnglishSTags::VP) {
		const SynNode* parent = node->getParent();
		const SynNode* head = node->getHead();

		if (!parent || parent->getTag() != EnglishSTags::VP 
				|| parent->getHead() != node) 
		{
			++ret;
		}
	}

	for (int i=0; i<node->getNChildren(); ++i) {
		ret += countVPs(node->getChild(i));
	}

	return ret;
}

bool ManualTemporalInstanceGenerator::dateAdjacentToArgument(
		const ValueMention* vm, const Mention* m)
{
	return (m->getNode()->getStartToken() == vm->getEndToken() + 1) 
		|| (m->getNode()->getEndToken() + 1 == vm->getStartToken());
}

bool ManualTemporalInstanceGenerator::dateWithinArgument(
		const ValueMention* vm, const Mention* m)
{
	return m->getNode()->getStartToken() <= vm->getStartToken()
		&& m->getNode()->getEndToken() >= vm->getEndToken();
}

bool ManualTemporalInstanceGenerator::dateInAdjacentPrepositionalPhrase(
		const SentenceTheory* st, const ValueMention* vm,
		const std::vector<const Mention*>& mentions)
{
	const SynNode* node = synNodeForVM(vm, st);

	if (node) {
		const SynNode* pp = ppContainingDateNode(node);
		for (const SynNode* pp = ppContainingDateNode(node); 
				pp && pp->getTag() == EnglishSTags::PP; pp = pp->getParent()) 
		{
			// we go up through multiple levels of PPs to handle things like
			// (PP (PP from 1983) (PP to 1990))
			BOOST_FOREACH(const Mention* m, mentions) {
				if (adjacent(pp, m)) {
					return true;
				}
			}
		}
	}
	return false;
}

const SynNode* ManualTemporalInstanceGenerator::synNodeForVM(const ValueMention* vm,
		const SentenceTheory* st)
{
	// first see if we align to a mention...
	const Mention* m = mentionForVM(vm, st);

	if (m) {
		// we need to check if the mention dominates multiple value
		// mentions. This happens for things like "1983 to 1990" where
		// the mention is at the QP-level which dominates both dates
		// (at least in some parses)

		switch (numDominatedVMs(m, st)) {
			case 0:
				throw UnrecoverableException("ManualTemporalInstanceGenerator::synNodeForVM",
						"Logic error - this should be impossible.");
			case 1:
				return m->getNode();
			default:
				return mentionForVMSyntaxOnly(vm, st);
		}
	}
	return 0;
}

int ManualTemporalInstanceGenerator::numDominatedVMs(const Mention* m,
		const SentenceTheory* st)
{
	const ValueMentionSet* vms = st->getValueMentionSet();
	int ret = 0;

	for (int i=0; i<vms->getNValueMentions(); ++i) {
		const ValueMention* vm = vms->getValueMention(i);
		if (dominatesValueMention(m, vm)) {
			++ret;
		}
	}
	return ret;
}

bool ManualTemporalInstanceGenerator::dominatesValueMention(const Mention* m,
		const ValueMention* vm)
{
	return dominatesValueMention(m->getNode(), vm);
}

bool ManualTemporalInstanceGenerator::dominatesValueMention(const SynNode* node,
		const ValueMention* vm)
{
	return vm->getStartToken()>=node->getStartToken()
		&& vm->getEndToken()<=node->getEndToken();
}

const SynNode* ManualTemporalInstanceGenerator::mentionForVMSyntaxOnly(
		const ValueMention* vm, const SentenceTheory* st)
{
	const SynNode* root = st->getPrimaryParse()->getRoot();

	if (dominatesValueMention(root, vm)) {
		return mentionForVMSyntaxOnly(vm, root);
	} else {
		throw UnexpectedInputException("ManualTemporalInstanceGenerator::mentionForVMSyntaxOnly",
				"Root note does not dominate value mention. This should never happen.");
	}
}

const SynNode* ManualTemporalInstanceGenerator::mentionForVMSyntaxOnly(
		const ValueMention* vm, const SynNode* node)
{
	for (int i=0; i<node->getNChildren(); ++i) {
		const SynNode* kid = node->getChild(i);
		if (dominatesValueMention(node, vm)) {
			return mentionForVMSyntaxOnly(vm, kid);
		}
	}

	return node;
}

const Mention* ManualTemporalInstanceGenerator::mentionForVM(const ValueMention* vm, 
		const SentenceTheory* st)
{
	const MentionSet* ms = st->getMentionSet();
	const Mention* bestMention = 0;
	int shortest_token_span = std::numeric_limits<int>::max();

	for (int i=0; i<ms->getNMentions(); ++i) {
		const Mention* m = ms->getMention(i);
		const SynNode* mSyn = m->getNode();
		if (mSyn->getStartToken() <= vm->getStartToken() &&
				mSyn->getEndToken() >= vm->getEndToken())
		{
			int token_span = mSyn->getEndToken() - mSyn->getStartToken();
			if (token_span < shortest_token_span) {
				bestMention = m;
				shortest_token_span = token_span;
			}
		}
	}

	return bestMention;
}

const SynNode* ManualTemporalInstanceGenerator::ppContainingDateNode(const SynNode* node) {
	const SynNode* par = node->getParent();

	if (par) {
		// simple (PP IN NP) case
		if (par->getTag() == EnglishSTags::PP) {
			return par;
		}

		if (par->getTag() == EnglishSTags::NP) {
			if (immediatelyDominatesTag(par, EnglishSTags::CC)) {
				const SynNode* grandPar = par->getParent();
				if (grandPar && grandPar->getTag() == EnglishSTags::PP) {
					// cases like (PP IN (NP DATE CC DATE))
					// "between 1997 and 2005"
					return grandPar;
				}
			}
		} else if (par->getTag() == EnglishSTags::QP) {
			const SynNode* grandPar = par->getParent();
			if (grandPar) {
				if (grandPar->getTag() == EnglishSTags::NP) {
					const SynNode * greatGrandPar = grandPar->getParent();
					if (greatGrandPar &&  greatGrandPar->getTag() == EnglishSTags::NP) {
						// (PP IN (NP (QP ....)))
						// e.g. "from 1983 to 1990"
						return greatGrandPar;
					}
				}
			}
		}
	}

	return 0;
}

bool ManualTemporalInstanceGenerator::immediatelyDominatesTag(const SynNode* node,
		const Symbol& tag) 
{
	for (int i=0; i<node->getNChildren(); ++i) {
		if (node->getChild(i)->getTag() == tag) {
			return true;
		}
	}
	return false;
}


bool ManualTemporalInstanceGenerator::adjacent(const SynNode* node,
		const Mention* m) {
	const SynNode* mSyn = m->getNode();
	return (node->getStartToken() == mSyn->getEndToken() + 1) 
		|| (node->getEndToken() + 1 == mSyn->getStartToken());
}


bool ManualTemporalInstanceGenerator::directlyConnectedToJoiningTreePath(
		const SentenceTheory* st, const ValueMention* vm,
		const std::vector<const Mention*>& mentions)
{
	return false;
}


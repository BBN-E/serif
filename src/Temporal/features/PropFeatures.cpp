#include "Generic/common/leak_detection.h"
#include "PropFeatures.h"
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/foreach_pair.hpp"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/PropTree/DocPropForest.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"
#include "Temporal/features/TemporalTreePathFeature.h"
#include "Temporal/ManualTemporalInstanceGenerator.h"

using std::vector;
using std::wstring;
using std::wstringstream;
using std::make_pair;
using boost::dynamic_pointer_cast;
using boost::make_shared;
typedef PropNode::ptr_t PropNode_ptr;
/*
PropJoinWord::PropJoinWord(const Symbol& word, const Symbol& relation)
	: TemporalFeature(L"Join", relation), _word(word)
{ }

PropJoinWord_ptr PropJoinWord::create(
	const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (parts.size() != 1) {
		throw UnexpectedInputException("PropJoinWord",
			"In metadata, expected exactly one field.");
	}

	Symbol word = parts[0];

	return make_shared<PropJoinWord>(word, relation);
}

std::wstring PropJoinWord::pretty() const {
	wstringstream str;

	str << type() << L"(" << _word << L")";

	return str.str();
}

std::wstring PropJoinWord::dump() const {
	return pretty();
}

std::wstring PropJoinWord::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << _word;

	return str.str();
}

bool PropJoinWord::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (PropJoinWord_ptr tjwf = 
				dynamic_pointer_cast<PropJoinWord>(other)) {
			return tjwf->_word == _word;
		}
	}
	return false;
}

size_t PropJoinWord::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _word.to_string());
	return ret;
}

bool PropJoinWord::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!TemporalFeature::relationNameMatches(inst)) {
		return false;
	}

	PropJoinWord_ptr featFromOther = generate(*inst, dt->getSentenceTheory(sn));

	if (featFromOther) {
		return equals(featFromOther);
	}

	return false;
}
PropJoinWord_ptr 
PropJoinWord::generate(const TemporalInstance& inst, const SentenceTheory* st)
{
	std::vector<const Mention*> mentions;
	ManualTemporalInstanceGenerator::mentionsFromRelation(st, inst.relation(),
			mentions);

	if (mentions.size() == 2) {
		std::vector<const SynNode*> path1 = 
			TemporalTreePathFeatureProposer::pathToRoot(mentions[0]->getNode());
		std::vector<const SynNode*> path2 = 
			TemporalTreePathFeatureProposer::pathToRoot(mentions[1]->getNode());
		size_t idx1 = path1.size() - 1;
		size_t idx2 = path2.size() - 1;

		if (!path1.empty() && !path2.empty() && path1[idx1] == path2[idx2]) {
			for (;idx1 >= 1 && idx2>=1 && path1[idx1] == path2[idx2]; --idx1, --idx2) {
			}

			if (path1[idx1] != path2[idx2]) {
				++idx1;
			}

			const SynNode* joinNode = path1[idx1];
			Symbol joinSym = joinNode->getHeadWord();

			if (!joinSym.is_null()) {
				return make_shared<PropJoinWord>(joinSym, inst.relation()->get_name());
			}
		}
	} 

	return PropJoinWord_ptr();
}

void PropJoinWord::addFeatures(const TemporalInstance& inst, const SentenceTheory* st,
		std::vector<TemporalFeature_ptr>& features)
{
	PropJoinWord_ptr feat = generate(inst,st);
	if (feat) {
		features.push_back(feat);
	}
}
*/

PropAttWord::PropAttWord(const Symbol& word, const Symbol& role,
		const Symbol& relation)
	: TemporalFeature(L"Att", relation), _word(word), _role(role)
{ }

PropAttWord_ptr PropAttWord::create(
	const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (parts.size() != 2) {
		throw UnexpectedInputException("PropAttWord",
			"In metadata, expected exactly one field.");
	}

	Symbol word = parts[0];
	Symbol role = parts[1];

	return make_shared<PropAttWord>(word, role, relation);
}

std::wstring PropAttWord::pretty() const {
	wstringstream str;

	str << type() << L"(" << _word << L", " << _role << L")";

	return str.str();
}

std::wstring PropAttWord::dump() const {
	return pretty();
}

std::wstring PropAttWord::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << _word << L"\t" << _role;

	return str.str();
}

bool PropAttWord::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (PropAttWord_ptr tjwf = 
				dynamic_pointer_cast<PropAttWord>(other)) {
			return tjwf->_word == _word && tjwf->_role == _role;
		}
	}
	return false;
}

size_t PropAttWord::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _word.to_string());
	boost::hash_combine(ret, _role.to_string());
	return ret;
}

bool PropAttWord::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!TemporalFeature::relationNameMatches(inst)) {
		return false;
	}

	const SentenceTheory* st = dt->getSentenceTheory(sn);
	const MentionSet* ms = st->getMentionSet();
	const Mention* m = inst->mentionForRole(_role.to_string(), ms);

	if (m) {
		DocPropForest_ptr forest = dt->getPropForest();
		PropNode_ptr node = nodeForMention(forest, sn, m);
		if (node) {
			for (size_t i=0; i<node->getNParents(); ++i) {
				PropNode_ptr parent = node->getParent(i);

				const Predicate* pred = parent->getRepresentativePredicate();
				return pred && !pred->pred().is_null() && pred->pred() == _word;
			}
		}
	}

	return false;
}

void 
PropAttWord::addToFV(const TemporalInstance& inst, const DocTheory* dt, int sn,
		vector<TemporalFeature_ptr>& fv)
{
	const MentionSet* ms = dt->getSentenceTheory(sn)->getMentionSet();
	const EntitySet* es = dt->getSentenceTheory(sn)->getEntitySet();
	DocPropForest_ptr forest = dt->getPropForest();

	BOOST_FOREACH(ElfRelationArg_ptr arg, inst.relation()->get_args()) {
		ElfIndividual_ptr indiv = arg->get_individual();
		if (indiv && indiv->has_mention_uid() ) {
			const Mention* m =  ms->getMention(indiv->get_mention_uid());
			addToFVForMention(m, forest, sn, arg->get_role(), 
					inst.relation()->get_name(), fv);
			const Entity* ent = es->getEntityByMention(m->getUID());
	
			if (ent) {
				for (int i=0; i<ent->getNMentions(); ++i) {
					const Mention* otherMent = es->getMention(ent->getMention(i));
					if (otherMent!=m && otherMent->getSentenceNumber() == sn) {
						addToFVForMention(otherMent, forest, sn, arg->get_role(),
								inst.relation()->get_name(), fv);
					}
				}
			}
		}
	}
}

void PropAttWord::addToFVForMention(const Mention* m , DocPropForest_ptr forest,
		int sn, const std::wstring& role, const std::wstring& relationName,
		std::vector<TemporalFeature_ptr>& fv)
{
	PropNode_ptr node = nodeForMention(forest, sn, m);
	if (node) {
		for (size_t i=0; i<node->getNParents(); ++i) {
			PropNode_ptr parent = node->getParent(i);
			const Predicate* pred = parent->getRepresentativePredicate();
			if (pred && !pred->pred().is_null()) {
				fv.push_back(make_shared<PropAttWord>(role, pred->pred(), 
							relationName));
			}
		}
	}
}

// eventually, move this to DocPropForest
PropNode_ptr PropAttWord::nodeForMention(DocPropForest_ptr forest, int sn,
		const Mention* m)
{
	PropNodes_ptr nodes = (*forest)[sn];
	PropNode_ptr mentNode = PropNode_ptr();

	BOOST_FOREACH(PropNode_ptr node, *nodes) {
		mentNode = findMentionUnderNode(node, m);
		if (mentNode) {
			break;
		}
	}
	
	return mentNode;
}

PropNode_ptr PropAttWord::findMentionUnderNode(PropNode_ptr node,
		const Mention* m) 
{
	if (node->getMention() == m) {
		return node;
	}

	PropNode_ptr ret = PropNode_ptr();
	for (size_t i=0; i<node->getNChildren(); ++i) {
		ret = findMentionUnderNode(node->getChildren()[i], m);
		if (ret) {
			break;
		}
	}

	return ret;
}



#include "Generic/common/leak_detection.h"
#include "TemporalTreePathFeature.h"
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
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"

using std::wstring;
using std::wstringstream;
using std::make_pair;
using boost::dynamic_pointer_cast;
using boost::make_shared;

TemporalTreePathFeature::TemporalTreePathFeature(
		const Symbol& slot, const std::vector<Symbol>& upPath,
		const std::vector<Symbol>& downPath,
		const Symbol& relation)
	: TemporalFeature(L"TreePath", relation), _upPath(upPath), 
	_downPath(downPath), _slot(slot)
{
	if (upPath.empty()) {
		throw UnexpectedInputException("TemporalTreePathFeature::TemporalTreePathFeature",
				"Up path cannot be empty.");
	}

	if (downPath.empty()) {
		throw UnexpectedInputException("TemporalTreePathFeature::TemporalTreePathFeature",
				"Up path cannot be empty.");
	}
}

TemporalTreePathFeature_ptr TemporalTreePathFeature::copyWithRelation(
		const Symbol& relation) 
{
	return make_shared<TemporalTreePathFeature>(_slot, _upPath, _downPath, 
			relation);
}


TemporalTreePathFeature_ptr TemporalTreePathFeature::create(
	const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (parts.size() < 1) {
		throw UnexpectedInputException("TemporalTreePathFeature",
			"In metadata, expected slot number but found nothing.");
	}

	std::wstring slot = parts[0];

	if (parts.size() < 2) {
		throw UnexpectedInputException("TemporalTreePathFeature",
				"In metadata, expected up path size in second position, found nothing");
	}

	unsigned int up_path_size;

	try {
		up_path_size = boost::lexical_cast<unsigned int>(parts[1]);
	} catch (boost::bad_lexical_cast&) {
		throw UnexpectedInputException("TemporalTreePathFeature",
				"Expected up path size, an integer, but found something else");
	}

	if (2+up_path_size > parts.size()) {
		throw UnexpectedInputException("TemporalTreePathFeature",
				"Up path claims more components than are present.");
	}

	std::vector<Symbol> upPath;
	for (unsigned int i=0; i<up_path_size; ++i) {
		upPath.push_back(parts[2+i]);
	}

	int base_index = 2 + up_path_size;

	if (base_index >= (int)parts.size()) {
		throw UnexpectedInputException("TemporalTreePathFeature::create",
				"In metadata, after processing up path, nothing left for down path");
	}

	unsigned int down_path_size;
	try {
		down_path_size = boost::lexical_cast<unsigned int>(parts[base_index]);
	} catch (boost::bad_lexical_cast&) {
		throw UnexpectedInputException("TemporalTreePathFeature::create",
				"Could not parse down path size in metadata");
	}

	if (base_index + down_path_size >= parts.size()) {
		throw UnexpectedInputException("TemporalTreePathFeature::create",
				"Not enough parts remaining in metadata for down path");
	}

	std::vector<Symbol> downPath;
	for (unsigned int i=0; i<down_path_size; ++i) {
		downPath.push_back(parts[base_index + i + 1]);
	}

	return make_shared<TemporalTreePathFeature>(slot, upPath, downPath, relation);
}

std::wstring TemporalTreePathFeature::pretty() const {
	wstringstream str;

	str << type() << L"(<" << _slot << L"> ";

	BOOST_FOREACH(const Symbol& s, _upPath) {
		str << L"> " << s.to_string();
	}

	BOOST_REVERSE_FOREACH(const Symbol& s, _downPath) {
		str << L" <" << s.to_string();
	}

	str << L")";

	return str.str();
}

std::wstring TemporalTreePathFeature::dump() const {
	return pretty();
}

std::wstring TemporalTreePathFeature::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << _slot;
	
	str << L"\t" << _upPath.size();

	BOOST_FOREACH(const Symbol& s, _upPath) {
		str << L"\t" << s.to_string();
	}

	str << L"\t" << _downPath.size(); 

	BOOST_FOREACH(const Symbol& s, _downPath) {
		str << L"\t" << s.to_string();
	}
	
	return str.str();
}

bool TemporalTreePathFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (TemporalTreePathFeature_ptr ttpf = 
				dynamic_pointer_cast<TemporalTreePathFeature>(other)) {
			return ttpf->_slot == _slot && ttpf->_upPath == _upPath &&
				ttpf->_downPath == _downPath;
		}
	}
	return false;
}

size_t TemporalTreePathFeature::calcHash() const {
	return pathHash(topHash());
}

size_t TemporalTreePathFeature::pathHash(size_t start) const {
	size_t ret = start;
	boost::hash_combine(ret, _slot.to_string());
	boost::hash_combine(ret, _upPath.size());
	BOOST_FOREACH(const Symbol& component, _upPath) {
		boost::hash_combine(ret, component.to_string());
	}
	boost::hash_combine(ret, _downPath.size());
	BOOST_FOREACH(const Symbol& component, _downPath) {
		boost::hash_combine(ret, component.to_string());
	}
	return ret;
}

bool TemporalTreePathFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!TemporalFeature::relationNameMatches(inst)) {
		return false;
	}

	const SentenceTheory* st = dt->getSentenceTheory(sn);
	const ValueMention* vm = inst->attribute()->valueMention();

	if (vm) {
		ElfRelationArg_ptr arg = inst->relation()->get_arg(_slot.to_string());

		if (arg) {
			TemporalTreePathFeature_ptr feat = 
				TemporalTreePathFeatureProposer::generateForArg(st, _slot, arg, 
						vm, inst->relation()->get_name());
			if (feat) {
				return equals(feat);
			}
		}
	}

	return false;
}

/*void TemporalTreePathFeatureProposer::observe(const TemporalInstance& inst, 
		const SentenceTheory* st)
{
	const ValueMentionSet* vms = st->getValueMentionSet();

	for (int i=0; i<vms->getNValueMentions(); ++i) {
		const ValueMention* vm = vms->getValueMention(i);
		BOOST_FOREACH(TemporalTreePathFeature_ptr feat, generate(inst, st)) {
			size_t hsh = feat->hash();
			FeatCounts::iterator probe  = _featureCounts.find(hsh);
			if (probe != _featureCounts.end()) {
				++probe->second;
			} else {
				_featureCounts.insert(make_pair(hsh, 1));
			}
		}
	}
}*/

void TemporalTreePathFeatureProposer::addApplicableFeatures(
		const TemporalInstance& inst, const SentenceTheory* st,
		std::vector<TemporalFeature_ptr>& fv) const
{
	const ValueMention* vm = inst.attribute()->valueMention();
	if (vm) {
		std::vector<TemporalTreePathFeature_ptr> feats = generate(inst, st);
		BOOST_FOREACH(TemporalTreePathFeature_ptr feat, feats) {
			/*size_t hsh = feat->hash();
			FeatCounts::const_iterator probe = _featureCounts.find(hsh);
			if (probe!= _featureCounts.end() && probe->second > 2) {*/
				fv.push_back(feat);
				fv.push_back(feat->copyWithRelation(inst.relation()->get_name()));
			//}
		}
	}
}


std::vector<TemporalTreePathFeature_ptr> 
TemporalTreePathFeatureProposer::generate(const TemporalInstance& inst,
		const SentenceTheory* st)
{
	std::vector<TemporalTreePathFeature_ptr> ret;

	if (inst.attribute()->valueMention()) {
		BOOST_FOREACH_PAIR(const std::wstring& role, 
				const std::vector<ElfRelationArg_ptr>& args,
				inst.relation()->get_arg_map()) 
		{
			if (args.size() == 0) {
				SessionLogger::warn("temp_path_empty_arg") << "When generating "
					<< " temporal path features, encountered empty argument list "
					<< " for role.";
			} else {
				if (args.size() > 1) {
					SessionLogger::warn("temp_path_too_many") << "When generating "
						<< " temporal path features, only using first of multiple "
						<< "arguments for role type.";
				}

				TemporalTreePathFeature_ptr feat = 
					generateForArg(st, role, args[0], inst.attribute()->valueMention(),
							inst.relation()->get_name());
				if (feat) {
					ret.push_back(feat);
				}
			}
		}
	}

	return ret;
}

TemporalTreePathFeature_ptr TemporalTreePathFeatureProposer::generateForArg(
		const SentenceTheory* st, const Symbol& role, ElfRelationArg_ptr arg, 
		const ValueMention* vm, const Symbol& relation) 
{
	TemporalTreePathFeature_ptr ret;
	const SynNode* argNode = synNodeForArg(st, arg);
	const SynNode* tmpNode = synNodeForVM(st, vm);

	if (argNode && tmpNode) {
		std::vector<const SynNode*> argNodes = pathToRoot(argNode);
		std::vector<const SynNode*> tmpNodes = pathToRoot(tmpNode);

		if (!argNodes.empty() && !tmpNodes.empty()) {
			size_t argIdx = argNodes.size() - 1;
			size_t tmpIdx = tmpNodes.size() - 1;

			if (argNodes[argIdx] == tmpNodes[tmpIdx]) {
				while (argIdx >0 && tmpIdx > 0
						&& argNodes[argIdx -1] == tmpNodes[tmpIdx -1])
				{
					--argIdx;
					--tmpIdx;
				}

				std::vector<Symbol> upPath;
				std::vector<Symbol> downPath;

				for (size_t i=0; i<=argIdx; ++i) {
					upPath.push_back(argNodes[i]->getTag());
				}

				for (size_t i=0; i<=tmpIdx; ++i) {
					downPath.push_back(tmpNodes[i]->getTag());
				}

				ret = make_shared<TemporalTreePathFeature>(role, upPath,
						downPath, relation);
			/*	SessionLogger::info("tree_path") << "Generated " 
					<< ret->pretty();*/
			} else {
				SessionLogger::warn("no_tmp_tree_path") << L"could not generate temporal "
					<< L"tree path feature: paths do no match at root";
			}
		} else {
			/*SessionLogger::warn("no_tmp_tree_path") << L"could not generate temporal "
				<< L"tree path feature: null path to root";*/
		}
	} else {
		SessionLogger::warn("no_tmp_tree_path") << L"could not generate temporal "
			<< L"tree path feature: null arg or tmp node";
	}

	return ret;
}

std::vector<const SynNode*> TemporalTreePathFeatureProposer::pathToRoot(const SynNode * node)
{
	std::vector<const SynNode*> ret;
	const SynNode* n = node->getParent();

	while (n) {
		ret.push_back(n);
		n = n->getParent();
	}

	return ret;
}

const SynNode* TemporalTreePathFeatureProposer::synNodeForArg(const SentenceTheory* st,
		ElfRelationArg_ptr arg) 
{
	ElfIndividual_ptr individual = arg->get_individual();

	if (individual && individual->has_mention_uid()) {
		MentionUID uid = individual->get_mention_uid();
		const MentionSet* ms = st->getMentionSet();
		const Mention* m = ms->getMention(uid);
		if (m) {
			return m->getNode();
		}
	}
	return 0;
}

const SynNode* TemporalTreePathFeatureProposer::synNodeForVM(const SentenceTheory* st,
		const ValueMention* vm)
{
	// this is somewhat hackishly defined to be the first SynNode found in
	// a pre-order traversal which is contained entirely in the bound of the 
	// value mention
	
	return synNodeForVM(st->getPrimaryParse()->getRoot(), vm->getStartToken(),
			vm->getEndToken());
}

const SynNode* TemporalTreePathFeatureProposer::synNodeForVM(
		const SynNode* node, int start_tok, int end_tok) 
{
	if (node->getStartToken() >= start_tok &&
			node->getEndToken() <= end_tok)
	{
		return node;
	}

	// sanity checks for efficiency
	if (node->getEndToken() >= start_tok && node->getStartToken() <= end_tok) {
		for (int i=0; i<node->getNChildren(); ++i) {
			const SynNode* s = synNodeForVM(node->getChild(i), start_tok, end_tok);
			if (s) {
				return s;
			}
		}
	}
	return 0;
}


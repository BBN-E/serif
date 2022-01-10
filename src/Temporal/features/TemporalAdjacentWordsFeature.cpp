#include "Generic/common/leak_detection.h"
#include "TemporalAdjacentWordsFeature.h"
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueMention.h"
#include "PredFinder/elf/ElfRelation.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"

using std::wstring;
using std::wstringstream;
using std::make_pair;
using boost::dynamic_pointer_cast;
using boost::make_shared;

TemporalAdjacentWordsFeature::TemporalAdjacentWordsFeature(
		const std::vector<Symbol>& words, bool preceding,
		const Symbol& relation)
	: TemporalFeature(L"Adjacent", relation), _words(words), _preceding(preceding)
{
	if (_words.empty()) {
		throw UnexpectedInputException(
			"TemporalAdjacentWordsFeature::TemporalAdjacentWordsFeature",
			"Adjacent words vector cannot be empty");
	}
}

TemporalAdjacentWordsFeature_ptr TemporalAdjacentWordsFeature::copyWithRelation(
		const Symbol& relation) 
{
	return make_shared<TemporalAdjacentWordsFeature>(_words, _preceding, relation);
}


TemporalAdjacentWordsFeature_ptr TemporalAdjacentWordsFeature::create(
	const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (parts.size() < 3) {
		throw UnexpectedInputException("TemporalAdjacentWordsFeature",
			"Metadata wrong size");
	}

	bool preceding = false;
	std::vector<Symbol> words;

	if (parts[0] == L"0") {
		preceding = false;
	} else if (parts[0] == L"1") {
		preceding = true;
	} else {
		throw UnexpectedInputException("TemporalAdjacentWordsFeature::create",
				"Expected 0 or 1 in first metadata field");
	}

	unsigned int n_parts = boost::lexical_cast<unsigned int>(parts[1]);

	if (n_parts > 0) {
		if (parts.size() != (2 + n_parts)) {
			throw UnexpectedInputException("TemporalAdjacentWordsFeature::create",
					"Number of remaining metadata parts does not match what is expected.");
		} else {
			for (size_t i=2; i<parts.size(); ++i) {
				words.push_back(parts[i]);
			}
		}
	} else {
		throw UnexpectedInputException("TemporalAdjacentWordsFeature::create",
				"Number of words must be at least 1");
	}

	return boost::make_shared<TemporalAdjacentWordsFeature>(
									words, preceding, relation);
}

std::wstring TemporalAdjacentWordsFeature::pretty() const {
	wstringstream str;

	str << type() << L"(" << 
		((_preceding)?L"LEFT":L"RIGHT") ;
	
	BOOST_FOREACH(const Symbol& word, _words) {
		str << L", " << word.to_string();
	}

	if (!relation().is_null()) {
		str << L", " << relation().to_string();
	}

	str << L")";

	return str.str();
}

std::wstring TemporalAdjacentWordsFeature::dump() const {
	return pretty();
}

std::wstring TemporalAdjacentWordsFeature::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << ((_preceding)?1:0) << L"\t" << _words.size();
	
	BOOST_FOREACH(const Symbol& word, _words) {
		str << L'\t' << word.to_string();
	}

	return str.str();
}

bool TemporalAdjacentWordsFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (TemporalAdjacentWordsFeature_ptr tawf = 
				dynamic_pointer_cast<TemporalAdjacentWordsFeature>(other)) {
			return tawf->_words == _words && tawf->_preceding == _preceding;
		}
	}
	return false;
}

size_t TemporalAdjacentWordsFeature::calcHash() const {
	return  wordsHash(topHash());
}

size_t TemporalAdjacentWordsFeature::wordsHash(size_t start) const {
	size_t ret = start;
	boost::hash_combine(ret, _preceding);
	boost::hash_combine(ret, _words.size());
	BOOST_FOREACH(const Symbol& word, _words) {
		boost::hash_combine(ret, word.to_string());
	}
	return ret;
}

bool TemporalAdjacentWordsFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!TemporalFeature::relationNameMatches(inst)) {
		return false;
	}

	const SentenceTheory* st = dt->getSentenceTheory(sn);
	const TokenSequence* ts = st->getTokenSequence();
	const ValueMention* vm = inst->attribute()->valueMention();

	if (vm) {
		if (_preceding) {
			int start_tok = vm->getStartToken();
			if ((size_t)start_tok >= _words.size()) {
				for (size_t i=0; i<_words.size(); ++i) {
					if (_words[i] != ts->getToken(start_tok - i - 1)->getSymbol()) {
						return false;
					}
				}
				return true;
			}
		} else {
			int end_tok = vm->getEndToken();
			if ((end_tok + (int)_words.size()) < ts->getNTokens()) {
				for (size_t i=0; i<_words.size(); ++i) {
					if (_words[i] != ts->getToken(1+end_tok+i)->getSymbol()) {
						return false;
					}
				}
				return true;
			} 
		}
	}

	return false;
}

/*void TemporalAdjacentWordsFeatureProposer::observe(const SentenceTheory* st) {
	const ValueMentionSet* vms = st->getValueMentionSet();

	for (int i=0; i<vms->getNValueMentions(); ++i) {
		const ValueMention* vm = vms->getValueMention(i);
		BOOST_FOREACH(TemporalAdjacentWordsFeature_ptr feat, generate(vm, st)) {
			size_t hsh = feat->wordsHash();
			FeatCounts::iterator probe  = _featureCounts.find(hsh);
			if (probe != _featureCounts.end()) {
				++probe->second;
			} else {
				_featureCounts.insert(make_pair(hsh, 1));
			}
		}
	}
}*/

void TemporalAdjacentWordsFeatureProposer::addApplicableFeatures(
		const TemporalInstance& inst, const SentenceTheory* st,
		std::vector<TemporalFeature_ptr>& fv) const
{
	const ValueMention* vm = inst.attribute()->valueMention();
	if (vm) {
		std::vector<TemporalAdjacentWordsFeature_ptr> feats = generate(vm, st);
		BOOST_FOREACH(TemporalAdjacentWordsFeature_ptr feat, feats) {
			/*size_t hsh = feat->wordsHash();
			FeatCounts::const_iterator probe = _featureCounts.find(hsh);
			if (probe!= _featureCounts.end() && probe->second > 2) {*/
				fv.push_back(feat);
				fv.push_back(feat->copyWithRelation(inst.relation()->get_name()));
			//}
		}
	}
}


std::vector<TemporalAdjacentWordsFeature_ptr> 
TemporalAdjacentWordsFeatureProposer::generate(const ValueMention* vm,
		const SentenceTheory* st)
{
	std::vector<TemporalAdjacentWordsFeature_ptr> ret;
	const TokenSequence* ts = st->getTokenSequence();

	if (vm) {
		int start_tok = vm->getStartToken();
		int end_tok = vm->getEndToken();

		std::vector<Symbol> syms;
		for (int i=0; i<PROPOSAL_LIMIT && start_tok - 1 - i >=0; ++i) {
			int tok_idx = start_tok -1 -i;
			syms.push_back(ts->getToken(tok_idx)->getSymbol());
			ret.push_back(make_shared<TemporalAdjacentWordsFeature>(syms, true));
		}

		syms.clear();

		for (int i=0; i<PROPOSAL_LIMIT && end_tok + i + 1 < ts->getNTokens(); ++i) {
			int tok_idx = end_tok +i +1;
			syms.push_back(ts->getToken(tok_idx)->getSymbol());
			ret.push_back(make_shared<TemporalAdjacentWordsFeature>(syms, false));
		}
	}
	return ret;
}


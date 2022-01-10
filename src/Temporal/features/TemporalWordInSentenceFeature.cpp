#include "Generic/common/leak_detection.h"
#include "TemporalWordInSentenceFeature.h"
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "English/parse/en_STags.h"
#include "Temporal/TemporalInstance.h"

using std::wstring;
using std::wstringstream;
using boost::dynamic_pointer_cast;

TemporalWordInSentenceFeature::TemporalWordInSentenceFeature(const Symbol& word, 
		const Symbol& relation)
	: TemporalFeature(L"WordInSentence", relation), _word(word)
{
	if (_word.is_null()) {
		throw UnexpectedInputException(
			"TemporalWordInSentenceFeature::TemporalWordInSentenceFeature",
			"Word cannot be Symbol()");
	}
}

TemporalWordInSentenceFeature_ptr TemporalWordInSentenceFeature::create(
	const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (parts.size() != 1) {
		throw UnexpectedInputException("TemporalWordInSentenceFeature",
			"Metadata wrong size");
	}

	return boost::make_shared<TemporalWordInSentenceFeature>(parts[0], relation);
}

std::wstring TemporalWordInSentenceFeature::pretty() const {
	wstringstream str;

	str << type () << L"(" << _word.to_string();

	if (!relation().is_null()) {
		str << L", " << relation().to_string();
	}

	str << L")";

	return str.str();
}

std::wstring TemporalWordInSentenceFeature::dump() const {
	return pretty();
}

std::wstring TemporalWordInSentenceFeature::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << _word;
	return str.str();
}

bool TemporalWordInSentenceFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (TemporalWordInSentenceFeature_ptr twisf = 
				dynamic_pointer_cast<TemporalWordInSentenceFeature>(other)) {
			return twisf->_word == _word;
		}
	}
	return false;
}

size_t TemporalWordInSentenceFeature::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _word.to_string());
	return ret;
}
			
bool TemporalWordInSentenceFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	const SentenceTheory* st = dt->getSentenceTheory(sn);
	const SynNode* node = st->getPrimaryParse()->getRoot();

	return TemporalFeature::relationNameMatches(inst) && matches(node);
}

bool TemporalWordInSentenceFeature::matches(const SynNode* node) const {
	if (node->isPreterminal()) {
		Symbol pos = node->getTag();
		Symbol tok = node->getHeadWord();

		if (isWordFeatureTriggerPOS(pos) && tok == _word) {
			return true;
		}
	} else {
		for (int i=0; i<node->getNChildren(); ++i) {
			if (matches(node->getChild(i))) {
				return true;
			}
		}
	}
	return false;
}

bool TemporalWordInSentenceFeature::isWordFeatureTriggerPOS(const Symbol& pos) {
	return (pos ==  EnglishSTags::MD || pos == EnglishSTags::RB || pos == EnglishSTags::RP
				|| pos == EnglishSTags::VB || pos == EnglishSTags::VBD || pos == EnglishSTags::VBG
				|| pos == EnglishSTags::VBN || pos == EnglishSTags::VBP || pos == EnglishSTags::VBP
				|| pos == EnglishSTags::VBZ || pos == EnglishSTags::VP);
}

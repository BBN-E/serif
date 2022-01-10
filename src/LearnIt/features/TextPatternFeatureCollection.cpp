#pragma warning( disable: 4996 )
#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include <map>
#include <utility>
#include <set>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/common/foreach_pair.hpp"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/features/TextPatternFeatureCollection.h"
#include "LearnIt/features/BrandyPatternFeature.h"

using std::wstring; using std::vector; using std::make_pair; using std::set;
using boost::make_shared;
using boost::split; using boost::is_any_of;
using boost::match_results; using boost::regex_search; using boost::wregex;

TextPatternFeatureCollection::TextPatternFeatureCollection(
	Target_ptr target, const std::vector<BrandyPatternFeature_ptr>& features,
	const wstring& search_pattern) :
_features(features.begin(), features.end())
{
	wregex regex_pattern(search_pattern);
	wstring::const_iterator start, end;
	match_results<wstring::const_iterator> what;
	vector<wstring> words;

	int feature_idx = 0;
	BOOST_FOREACH(BrandyPatternFeature_ptr feature, features) {
		words.clear();
		start = feature->patternString().begin();
		end = feature->patternString().end();
		
		while(regex_search(start, end, what, regex_pattern)) {
            std::wstring wstr(what[1].first, what[1].second);
			split(words, wstr, is_any_of(L" "));
			start = what[0].second;
		}
		if (words.empty()) {
			SessionLogger::info("ALWAYS_USE_REGEX_PATTERN") << "Cannot canopy "
				<< "the pattern with the following BPL code, so we will always apply "
				<< "it: " <<  feature->patternString();
			_alwaysUse.push_back(feature_idx);
		}
		BOOST_FOREACH(const wstring& word, words) {
			_wordsToPatterns.insert(make_pair(Symbol(word), feature_idx));
		}
		_votes_required.push_back(static_cast<int>(words.size()));
		++feature_idx;
	}

	if (_alwaysUse.size() > 0) {
		SessionLogger::warn("ALWAYS_USE_REGEX_PATTERN") 
			<< "For TextPatternFeatureCollection, always applying " 
			<< _alwaysUse.size() << " features because we can't canopy them.";
	}
}

TextPatternFeatureCollection_ptr 
TextPatternFeatureCollection::create(const wstring& featureClass, Target_ptr target, 
			FeatureAlphabet_ptr alphabet, double threshold,
			bool include_negatives, const wstring& regex) 
{
	FeatureList_ptr tmpFeatures = Feature::featuresOfClass(alphabet, 
			featureClass, target, threshold, include_negatives);
	std::vector<BrandyPatternFeature_ptr> features;
	BOOST_FOREACH(const Feature_ptr& f, *tmpFeatures) {
		BrandyPatternFeature_ptr bpfp = boost::dynamic_pointer_cast<BrandyPatternFeature>(f);
		if (bpfp) {
			features.push_back(bpfp);
		} else {
			throw UnrecoverableException("TextPatternFeatureCollection",
				"Internal inconsistency: features do not result in objects "
				"of the right type");
		}
	}
	return make_shared<TextPatternFeatureCollection>(target, features, regex);
}

TextPatternFeatureCollection_ptr
TextPatternFeatureCollection::createFromKeywordInSentencePatterns(Target_ptr target,
			  FeatureAlphabet_ptr alphabet, double threshold, 
			  bool include_negatives) 
{
	return create(L"KeywordInSentenceFeature", target, alphabet, threshold, 
		include_negatives, L"\\(text \\(string \"([^)]*)\"\\)\\)");
}

TextPatternFeatureCollection_ptr
TextPatternFeatureCollection::createFromTextPatterns(Target_ptr target,
			  FeatureAlphabet_ptr alphabet, double threshold, 
			  bool include_negatives) 
{
	return create(L"TextPatternFeature", target, alphabet, threshold, include_negatives,
			L"\\(text RAW_TEXT \\(string \"([^)]*)\"\\)\\)");
}

FeatureCollectionIterator<BrandyPatternFeature>
TextPatternFeatureCollection::applicableFeatures(const SlotFillersVector& slotFillersVector,
	   const DocTheory* doc, int sent_no)
{
	_applicableFeatures.clear();
	_votes.clear();
	TokenSequence* toks = doc->getSentenceTheory(sent_no)->getTokenSequence();
	set<Symbol> syms;

	for (int i=0; i<toks->getNTokens(); ++i) {
		syms.insert(Symbol(MainUtilities::normalizeString(
			toks->getToken(i)->getSymbol().to_string())));
	}

	BOOST_FOREACH(const Symbol& sym, syms) {
		SymbolCanopy::Features possible_patterns = _wordsToPatterns.equal_range(sym);
		for (SymbolCanopy::KeyToFeatures::const_iterator it = possible_patterns.first;
			it!=possible_patterns.second; ++it) 
		{
			FeatureVotes::iterator probe = _votes.find(it->second);
			if (probe != _votes.end()) {
				probe->second += 1;
			} else {
				_votes.insert(make_pair(it->second, 1));
			}
		}
	}		
		
	BOOST_FOREACH_PAIR(int feature, int votes, _votes) {
		if (votes >= _votes_required[feature]) {
			_applicableFeatures.insert(feature);
		}
	}

	_applicableFeatures.insert(_alwaysUse.begin(), _alwaysUse.end());		

	return FeatureCollectionIterator<BrandyPatternFeature>
		(_features, _applicableFeatures, true);
}

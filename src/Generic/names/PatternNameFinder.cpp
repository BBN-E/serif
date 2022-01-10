// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/names/PatternNameFinder.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/common/Sexp.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

PatternNameFinder::PatternNameFinder() : _docTheory(0), _sentence_number(-1) {

	std::string pattern_set_list = ParamReader::getRequiredParam("name_pattern_set_list");
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(pattern_set_list));
	UTF8InputStream& stream(*stream_scoped_ptr);
	std::wstring line;
	while (stream) {
		stream.getLine(line);
		if (!stream) 
			break;
		if (line.size()>0 && line[0]!=L'#') {
			std::string s = UnicodeUtil::toUTF8StdString(line);
			s = ParamReader::expand(s);
			PatternSet_ptr ps = boost::make_shared<PatternSet>(s.c_str());
			_patternSets.push_back(ps);
		}
	}
	stream.close();

	BOOST_FOREACH(PatternSet_ptr ps, _patternSets) 
		if (!ps->validForPreParseMatching()) {
			throw UnexpectedInputException("PatternNameFinder::PatternNameFinder", "Only regex patterns with text pattern children allowed in pattern file");
		}
}

PatternNameFinder::~PatternNameFinder() {}

void PatternNameFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num) {
	_docTheory = docTheory;
	_sentence_number = sentence_num;
}

void PatternNameFinder::augmentPIdFSentence(PIdFSentence &sentence, DTTagSet *tagSet) {

	if (_docTheory == 0)
		throw InternalInconsistencyException("PatternNameFinder::augmentPIdFSentence", "DocTheory not set");

	SentenceTheory *sTheory = _docTheory->getSentenceTheory(_sentence_number);

	std::vector<PatternFeatureSet_ptr> all_results;
	BOOST_FOREACH(PatternSet_ptr ps, _patternSets) {
		PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(_docTheory, ps);
		std::vector<PatternFeatureSet_ptr> results = pm->getSentenceSnippets(sTheory, 0, true);
		all_results.insert(all_results.end(), results.begin(), results.end());
	}

	for (size_t h = 0; h < all_results.size(); h++) {
		PatternFeatureSet_ptr pfs = all_results[h];
		for (size_t f = 0; f < pfs->getNFeatures(); f++) {
			PatternFeature_ptr feat = pfs->getFeature(f);
			if (ReturnPatternFeature_ptr rf = boost::dynamic_pointer_cast<ReturnPatternFeature>(feat)) {
				Symbol entityTypeSym = rf->getReturnValue(L"type");
				bool force = (rf->getReturnValue(L"force") == L"YES");
			
				if (!EntityType::isValidEntityType(entityTypeSym) && entityTypeSym != Symbol(L"NONE"))
					throw UnexpectedInputException("PatternNameFinder::augmentPIdFSentence", "Bad entity type in pattern file");

				if (TokenSpanReturnPFeature_ptr tsrf = boost::dynamic_pointer_cast<TokenSpanReturnPFeature>(rf)) {
					int start_token = tsrf->getStartToken();
					int end_token = tsrf->getEndToken();

					// only coerce entity type if PIdF didn't find another type in span
					bool found_different_type = false;
					for (int i = start_token; i <= end_token; i++) {
						int tag = sentence.getTag(i);
						Symbol reducedTag = tagSet->getReducedTagSymbol(tag);
						if (reducedTag != tagSet->getNoneTag() && reducedTag != entityTypeSym) {
							found_different_type = true;
							break;
						}
					}

					if (!found_different_type || force) {
						// coerce
						std::wstring stTag = entityTypeSym.to_string();
						stTag += L"-ST";

						std::wstring coTag = entityTypeSym.to_string();
						coTag += L"-CO";
						
						if (tagSet->getTagIndex(Symbol(coTag)) == -1 ||
							tagSet->getTagIndex(Symbol(stTag)) == -1) 
						{
							throw UnexpectedInputException("PatternNameFinder::augmentPIdFSentence",
								"Could not find tag index for entity type. All types need to be added to pidf_tag_set_file.");
						}

						if (sentence.getTag(start_token) != tagSet->getTagIndex(Symbol(coTag)))
							sentence.setTag(start_token, tagSet->getTagIndex(Symbol(stTag)));

						for (int i = start_token + 1; i <= end_token; i++) 
							sentence.setTag(i, tagSet->getTagIndex(Symbol(coTag)));

						if (end_token + 1 < sentence.getLength() && sentence.getTag(end_token + 1) != tagSet->getTagIndex(Symbol(coTag))) 
							sentence.changeToStartTag(end_token + 1);
					}

				} 
			}
		}
	}
}

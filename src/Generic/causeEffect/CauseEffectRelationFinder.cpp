// Copyright 2017 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/causeEffect/CauseEffectRelationFinder.h"
#include "Generic/causeEffect/CauseEffectRelation.h"

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

namespace {
	Symbol CAUSE(L"CAUSE");
	Symbol EFFECT(L"EFFECT");
}

CauseEffectRelationFinder::CauseEffectRelationFinder() { 
	_do_cause_effect_relation_finding = ParamReader::isParamTrue("do_cause_effect_relation_finding");

	if (_do_cause_effect_relation_finding) {
		_causeEffectPatternSets = loadPatternSets(ParamReader::getRequiredParam("cause_effect_pattern_set_list"));
		_outputDir = ParamReader::getRequiredParam("cause_effect_relation_output_dir");
	}
}

CauseEffectRelationFinder::~CauseEffectRelationFinder() { }

std::vector<CauseEffectRelationFinder::PatternSetAndRelationType> CauseEffectRelationFinder::loadPatternSets(const std::string &patternSetFileList) {

	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(patternSetFileList.c_str()));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if( (!stream.is_open()) || (stream.fail()) ){
		std::stringstream errmsg;
		errmsg << "Failed to open cause_effect_pattern_set_list file \"" << patternSetFileList << "\"" << " specified by parameter cause_effect_pattern_set_list";
		throw UnexpectedInputException("CauseEffectRelationFinder::loadPatternSets()", errmsg.str().c_str() );
	}

	std::vector<CauseEffectRelationFinder::PatternSetAndRelationType> patternSetAndRelationType;

	while (!(stream.eof() || stream.fail())) {
		std::wstring wline;
		std::getline(stream, wline);
		wline = wline.substr(0, wline.find_first_of(L'#')); // Remove comments.
		boost::trim(wline);
		std::string line = UnicodeUtil::toUTF8StdString(wline);

		if (!line.empty()) {
			// Expand parameter variables.
			try {
				line = ParamReader::expand(line);
			} catch (UnexpectedInputException &e) {
				std::ostringstream prefix;
				prefix << "while processing cause_effect_pattern_set_list file " << patternSetFileList << ": ";
				e.prependToMessage(prefix.str().c_str());
				throw;
			}

			// Parse the line
			std::string::size_type splitpos = line.find_first_of(":");
			if (splitpos == std::string::npos)
				throw UnexpectedInputException("CauseEffectRelationFinder::loadPatternSets",
					"Parse error: expected each line to contain 'code:filename'");
			std::string relationType = line.substr(0, splitpos);
			std::string filename = line.substr(splitpos+1);
			boost::trim(relationType);
			boost::trim(filename);
			if (relationType.empty() || filename.empty())
				throw UnexpectedInputException("CauseEffectRelationFinder::loadPatternSets",
					"Parse error: expected each line to contain 'relation_type:filename'");
			std::wstring relationTypeWStr(relationType.begin(), relationType.end());
			Symbol relationTypeSym(relationTypeWStr); // default relation type for this pattern set.
			// Normalize filename
			boost::algorithm::replace_all(filename, "/", SERIF_PATH_SEP);
			boost::algorithm::replace_all(filename, "\\", SERIF_PATH_SEP);
			// Load the pattern set, and add it to our list of pattern sets.
			try {
				PatternSet_ptr patternSet = boost::make_shared<PatternSet>(filename.c_str());
				patternSetAndRelationType.push_back(PatternSetAndRelationType(patternSet, relationTypeSym));
			} catch (UnrecoverableException &exc) {
				std::ostringstream prefix;
				prefix << "While processing \"" << filename << "\": ";
				exc.prependToMessage(prefix.str().c_str());
				throw;
			}
		}
	}
	return patternSetAndRelationType;
}

void CauseEffectRelationFinder::findCauseEffectRelations(DocTheory *docTheory) {
	if (!_do_cause_effect_relation_finding)
		return;

	UTF8OutputStream out;
	boost::filesystem::path outputDir(_outputDir);
	boost::filesystem::create_directory(outputDir);
	boost::filesystem::path fileName(UnicodeUtil::toUTF8StdString(docTheory->getDocument()->getName().to_string()) + ".json");
	boost::filesystem::path	filePath = outputDir / fileName;
	out.open(filePath.string().c_str());

	std::vector<CauseEffectRelation_ptr> results;
	BOOST_FOREACH(PatternSetAndRelationType pair, _causeEffectPatternSets) {

		PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, pair.first);

		for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
			SentenceTheory *sentTheory = patternMatcher->getDocTheory()->getSentenceTheory(sentno);
			std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
			BOOST_FOREACH(PatternFeatureSet_ptr match, matches) {
				createCauseEffectRelations(match, results, sentTheory, pair.second);
			}
		}
	}

	out << L"[\n";
	BOOST_FOREACH(CauseEffectRelation_ptr cer, results) {
		out << cer->getJsonString(docTheory->getDocument()->getName());
		if (cer != results.at(results.size() - 1))
			out << L",";
		out << L"\n";
	}
	out << L"]\n";

	out.close();
}

void CauseEffectRelationFinder::createCauseEffectRelations(
	PatternFeatureSet_ptr match, 
	std::vector<CauseEffectRelation_ptr> &results, 
	const SentenceTheory* sentTheory, 
	Symbol relationType) 
{
	Symbol patternLabel = Symbol();
	std::vector<const Proposition *> causes;
	std::vector<const Proposition *> effects;

	for (size_t f = 0; f < match->getNFeatures(); f++) {
		PatternFeature_ptr sf = match->getFeature(f);
		if (TopLevelPFeature_ptr tsf = boost::dynamic_pointer_cast<TopLevelPFeature>(sf)) {
			patternLabel = tsf->getPatternLabel();
		} else if (PropositionReturnPFeature_ptr prf = boost::dynamic_pointer_cast<PropositionReturnPFeature>(sf)) {
			Symbol returnLabel = prf->getReturnLabel();
			if (returnLabel == CAUSE)
				causes.push_back(prf->getProp());
			else if (returnLabel == EFFECT)
				effects.push_back(prf->getProp());
			else
				throw UnexpectedInputException("CauseEffectRelationFinder::findCauseEffectRelations", "Return label not CAUSE or EFFECT");
		}
	}

	if (causes.size() > 0 && effects.size() > 0) {
		BOOST_FOREACH(const Proposition *cause, causes) {
			BOOST_FOREACH(const Proposition *effect, effects) {
				if (cause->getPredHead() == 0 || effect->getPredHead() == 0)
					continue;
				CauseEffectRelation_ptr cer = boost::make_shared<CauseEffectRelation>(cause, effect, sentTheory, relationType);
				results.push_back(cer);
			}
		}
	} else {
		SessionLogger::info("CauseEffect") << "Top level pattern match missing CAUSE and/or EFFECT" << std::endl;
	}
}

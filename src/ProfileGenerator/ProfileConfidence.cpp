// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/UnicodeUtil.h"

#include "ProfileGenerator/ProfileConfidence.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

ProfileConfidence::ProfileConfidence() {
	// Read in weights model file
	_weights = readFeatureWeights(ParamReader::getRequiredParam("profile_slot_confidence_weights"));

	// Read in feature correctness model file
	_correctness = readFeatureCorrectness(ParamReader::getRequiredParam("profile_slot_confidence_correctness"));

	// Read in confidence thresholds model file, if any
	std::string threshold_filename = ParamReader::getParam("profile_slot_confidence_thresholds");
	if (!threshold_filename.empty())
		_thresholds = readSlotThresholds(threshold_filename);

	// Optional parameter that specifies a debug log file
	std::string debug_log_filename = ParamReader::getParam("confidence_debug_log");
	if (!debug_log_filename.empty())
		_debug_log = UTF8OutputStream::build(debug_log_filename, false);
	else
		_debug_log = NULL;
}

ProfileConfidence::~ProfileConfidence() {
	if (_debug_log) {
		_debug_log->close();
		delete _debug_log;
	}
}

void ProfileConfidence::setConfidencesForSlot(std::string slot_type, ProfileSlot_ptr slot) {

	// Currently deprecated
	return;

	// Calculate a confidence for each of this slot's output hypotheses
	if (_debug_log)
		*_debug_log << L"\nslot " << slot->getDisplayName() << L"\n";
	std::vector<double> confidences;
	std::list<GenericHypothesis_ptr>& hypotheses = slot->getOutputHypotheses();
	BOOST_FOREACH(GenericHypothesis_ptr hypothesis, hypotheses) {
		// Track the maximum confidence of the facts in this hypothesis
		double max_confidence = 0.0;

		// Calculate the confidence contribution from each fact
		BOOST_FOREACH(PGFact_ptr fact, hypothesis->getSupportingFacts()) {
			// Accumulate the weighted correctness contributions from each fact feature
			double confidence = 0.0;			
			confidence += getWeightedFactCountCorrectness(slot_type, hypothesis, fact);
			confidence += getWeightedLanguageCorrectness(slot_type, fact);
			confidence += getWeightedPatternCorrectness(slot_type, fact);
			confidence += getWeightedAgentMentionCorrectness(slot_type, fact);
			confidence += getWeightedAnswerMentionCorrectness(slot_type, fact);

			// Escape hatch, go high confidence if this is an English fact with >10 sibling entries
			if (!fact->isMT() && hypothesis->nSupportingFacts() > 10 && confidence < 0.95)
				confidence = 0.95;

			// Update the maximum if necessary
			if (confidence > max_confidence)
				max_confidence = confidence;

			// Optionally log this fact's features and confidence
			if (_debug_log) {
				/* Update for new format later if desired
				*_debug_log << L"    fact " << confidence << L" " << hypothesis->nSupportingFacts() << L" " << !fact->is_mt << L" " << fact->pattern_name << L" " << fact->agent1_mention_conf << L" " << fact->answer_mention_conf << L"\n";
				*_debug_log << L"      literal '" << fact->literal_string_val << L"'\n";
				*_debug_log << L"      resolved '" << fact->resolved_string_val << L"'\n";
				*_debug_log << L"      entity '" << fact->entity_name << L"'\n";
				*/
			}
		}

		// Convert the maximum confidence to a binned confidence, if thresholds are available
		double threshold_confidence = applyConfidenceThreshold(slot_type, max_confidence);

		// Optionally log this hypothesis' text and confidence
		if (_debug_log)
			*_debug_log << L"  hypo " << max_confidence << L" " << threshold_confidence << L" '" << hypothesis->getDisplayValue() << L"'\n\n";

		// Done for this hypothesis; store it!
		hypothesis->setConfidence(threshold_confidence);
	}
}

double ProfileConfidence::getSlotCorrectness(std::string slot_type) {
	// Get the overall correctness for a slot, when we don't have feature-specific correctness
	SlotAndFeature saf = SlotAndFeature(slot_type, L"slot");
	CorrectnessTable::iterator ci = _correctness.find(saf);
	if (ci != _correctness.end())
		return ci->second.second;
	else
		// Either an unmodeled slot type or one of the special values in the enumeration,
		// so make it low confidence
		return 0.5;
}

double ProfileConfidence::getWeightedFactCountCorrectness(std::string slot_type, GenericHypothesis_ptr hypothesis, PGFact_ptr fact) {
	// Determine if the document is English from the document Brandy ID prefix
	bool is_english = !(fact->isMT());

	// Loop through the correctness table for this slot and feature until a match is found
	int nFacts = hypothesis->nSupportingFacts();
	SlotAndFeature saf = SlotAndFeature(slot_type, L"fact count");
	BOOST_FOREACH(CorrectnessTable::value_type correctness, _correctness.equal_range(saf)) {
		// If the language matches and the fact count is in range, return the weighted correctness
		LanguageAndFactCount lafc = boost::get<LanguageAndFactCount>(correctness.second.first);
		if (lafc.first == is_english && nFacts >= lafc.second.first && nFacts <= lafc.second.second)
			return getWeight(saf)*correctness.second.second;
	}

	// Not found, use the overall slot correctness
	return getWeight(saf)*getSlotCorrectness(slot_type);
}

double ProfileConfidence::getWeightedLanguageCorrectness(std::string slot_type, PGFact_ptr fact) {
	// Determine if the document is English from the document Brandy ID prefix
	bool is_english = !(fact->isMT());

	// Loop through the correctness table for this slot and feature until a match is found
	SlotAndFeature saf = SlotAndFeature(slot_type, L"language");
	BOOST_FOREACH(CorrectnessTable::value_type correctness, _correctness.equal_range(saf)) {
		// If the language matches, return the weighted correctness
		if (boost::get<bool>(correctness.second.first) == is_english)
			return getWeight(saf)*correctness.second.second;
	}

	// Not found, use the overall slot correctness
	return getWeight(saf)*getSlotCorrectness(slot_type);
}

double ProfileConfidence::getWeightedPatternCorrectness(std::string slot_type, PGFact_ptr fact) {
	// Loop through the correctness table for this slot and feature until a match is found

	
	SlotAndFeature saf = SlotAndFeature(slot_type, L"pattern");
	
	/*
	// Pattern names are now deprecated
	BOOST_FOREACH(CorrectnessTable::value_type correctness, _correctness.equal_range(saf)) {
		// If the pattern name matches, return the weighted correctness
		if (boost::get<std::string>(correctness.second.first) == fact->pattern_name)
			return getWeight(saf)*correctness.second.second;
	}*/

	// Not found, use the overall slot correctness
	return getWeight(saf)*getSlotCorrectness(slot_type);
}

double ProfileConfidence::getWeightedAgentMentionCorrectness(std::string slot_type, PGFact_ptr fact) {
	// Loop through the correctness table for this slot and feature until a match is found
	SlotAndFeature saf = SlotAndFeature(slot_type, L"agent mention");
	PGFactArgument_ptr agentArg = fact->getAgentArgument();
	BOOST_FOREACH(CorrectnessTable::value_type correctness, _correctness.equal_range(saf)) {
		// If the agent mention confidence matches, return the weighted correctness

		// Not currently working since we updated mention confidences
		//if (boost::get<MentionConfidenceAttribute>(correctness.second.first) == agentArg->getSERIFMentionConfidence())
		//	return getWeight(saf)*correctness.second.second;
	}

	// Not found, use the overall slot correctness
	return getWeight(saf)*getSlotCorrectness(slot_type);
}

double ProfileConfidence::getWeightedAnswerMentionCorrectness(std::string slot_type, PGFact_ptr fact) {
	// Loop through the correctness table for this slot and feature until a match is found
	SlotAndFeature saf = SlotAndFeature(slot_type, L"answer mention");
	//PGFactArgument_ptr answerArg = fact->getAnswerArgument();
	BOOST_FOREACH(CorrectnessTable::value_type correctness, _correctness.equal_range(saf)) {
		// If the agent mention confidence matches, return the weighted correctness
		// Not currently working since we updated mention confidences
		/*
		if (boost::get<MentionConfidenceAttribute>(correctness.second.first) == firstArg->getSERIFMentionConfidence())
			return getWeight(saf)*correctness.second.second;
			*/
	}

	// Not found, use the overall slot correctness
	return getWeight(saf)*getSlotCorrectness(slot_type);
}

double ProfileConfidence::getWeight(SlotAndFeature saf) {
	// If no entry for this combination is available, the weight should be even
	//   Currently five features; maybe make this dependent on feature list
	WeightsTable::iterator wi = _weights.find(saf);
	if (wi != _weights.end())
		return wi->second;
	else
		return 0.2;
}

double ProfileConfidence::applyConfidenceThreshold(std::string slot_type, double confidence) {
	// No-op if no thresholds were specified
	if (_thresholds.size() == 0)
		return confidence;

	// Loop through the thresholds table for this slot until a match is found
	BOOST_FOREACH(ThresholdsTable::value_type threshold, _thresholds.equal_range(slot_type)) {
		// If the confidence is in the range (inclusive), return the output value for this threshold
		if (threshold.second.first.first <= confidence && confidence <= threshold.second.first.second)
			return threshold.second.second;
	}

	// Not found, use global threshold (if any)
	BOOST_FOREACH(ThresholdsTable::value_type threshold, _thresholds.equal_range(L"ALL")) {
		// If the confidence is in the range (inclusive), return the output value for this threshold
		if (threshold.second.first.first <= confidence && confidence <= threshold.second.first.second)
			return threshold.second.second;
	}
	
	// Not found, leave as-is
	return confidence;
}

WeightsTable ProfileConfidence::readFeatureWeights(std::string filename) {
	// Read the file into columns and rows
	std::set< std::vector<std::wstring> > rows = InputUtil::readColumnFileIntoSet(filename, false, L"\t");

	// Store the confidence model weights by slot type and feature name
	WeightsTable weights;
	BOOST_FOREACH(std::vector<std::wstring> row, rows) {
		// Convert the columns from the file
		std::string slot_type = UnicodeUtil::toUTF8StdString(row[0]);
		std::wstring feature = row[1];
		double weight = boost::lexical_cast<double>(row[2]);

		// Add this feature weight to the map
		weights.insert(WeightsTable::value_type(SlotAndFeature(slot_type, feature), weight));
	}
	return weights;
}

CorrectnessTable ProfileConfidence::readFeatureCorrectness(std::string filename) {
	// Read the file into columns and rows
	std::set< std::vector<std::wstring> > rows = InputUtil::readColumnFileIntoSet(filename, false, L"\t");

	// Store the confidence model weights by slot type and feature name
	CorrectnessTable correctness;
	BOOST_FOREACH(std::vector<std::wstring> row, rows) {
		// Convert the columns from the file
		std::string slot_type = UnicodeUtil::toUTF8StdString(row[0]);
		std::wstring feature = row[1];
		double weight = 0.0;
		
		// Fact count range has more columns, slot has fewer
		if (feature == L"fact count")
			weight = boost::lexical_cast<double>(row[4]);
		else if (feature == L"slot")
			weight = boost::lexical_cast<double>(row[2]);
		else
			weight = boost::lexical_cast<double>(row[3]);			

		// Convert feature values as necessary
		FeatureValue feature_value = false;
		if (feature == L"fact count") {
			// Parse the Boolean language constraint
			bool is_english = boost::iequals(row[2], L"true");

			// Parse the pair of integer fact count bounds
			std::vector<std::wstring> fact_range_strings;
			boost::split(fact_range_strings, row[3], boost::is_space());
			FactCountRange fact_range;
			fact_range.first = boost::lexical_cast<int>(fact_range_strings[0]);
			fact_range.second = boost::lexical_cast<int>(fact_range_strings[1]);

			// Handle no bound with infinities (to make later range check work)
			if (fact_range.first == -1)
				fact_range.first = -std::numeric_limits<int>::max();
			if (fact_range.second == -1)
				fact_range.second = std::numeric_limits<int>::max();

			// Store the language/range pair (this is the most complicated feature)
			feature_value = FeatureValue(LanguageAndFactCount(is_english, fact_range));
		} else if (feature == L"language") {
			// Parse the Boolean language constraint
			bool is_english = boost::iequals(row[2], L"true");
			feature_value = FeatureValue(is_english);
		} else if (feature == L"pattern") {
			// Convert pattern name to match Fact member variable
			std::string pattern_name = UnicodeUtil::toUTF8StdString(row[2]);
			feature_value = FeatureValue(pattern_name);
		} else if (feature == L"agent mention" || feature == L"answer mention") {
			// Cast to MentionConfidence enumeration
			//MentionConfidenceAttribute mention_confidence =  MentionConfidenceAttribute::getFromString(row[2].c_str());
			//feature_value = FeatureValue(mention_confidence);
			continue; // until we update with new mention confidences, this won't work
		}

		// Add this feature correctness to the map
		correctness.insert(CorrectnessTable::value_type(SlotAndFeature(slot_type, feature), FeatureValueAndWeight(feature_value, weight)));
	}
	return correctness;
}

ThresholdsTable ProfileConfidence::readSlotThresholds(std::string filename) {
	// Read the file into columns and rows
	std::set< std::vector<std::wstring> > rows = InputUtil::readColumnFileIntoSet(filename, false, L"\t");

	// Store the confidence model output thresholds and value by slot type
	ThresholdsTable thresholds;
	BOOST_FOREACH(std::vector<std::wstring> row, rows) {
		// Convert the columns from the file
		ConfidenceThresholdSlot slot_type;
		if (row[0] == L"ALL")
			slot_type = ConfidenceThresholdSlot(row[0]);
		else
			slot_type = ConfidenceThresholdSlot(row[0]);
		double range_start = boost::lexical_cast<double>(row[1]);
		double range_end = boost::lexical_cast<double>(row[2]);
		double output_value = boost::lexical_cast<double>(row[3]);

		// Add this threshold to the map
		thresholds.insert(ThresholdsTable::value_type(slot_type, ConfidenceRangeAndValue(ConfidenceRange(range_start, range_end), output_value)));
	}
	return thresholds;
}

std::ostream& std::operator<<(std::ostream& out, const FactCountRange& lafc) {
	out << "[" << lafc.first << ", " << lafc.second << "]";
	return out;
}

std::ostream& std::operator<<(std::ostream& out, const LanguageAndFactCount& lafc) {
	out << lafc.first << " " << lafc.second;
	return out;
}

std::ostream& std::operator<<(std::ostream& out, const FeatureValueAndWeight& fvaw) {
	out << fvaw.first << " " << fvaw.second;
	return out;
}

std::ostream& std::operator<<(std::ostream& out, const SlotAndFeature& saf) {
	out << saf.first << " " << UnicodeUtil::toUTF8StdString(saf.second);
	return out;
}

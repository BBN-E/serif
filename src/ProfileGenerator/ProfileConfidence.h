// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROFILE_CONFIDENCE_H
#define PROFILE_CONFIDENCE_H

#include <map>
#include <string>

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/theories/MentionConfidence.h"

#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"

#include <boost/variant.hpp>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(ProfileConfidence);

typedef std::pair<std::string, std::wstring> SlotAndFeature;
typedef std::map<SlotAndFeature, double> WeightsTable;
typedef std::pair<int, int> FactCountRange;
typedef std::pair<bool, FactCountRange> LanguageAndFactCount;
typedef boost::variant<LanguageAndFactCount, bool, std::string, int> FeatureValue;
typedef std::pair<FeatureValue, double> FeatureValueAndWeight;
typedef std::multimap<SlotAndFeature, FeatureValueAndWeight> CorrectnessTable;
typedef std::pair<double, double> ConfidenceRange;
typedef std::pair<ConfidenceRange, double> ConfidenceRangeAndValue;
typedef boost::variant<std::wstring, std::string> ConfidenceThresholdSlot;
typedef std::multimap<ConfidenceThresholdSlot, ConfidenceRangeAndValue> ThresholdsTable;

class ProfileConfidence {
public:
	ProfileConfidence();
	~ProfileConfidence();

	void setConfidencesForSlot(std::string slot_type, ProfileSlot_ptr slot);

protected:
	
	double getSlotCorrectness(std::string slot_type);
	double getWeightedFactCountCorrectness(std::string slot_type, GenericHypothesis_ptr hypothesis, PGFact_ptr fact);
	double getWeightedLanguageCorrectness(std::string slot_type, PGFact_ptr fact);
	double getWeightedPatternCorrectness(std::string slot_type, PGFact_ptr fact);
	double getWeightedAgentMentionCorrectness(std::string slot_type, PGFact_ptr fact);
	double getWeightedAnswerMentionCorrectness(std::string slot_type, PGFact_ptr fact);

	double getWeight(SlotAndFeature saf);

	double applyConfidenceThreshold(std::string slot_type, double confidence);

	static WeightsTable readFeatureWeights(std::string filename);

	static CorrectnessTable readFeatureCorrectness(std::string filename);

	static ThresholdsTable readSlotThresholds(std::string filename);

private:
	WeightsTable _weights;

	CorrectnessTable _correctness;

	ThresholdsTable _thresholds;

	UTF8OutputStream* _debug_log;
};

// Have to declare these in std otherwise template lookup fails; the boost mailing list said so
namespace std {
	// Stream operators useful for debugging/logging
	std::ostream& operator<<(std::ostream& out, const FactCountRange& lafc);
	std::ostream& operator<<(std::ostream& out, const LanguageAndFactCount& lafc);
	std::ostream& operator<<(std::ostream& out, const FeatureValueAndWeight& fvaw);
	std::ostream& operator<<(std::ostream& out, const SlotAndFeature& saf);
}

#endif

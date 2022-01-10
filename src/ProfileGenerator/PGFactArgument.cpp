// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ProfileGenerator/PGFactArgument.h"
#include "boost/foreach.hpp"


bool PGFactArgument::isReliable() { 

	// Note that this applies to both external facts and SERIF facts now Right now this only applies to external facts (e.g. DBPedia)
	// Eventually it would apply to other extractors as well, so we should be careful
	// Note that _confidence is /definitely/ unreliable for SERIF, so we shouldn't use it
	//  if _serif_mention_confidence is set.
	if (_serif_mention_confidence == MentionConfidenceStatus::UNKNOWN_CONFIDENCE)
		return _confidence > 0.59;

	// This is essentially a hold-over from days past, when we just used SERIF for ProfileGenerator and
	//   could rely on it. Now we just map SERIF confidences to float values upon upload, which
	//   is taken care of above.
	return (_serif_mention_confidence == MentionConfidenceStatus::ANY_NAME ||
		_serif_mention_confidence == MentionConfidenceStatus::COPULA_DESC ||
		_serif_mention_confidence == MentionConfidenceStatus::TITLE_DESC ||
		_serif_mention_confidence == MentionConfidenceStatus::APPOS_DESC ||
		_serif_mention_confidence == MentionConfidenceStatus::WHQ_LINK_PRON ||
		_serif_mention_confidence == MentionConfidenceStatus::NAME_AND_POSS_PRON ||
		_serif_mention_confidence == MentionConfidenceStatus::DOUBLE_SUBJECT_PERSON_PRON ||
		_serif_mention_confidence == MentionConfidenceStatus::ONLY_ONE_CANDIDATE_PRON ||
		_serif_mention_confidence == MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_PRON);
}

bool PGFactArgument::isEquivalent(PGFactArgument_ptr other) {
	if (_role != other->getRole())
		return false;

	if (_actor_id != -1 && other->getActorId() == _actor_id)
		return true;

	if (_resolved_string_value != L"" && other->getResolvedStringValue() == _resolved_string_value)
		return true;

	return false;
}

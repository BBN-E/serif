// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/SentenceItemIdentifier.h"

// Default value: 500.  This can be overridden by langauge modules.
// I would like to increase this to 32767 (i.e., 2**16/2-1), but 
// doing so would break backwards compatibility for some serialized
// values (where mention ids are serialized as a single int).
int MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS = 500;

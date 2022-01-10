// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LIMITS_H
#define LIMITS_H


/** This file defines system-wide limits. It is no longer possible
 * to define language-specific values for these limits.  If a limit
 * needs to vary by language, then it should not be a preprocessor
 * define; it should be a real variable. */

// TODO: Get rid of all these awful constants and use std::vector instead.

/** Tne maximum number of entity types */
#define MAX_ENTITY_TYPES 50

/** The maximum number of sentences per document: */
#define MAX_DOCUMENT_SENTENCES 20000

/** The maximum number of tokens per document region: */
#define MAX_REGION_TOKENS 10000

/** The maximum number of tokens per sentence: */
// Formerly, this was 300 for Chinese Korean and English:
#define MAX_SENTENCE_TOKENS 1000

/** The maximum number of NP Chunks per sentence */
#define MAX_NP_CHUNKS MAX_SENTENCE_TOKENS
/** The maximum number of wchar_t chars per token: */
#define MAX_TOKEN_SIZE 100
/*** The maximum number of Morphological Analyses that can be attached to a Token */
#define MAXIMUM_MORPH_ANALYSES 50

/** The maximum number of values per sentence: */
#define MAX_SENTENCE_VALUES 200

/** The maximum number of propositions per sentence: */
#define MAX_SENTENCE_PROPS 200

/** The maximum number of relations per sentence: */
#define MAX_SENTENCE_RELATIONS 100

/** The maximum number of events per sentence: */
#define MAX_SENTENCE_EVENTS 100

/** The maximum number of values per document: */
#define MAX_DOCUMENT_VALUES 10000

/** The maximum number of relations per document: */
#define MAX_DOCUMENT_RELATIONS 1000

#endif

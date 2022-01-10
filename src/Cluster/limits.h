// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LIMITS_H
#define LIMITS_H


/** This file defines system-wide limits. To make language-specific
  * overrides, edit your language's counterpart of this file, which
  * would be #include'ed below. */

/** Tne maximum number of entity types */
#define MAX_ENTITY_TYPES 50

#define MAX_DOC_REGIONS 100
#define MAX_DOCUMENT_VALUES 1000

/** The maximum number of sentences per document: */
#define MAX_DOCUMENT_SENTENCES 200000

/** The maximum number of tokens per document region: */
#define MAX_REGION_TOKENS 10000

/** The maximum number of wchar_t chars per sentence: */
#define MAX_SENTENCE_CHARS 5000

/** The maximum number of tokens per sentence: */
#define MAX_SENTENCE_TOKENS 5000
/** The maximum number of NP Chunks per sentence */
#define MAX_NP_CHUNKS MAX_SENTENCE_TOKENS
/** The maximum number of wchar_t chars per token: */
#define MAX_TOKEN_SIZE 500
/*** The maximum number of Morphological Analyses that can be attached to a Token */
#define MAXIMUM_MORPH_ANALYSES 50

/** The maximum number of values per sentence: */
#define MAX_SENTENCE_VALUES 100

/** The maximum number of propositions per sentence: */
#define MAX_SENTENCE_PROPS 200

/** The maximum number of relations per sentence: */
#define MAX_SENTENCE_RELATIONS 100

/** The maximum number of events per sentence: */
#define MAX_SENTENCE_EVENTS 100

/** The maximum number of relations per document: */
#define MAX_DOCUMENT_RELATIONS 1000

#endif

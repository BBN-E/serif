// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_ARABIC_POTENTIAL_RELATION_EXTRACTOR_H
#define AR_ARABIC_POTENTIAL_RELATION_EXTRACTOR_H


#include "Generic/common/GrowableArray.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/Metadata.h"
#include "Generic/preprocessors/IdfSpan.h"
#include "Generic/preprocessors/EntityTypes.h"
#include "Generic/common/IDGenerator.h"
#include "Generic/common/hash_map.h"

class Parse;
class NPChunkTheory;
class Document;
class Sentence;
class Region;
class EntityType;
class SymbolSubstitutionMap;
class TokenSequence;
class NameTheory;
class Token;
class SynNode;
class MentionSet;
class Mention;
class SentenceTheory;
class DocTheory;
class ArabicPotentialRelationCollector;

class PotentialRelationExtractor {

public:
	PotentialRelationExtractor(const DataPreprocessor::EntityTypes *entityTypes, ArabicPotentialRelationCollector *collector);
	~PotentialRelationExtractor();

	void processDocument(char *input_file, char *parse_file, char *output_dir = 0, char *output_prefix = 0);
	void outputPacketFile(const char *output_dir, const char *packet_name); 

	const DataPreprocessor::EntityTypes *_entityTypes;

private:

	int getSentences(Sentence **results, TokenSequence **tokenSequences, int max_sentences, 
						const Region* const* regions, int num_regions); 
	NameTheory* getNameTheory(TokenSequence *tokenSequence); 
	NPChunkTheory* constructMaximalNPChunkTheory(Parse *curr_parse, NameTheory *nameTheory);
	SynNode* convertToSynNodeWithMentions(const SynNode* parse, SynNode* parent, 
									    NameTheory *nameTheory, int start_token); 
	void loadMentionSet(MentionSet *mentionSet, NameTheory *nameTheory);
	bool isPunctuation(Symbol word);

	GrowableArray <Parse *> _parses;
	Document *_document;
	Metadata *_metadata;
	DocTheory *_docTheory;

	SymbolSubstitutionMap *_reverseSubstitutionMap;
	IDGenerator _nodeIDGenerator;

	ArabicPotentialRelationCollector *_relationCollector;


	DataPreprocessor::IdfSpan* getIdfSpan(const Token *token);
	DataPreprocessor::IdfSpan* getPronounSpan(const Token *token);
	bool isValid(Span *span);
	Symbol _primary_parse;
	/// A filter class for finding Idf spans at a document offset.
	class IdfSpanFilter : public Metadata::SpanFilter {
		public:
			IdfSpanFilter() {}

			/// Determines whether a span is an Idf span.
			virtual bool keep(Span *span) const;
	};

	/// A filter class for finding Pronoun spans at a document offset.
	class PronounSpanFilter : public Metadata::SpanFilter {
		public:
			PronounSpanFilter() {}

			/// Determines whether a span is an Pronoun span.
			virtual bool keep(Span *span) const;
	};



public:
	struct HashKey {
		size_t operator()(const int a) const {
			return a;
		}
	};
    struct EqualKey {
        bool operator()(const int a, const int b) const {
            return a == b;
        }
    };
	typedef serif::hash_map<int, EntityType, HashKey, EqualKey> IntegerMap;
private:	
	IntegerMap *_idTypeMap;
};

#endif

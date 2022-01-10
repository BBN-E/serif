// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KBA_STREAM_CORPUS_ITEM_WRITER_H
#define KBA_STREAM_CORPUS_ITEM_WRITER_H

#include <map>
#include <list>
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "KbaStreamCorpus/thrift-cpp-gen/streamcorpus_types.h"
#include "KbaStreamCorpus/KbaStreamCorpusItemReader.h"

// Forward Declarations
class DocTheory;
class SentenceTheory;
class Token;
class TokenSequence;
class SynNode;
class Mention;
class Entity;

class KbaStreamCorpusItemWriter {
public:
	KbaStreamCorpusItemWriter();
	~KbaStreamCorpusItemWriter();
	void addDocTheoryToItem(const DocTheory* docTheory, streamcorpus::StreamItem *item);

private:
	const DocTheory *_docTheory;
	streamcorpus::StreamItem *_item;
	std::map<const Token*, streamcorpus::Token*> _tokenMap; // serif_tok -> sc_tok
	std::map<const MentionUID, int> _mentionId; // serif_mention.id -> mention_id
	std::map<const MentionUID, int> _mentionSent; // serif_mention.id -> sent_id
	std::map<const int, int> _equivId; // serif_entity.id -> equiv_id

	std::string _offsetContentForm;

	bool _include_serifxml;
	bool _include_results;

	KbaStreamCorpusItemReader _itemReader; // to determine offset source.

	void addTagging();
	void addSentences();
	void addMentions();
	void addParses();
	void addEntities();
	void addRelations();
	void serifToScSentence(const SentenceTheory *sentTheory, int doc_token_num_offset, 
						   streamcorpus::Sentence &scSent);
	bool fixMentionOverlaps(std::list<const Mention*> &mentionList, int num_tokens);
	streamcorpus::EntityType::type serifToScEntityType(Symbol etype);
	streamcorpus::RelationType::type serifToScRelationType(Symbol etype);
	streamcorpus::MentionType::type serifToScMentionType(Mention::Type mtype);
	void addDepParse(const TokenSequence *tokSeq, const SynNode *node, 
					 int parent_id=-1, std::string path="") ;
	std::string getPathToHeadTerminal(const SynNode *node);
	std::string getPathFromHeadTerminal(const SynNode *node, const std::string& path);

	// Statistics -- we do *not* reset these for each new document.
	size_t _num_mentions;
	size_t _num_entities;
	size_t _num_relations;
	size_t _num_mentions_discarded_from_overlap;
	size_t _num_relations_discarded_because_mention_not_found;
};

#endif

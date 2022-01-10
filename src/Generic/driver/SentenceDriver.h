// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_DRIVER_H
#define SENTENCE_DRIVER_H

#include "Generic/driver/Stage.h"
#include "Generic/theories/SentenceTheory.h"

#include "dynamic_includes/common/ProfilingDefinition.h"
#include "Generic/common/GenericTimer.h"

class SessionProgram;
class DocumentDriver;
class SentenceTheoryBeam;
class DocTheory;
class Sentence;
class StateSaver;
class StateLoader;
class SessionLogger;
class MorphologicalAnalyzer;
class MorphSelector;
class MTNormalizer;
class NPChunkFinder;
class MTResultSaver;
class DependencyParser;
class ActorMentionFinder;

#define MAX_INTENTIONAL_FAILURES 1000


/** SentenceDriver is invoked by DocumentDriver to handle the processing
  * of sentence-level stuff.
  *
  * Its public methods are called in the following order:
  *   beginBatch()
  *     beginDocument()
  *       run()
  *       run()
  *       ...
  *     endDocument()
  *     beginDocument()
  *       run()
  *       run()
  *       ...
  *     endDocument()
  *     ...
  *   endBatch()
  *
  * In principle you can do multiple batches but this is untested.
  *
  */

class SentenceDriver {
public:

	SentenceDriver();
	virtual ~SentenceDriver();

	/** Let the sentence driver know that we're about to start processing a
	  * collection of documents.  sessionLogger will be used to generate 
	  * log messages during this batch.  stateLoader will be used to load 
	  * each document's sentences from a state file (if we're loading state).
	  * 
	  * If appropriate, beginBatch() will load the lexicon file from disk.
	  *
	  * The SentenceDriver takes ownership of the stateLoader that is passed
	  * in to this method. */
	void beginBatch(const SessionProgram *sessionProgram, SessionLogger *sessionLogger, StateLoader *stateLoader, bool use_lexicon_file=true);

	/** Let the sentence driver know that we're done processing a collection
	  * of documents.  This deletes the sentence driver's stateLoader, as well
	  * as the stage state savers.  It saves the lexicon, if appropriate. */
	void endBatch();

	void printDocName(const wchar_t *document_name);
	void clearLine();
	virtual void beginDocument(DocTheory *docTheory);
	virtual void endDocument();

	SentenceTheory *run(DocTheory *docTheory, int sent_no,
						const Sentence *sentence);
	virtual SentenceTheoryBeam *run(DocTheory *docTheory, int sent_no, Stage startStage, Stage endStage);
	void setMaxParserSeconds(int maxsecs);
	StateSaver *getStageStateSaver(Stage stage);

	StateLoader* getStateLoader() { return _stateLoader; }

	// switch to this StateLoader, deleting the old loader
	void resetStateLoader( StateLoader * stateLoader );

	// Set up the state savers.  document_name should be provided
	// if we're constructing single-document state files; and should
	// be the filename of the document we'll be saving state for.
	void resetStateSavers( const wchar_t * document_name = 0 );

	void makeNewStateSavers();
	void cleanup();

	/** Use state-loader to load a beam state from disk. */
	SentenceTheoryBeam *loadBeamState(Stage stage,
									  int sent_no,
									  DocTheory *docTheory);

	/** Load any models required to perform the specified stage.  If any error occurs
	  * during model loading, then an exception will be raised, which indicates what
	  * model was being loaded when the error occured.  This method may safely be called
	  * multiple times for the same stage -- if the stage has already been initialized,
	  * then it will simply return. */
	void loadModelsForStage(Stage stage);

	/** Return true if the models for the specified stage have been loaded. */
	virtual bool stageModelsAreLoaded(Stage stage);
protected:
	// stage constants
	Stage _START_Stage;
	Stage _tokens_Stage;
	Stage _partOfSpeech_Stage;
	Stage _names_Stage;
	Stage _nestedNames_Stage;
	Stage _values_Stage;
	Stage _parse_Stage;
	Stage _npchunk_Stage;
	Stage _dependencyParse_Stage;
	Stage _mentions_Stage;
	Stage _actorMatch_Stage;
	Stage _props_Stage;
	Stage _metonymy_Stage;
	Stage _entities_Stage;
	Stage _relations_Stage;
	Stage _events_Stage;
	Stage _END_Stage;

	// The session program and session logger are *not* owned by the 
	// SentenceDriver.  They're initialized when beginBatch() is called,
	// and zeroed out when endBatch() is called.
	const SessionProgram *_sessionProgram;
	SessionLogger *_sessionLogger;

	// All stage-specific subcomponents:
	class Tokenizer *_tokenizer;
	class MorphologicalAnalyzer *_morphAnalysis;
	class MorphSelector *_morphSelector;
	class MTNormalizer *_mtNormalizer;
	class PartOfSpeechRecognizer *_posRecognizer;
	class NameRecognizer *_nameRecognizer;
	class NestedNameRecognizer *_nestedNameRecognizer;
	class ValueRecognizer *_valueRecognizer;
	class Parser *_parser;
	class DescriptorRecognizer *_descriptorRecognizer;
	class PropositionFinder *_propositionFinder;
	class MetonymyAdder *_metonymyAdder;
	class DummyReferenceResolver *_dummyReferenceResolver;
	class EventFinder *_eventFinder;
	class RelationFinder *_relationFinder;
	class NPChunkFinder *_npChunkFinder;
	class MTResultSaver *_mtResultSaver;
	class DependencyParser *_dependencyParser;
	class ActorMentionFinder *_actorMentionFinder;

	// maximum beam width after each stage
	int _tokens_beam_width;
	int _pos_beam_width;
	int _names_beam_width;
	int _nested_names_beam_width;
	int _values_beam_width;
	int _parse_beam_width;
	int _npchunk_beam_width;
	int _dependency_parse_beam_width;
	int _mentions_beam_width;
	int _actor_match_beam_width;
	int _props_beam_width;
	int _relations_beam_width;
	int _events_beam_width;
	int _entities_beam_width;

	// maximum beam width at any time
	int _beam_width;

	// These methods initialize the models used by individual stages.  They are meant 
	// to be called by loadModelsForStage().  It is *not* safe to call any of these
	// methods multiple times -- doing so will result in a memory leak.
	virtual void loadTokenizerModels();
	virtual void loadPartOfSpeechModels();
	virtual void loadNameModels();
	virtual void loadNestedNameModels();
	virtual void loadValueModels();
	virtual void loadParseModels();
	virtual void loadNPChunkModels();
	virtual void loadDependencyParseModels();
	virtual void loadMentionModels();
	virtual void loadActorMatchModels();
	virtual void loadPropositionModels();
	virtual void loadMetonymyModels();
	virtual void loadEntityModels();
	virtual void loadEventModels();
	virtual void loadRelationModels();


	Symbol _primary_parse;	// "npchunk_parse", "full_parse", or "dependency_parse" depending on which parse mentions are linked to

	StateLoader *_stateLoader;
	Stage::HashMap<StateSaver*> _stageStateSavers;
	
	/** Token theory buffer */
	class TokenSequence **_tokenSequenceBuf;
	int _max_token_sequences; /// size of buffer
	int _n_token_sequences;

	/** PartOfSpeech theory buffer */
	class PartOfSpeechSequence **_partOfSpeechSequenceBuf;
	int _max_pos_sequences; /// size of buffer
	int _n_pos_sequences;

	/** Name theory buffer */
	class NameTheory **_nameTheoryBuf;
	int _max_name_theories; /// size of buffer
	int _n_name_theories;

	/** Nested name theory buffer */
	class NestedNameTheory **_nestedNameTheoryBuf;
	int _max_nested_name_theories; /// size of buffer
	int _n_nested_name_theories;

	/** Value theory buffer */
	class ValueMentionSet **_valueSetBuf;
	int _max_value_sets; /// size of buffer
	int _n_value_sets;

	/** Parse theory buffer */
	class Parse **_parseBuf;
	int _max_parses; /// size of buffer
	int _n_parses;
	
	/** NPChunk theory buffer */
	class NPChunkTheory ** _npChunkBuf;
	int _max_np_chunk;	//for now = 1;
	int _n_np_chunk;

	/** Dependency Parse theory buffer */
	class DependencyParseTheory ** _dependencyParseBuf;
	int _max_dependency_parses;
	int _n_dependency_parses;

	/** Descriptor theory buffer */
	class MentionSet **_mentionSetBuf;
	int _max_mention_sets; /// size of buffer
	int _n_mention_sets;

	/** ActorMatch set theory buffer */
	class ActorMentionSet *_actorMentionSetBuf;

	/** Proposition theory buffer */
	class PropositionSet *_propositionSetBuf;

	/** Entity theory buffer */
	class EntitySet *_entitySetBuf;

	/** Relation theory buffer */
	class RelMentionSet *_relationSetBuf;

	/** Event theory buffer */
	class EventMentionSet *_eventSetBuf;


	bool _intentional_failures[MAX_INTENTIONAL_FAILURES];
	static int _n_docs_processed;

	/** Initialize intentional failure facility */
	void initIntentionalFailures();

	/** Populate beam with different versions of given theory, each
	  * with a different subtheory from the array of subtheories provided. */
	void addSubtheories(SentenceTheoryBeam *nextBeam,
						const SentenceTheory *currentTheory,
						SentenceTheory::SubtheoryType subtheoryType,
						int n_new_subtheories,
						SentenceSubtheory *const *newSubtheories);

	/** Use state-saver to save a beam state to disk. */
	void saveBeamState(Stage stage, SentenceTheoryBeam *theoryBeam,
					   int sent_no);

	/** Come up with appropriate name for sentence beam state
	  * tree for state following given stage. */
	static void getStateTreeName(wchar_t *result, Stage stage);

	bool _use_sentence_level_relation_finding;
	bool _use_sentence_level_event_finding;

    bool _use_npchunker_constraints;
	bool _do_morph_selection;

	bool _do_mt_normalization;

	bool _use_lexicon_file;

	void clearStageStateSavers();

public:
	mutable Stage::HashMap<GenericTimer> stageLoadTimer;
	mutable Stage::HashMap<GenericTimer> stageProcessTimer;

	void logTrace();
};


#endif

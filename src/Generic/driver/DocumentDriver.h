// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_DRIVER_H
#define DOCUMENT_DRIVER_H


#include <wchar.h>
#include "Generic/driver/SessionProgram.h"

#include <string>
#include <map>

class Document;
class SentenceDriver;
class ResultCollector;
class DocumentReader;
class DocumentSplitter;
class SentenceBreaker;
class DocTheory;
class SessionLogger;
class GenericsFilter;
class DocEntityLinker;
class DocRelationEventProcessor;
class DocValueProcessor;
class XDocClient;
class ClutterFilter;
class StateLoader;
class LocatedString;
class PropositionStatusClassifier;
class FactFinder;
class ActorMentionFinder;
class ConfidenceEstimator;
class CauseEffectRelationFinder;

// session logging stuff
extern const wchar_t *CONTEXT_NAMES[];
const int SESSION_CONTEXT = 0;
const int DOCUMENT_CONTEXT = 1;
const int SENTENCE_CONTEXT = 2;
const int STAGE_CONTEXT = 3;
const int N_CONTEXTS = 4;

// For document driver tracing
#include "dynamic_includes/common/ProfilingDefinition.h"

#include "Generic/common/GenericTimer.h"

/** DocumentDriver handles the processing of batches of documents.
  * It hands control over to SentenceDriver for sentence-level stuff
  */

class DocumentDriver {
public:
	/** Create a new DocumentDriver to process SERIF documents.  The settings
	  * for the DocumentDriver (e.g., what models to use) are read from the
	  * global parameter file.  If a sessionProgram and resultCollector are
	  * specified, then they are used to call beginBatch. */
	DocumentDriver(const SessionProgram *sessionProgram=0,
				   ResultCollector *resultCollector=0,
				   SentenceDriver *sentenceDriver=0);
	virtual ~DocumentDriver();

	/** Begin processing a new batch of documents.  The session program specifies
	  * what documents to process, and what stages to run; and the result collector
	  * is used to record the output for each document in the batch.  */
	void beginBatch(const SessionProgram *sessionProgram, ResultCollector *resultCollector, bool use_lexicon_file=true);

	/** Clean up after processing a batch of documents.  Each call to beginBatch()
	  * should be matched by a call to endBatch(). */
	void endBatch();

	/** Return true if the DocumentDriver is currently processing a batch (i.e.,
	  * if beginBatch() has been called without a matching call to endBatch(). */
	bool inBatch();

	/** In normal Serif, use this to run through the whole batch */
	void run();

	void runOnDocTheory(DocTheory *docTheory, const wchar_t *document_filename=0, std::wstring *results=0);

	void serifXMLTest(); // Temporary procedure during serifxml development

	/** When using Serif as a resident service, use this to feed in a doc.
	  * If 'results' is not NULL, then the results will be written there. */
	void runOnString(const wchar_t *xmlString, std::wstring* results=0);

	// The function is used in resident Serif to parse a specific document.
	// (Same as DocumentDriver::run() but without a while a batch while loop.
	void run(const wchar_t *document_filename);

	ResultCollector* getResultCollector() { return _resultCollector; }
	ClutterFilter* getClutterFilter() const { return _clutterFilter; }
	
	//MEM: This GIVES the DocumentReader to the driver, i.e. the DRIVER will delete this reader
	void giveDocumentReader(DocumentReader *documentReader);

	const SessionProgram* getSessionProgram() const { return _sessionProgram; }

	void setMaxParserSeconds(int maxsecs);

	/** Load any models required to perform the specified stage.  If any error occurs
	  * during model loading, then an exception will be raised, which indicates what
	  * model was being loaded when the error occured.  This method may safely be called
	  * multiple times for the same stage -- if the stage has already been initialized,
	  * then it will simply return. */
	void loadModelsForStage(Stage stage);

	/** Return true if the models for the specified stage have been loaded. */
	bool stageModelsAreLoaded(Stage stage);

    void addAlternateResultCollectors(std::vector<ResultCollector*> *alternateResultCollectors) { _alternateResultCollectors = alternateResultCollectors; }

	/** Abstract base class for document-level processing stages.  Each 
	  * document-level processing stage defines a single method, process(),
	  * which acts on a given docTheory, updating it with any new information
	  * that is added by the stage.  Document-level processing stages must 
	  * come before the first sentence-level stage or after the last sentence-
	  * level stage.
	  *
	  * @see addDocTheoryStage() */
	struct DocTheoryStageHandler {
		virtual ~DocTheoryStageHandler() {}
		virtual void process(DocTheory *docTheory) = 0;
	};

	/** Add a new document-level processing stage to Serif's pipeline.  
	  *
	  * @param stage A Stage object specifying when the stage should be
	  * run.  This stage should come either before the first sentence-level
	  * stage, or after the last sentence-level stage.  If the 
	  * DocumentDriver class already knows how to handle this stage, then
	  * it will thrown an InternalInconsistencyException.
	  *
	  * @param StageHandler The class that should be used by the new
	  * stage to process documents.  It should be a subclass of 
	  * DocTheoryStageHandler.
	  *
	  * @note this is a static method, and affects all DocumentDrivers. 
	  */
	template<typename StageHandler> 
	static void addDocTheoryStage(Stage stage) {
		setStageHandler(stage, boost::shared_ptr<DocTheoryStageHandlerFactory>(_new DocTheoryStageHandlerFactoryFor<StageHandler>()));
	}
	struct DocTheoryStageHandlerFactory {
		virtual DocTheoryStageHandler* build() = 0; };

private:

	bool PRINT_COREF_FOR_OUTSIDE_SYSTEM;
	bool PRINT_SENTENCE_SELECTION_INFO;
	bool PRINT_SENTENCE_SELECTION_INFO_UNTOK;
	bool PRINT_TOKENIZATION;
	bool PRINT_PARSES;
	bool PRINT_MT_SEGMENTATION;
	bool PRINT_NAME_ENAMEX_TOKENIZED;
	bool PRINT_NAME_ENAMEX_ORIGINAL_TEXT;
	bool PRINT_NAME_VALUE_ENAMEX_ORIGINAL_TEXT;
	bool PRINT_HTML_DISPLAY;
	bool PRINT_PROPOSITON_TREES;

	// Both default to ENAMEX-style output, but can be forced to do color-coded HTML if desired
	void addNamesToDocString(DocTheory *docTheory, LocatedString *docText, bool output_html = false);
	void addValuesToDocString(DocTheory *docTheory, LocatedString *docText, bool output_html = false);

	// Is our source format serifxml?  If so, then we bypass the 
	// DocumentReader.
	bool _use_serifxml_source_format;

protected:

	/** ignore-errors param */
	bool _ignore_errors;

	/** Information we got from params */
	const SessionProgram *_sessionProgram;

	SessionLogger *_localSessionLogger;
	bool _globalSessionLoggerIsLocalSessionLogger;

	/** We hand each sentence off to this guy */
	SentenceDriver *_sentenceDriver;

	ResultCollector *_resultCollector;
    std::vector<ResultCollector*>* _alternateResultCollectors;

	DocumentReader *_documentReader;

	DocumentSplitter *_documentSplitter;

	SentenceBreaker *_sentenceBreaker;

	/** was dump-theories param turned on: */
	bool _dump_theories;

	/** Param for delaying creation of StateLoader: primarily used for server mode **/
	bool _delay_state_file_check;

	/** Optionally time out on a document **/
	double _max_document_processing_milliseconds;

	/** Document-level box that links entities in a second pass
	 * (often "strategically") */
	DocEntityLinker *_docEntityLinker;

	/** Document-level box that makes generic decisions */
	GenericsFilter *_genericsFilter;

	/** document-level clutter filter */
	ClutterFilter *_clutterFilter;

	/** Document-level box that makes xdoc decisions */
	XDocClient *_xdocClient;

	/** Document-level box for events and relations */
	DocRelationEventProcessor *_docRelationEventProcessor;

	/** Document-level box for values */
	DocValueProcessor *_docValueProcessor;

	/** Document-level confidence estimation */
	ConfidenceEstimator *_confidenceEstimator;

	/** Document-level engine for "fact finding" */
	FactFinder *_factFinder;

	/** Document-level proposition status classifier. */
	PropositionStatusClassifier *_propStatusClassifier;

	/** Docuemnt-level actor processor */
	ActorMentionFinder *_docActorProcessor;

	/** Cause-effect pattern-based finder */
	CauseEffectRelationFinder *_causeEffectRelationFinder;

	/** We perform some cleanup tasks (such as GC on the symbol table, and 
	  * discarding wordnet caches) periodically, to avoid unbounded memory
	  * growth.  These variables keep track of how often we should do cleanup,
	  * and how long it's been since our last cleanup*/
	size_t _num_docs_per_cleanup;
	size_t _num_docs_processed;

protected:
	// support functions

	/** Run serif on the given document.  If <code>document_filename</code> is not NULL, 
	  * then the output stage will write its output to a file whose name is based on 
	  * document_filename.  Otherwise, if <code>results</code> is not NULL, then the 
	  * results will be appended to <code>results</code>. */
	void runOnDocument(Document *document, const wchar_t *document_filename=0, std::wstring *results=0);

	/** This is for running on a batch that consists of one document */
	void runOnSingletonBatch(Document *document, std::wstring *results);

	// for now, the following are virtual because FBSerif overrides them, but
	// this will probably change... -- SRS
	virtual Document *loadDocument(const wchar_t *document_filename);

	/** Factory Method for creating the correct DocumentReader */
	virtual DocumentReader *createDocumentReader();

	virtual void outputResults(const wchar_t *document_filename,
							   DocTheory *docTheory,
							   const wchar_t *output_dir);

	virtual void outputResults(std::wstring *results,
							   DocTheory *docTheory);

	virtual void outputSerifXMLResults(const wchar_t *document_filename,
									   DocTheory *docTheory,
									   const wchar_t *output_dir);

	virtual void outputSerifXMLResults(std::wstring *results,
									   DocTheory *docTheory);

	virtual void outputMTResults(const wchar_t *document_filename,
								 DocTheory *docTheory,
								 const wchar_t *output_dir);

	virtual void outputMTResults(std::wstring *results,
								 DocTheory *docTheory);

	virtual void outputMSAResults(const wchar_t *document_filename,
								  DocTheory *docTheory,
								  const wchar_t *output_dir);

	// These methods initialize the models used by individual stages.  They are meant 
	// to be called by loadModelsForStage().  It is *not* safe to call any of these
	// methods multiple times -- doing so will result in a memory leak.
	virtual void loadSentenceBreakerModels();
	virtual void loadPropStatusModels();
	virtual void loadDocEntityModels();
	virtual void loadDocRelationsEventsModels();
	virtual void loadDocValuesModels();
	virtual void loadConfidenceModels();
	virtual void loadFactFinder();
	virtual void loadGenericsModels();
	virtual void loadClutterModels();
	virtual void loadXDocModels();
	virtual void loadDocActorModels();
	virtual void loadCauseEffectModels();

	void invokeScoreScript();

	/** Create human-readable dump of doc theory contents */
	virtual void dumpDocumentTheory(DocTheory *docTheory,
							const wchar_t *document_filename);
	
	void saveDocTheoryState(DocTheory *docTheory, Stage stage);
	void loadDocTheory(DocTheory *docTheory, Stage stage);
	void saveSentenceBreakState(DocTheory *docTheory);

 protected:
	void logSessionStart();

protected:
	/** If the session program is starting from scratch, then create a new
	  * DocTheory for the given document and return it.  Otherwise, load 
	  * any annotations we've already done from state files, and use that
	  * to create a new DocTheory. */
	DocTheory* getInitialDocTheory(Document *document);

public:
	mutable Stage::HashMap<GenericTimer> stageLoadTimer;
	mutable Stage::HashMap<GenericTimer> stageProcessTimer;
	mutable GenericTimer documentProcessTimer;

	void logTrace() const;

	/** Return the overall throughput in mb/hr. */
	float getThroughput(bool include_load_times=false);
private:
	size_t totalBytesProcessed;

	/** If symbol reference counting is used, and the symbol table size 
	  * exeeds this limit, then call Symbol::discardUnusedSymbols(). 
	  * This is set w/ the parameter "max_symbol_table_size".  Set to 0
	  * to disable symbol cleanup. */
	size_t _maxSymbolTableSize;

private:
	// Support types/methods for addDocTheoryStage():
	template<typename T>
	struct DocTheoryStageHandlerFactoryFor: public DocTheoryStageHandlerFactory {
		DocTheoryStageHandler* build() { return _new T(); } };
	static void setStageHandler(Stage stage, boost::shared_ptr<DocTheoryStageHandlerFactory> factory);
	typedef std::map<Stage, boost::shared_ptr<DocTheoryStageHandlerFactory> > DocTheoryStageHandlerFactoryMap;
	typedef std::map<Stage, DocTheoryStageHandler*> DocTheoryStageHandlerMap;
	static DocTheoryStageHandlerFactoryMap &_docTheoryStageHandlerFactories();
	DocTheoryStageHandlerMap _docTheoryStageHandlers;
};


#endif

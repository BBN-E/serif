// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/LocatedString.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/limits.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/sentences/SentenceBreaker.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/EntityType.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/parse/Parser.h"
#include "Generic/parse/Constraint.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/descriptors/DescriptorRecognizer.h"
#include "Generic/preprocessors/EntityTypes.h"
#include "Generic/preprocessors/IdfSpan.h"
#include "Generic/preprocessors/CorefSpan.h"
#include "Generic/preprocessors/DescriptorNPSpanCreator.h"
#include "Generic/preprocessors/Attributes.h"
#include "Generic/preprocessors/ResultsCollector.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/common/ParamReader.h"
#include "Generic/PNPChunking/NPChunkFinder.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/linuxPort/serif_port.h"

#include "AnnotatedParsePreprocessor/DocumentProcessor.h"


namespace DataPreprocessor {


Sentence *appSentences[MAX_DOCUMENT_SENTENCES];

static const Symbol ENAMEX_TYPE_SPAN = Symbol(L"Enamex");
static const Symbol TIMEX_TYPE_SPAN = Symbol(L"Timex");
static const Symbol NUMEX_TYPE_SPAN = Symbol(L"Numex");
static const Symbol LIST_TYPE_SPAN = Symbol(L"List");
static const Symbol PRONOUN_TYPE_SPAN = Symbol(L"Pronoun");
static const Symbol HEAD_TYPE_SPAN = Symbol(L"Head");
static const Symbol DESC_TYPE_SPAN = Symbol(L"Descriptor");
static const Symbol NONE_SYM = Symbol(L"NONE");
static const Symbol OTH_SYM = Symbol(L"OTH");

static const Symbol NULL_SYM = Symbol();

// Session logging stuff
const int SESSION_CONTEXT = 0;
const int DOCUMENT_CONTEXT = 1;
const int SENTENCE_CONTEXT = 2;
const int STAGE_CONTEXT = 3;
const int N_CONTEXTS = 4;

/**
 * @param entityTypes the entity types to use in processing
 * @param results the results collector.
 */
DocumentProcessor::DocumentProcessor(const EntityTypes *entityTypes)
{
	_entityTypes = entityTypes;
	_tokenizer = Tokenizer::build();
	_sentenceBreaker = SentenceBreaker::build();
	_parser = Parser::build();
	_descriptorRecognizer = _new DescriptorRecognizer();
	_npChunkFinder = 0;
	_npChunkTheory = 0;
	if(Symbol(L"true") ==
		ParamReader::getParam(Symbol(L"do_np_chunk")))
	{
		std::cout<<"Initialize NPChunkFinder"<<std::endl;
		_npChunkFinder = NPChunkFinder::build();
		_npChunkTheory = 0;
	}
	
}

DocumentProcessor::~DocumentProcessor()
{
	delete _tokenizer;
	delete _sentenceBreaker;
	delete _parser;
	delete _descriptorRecognizer;
	delete _npChunkFinder;
}

void DocumentProcessor::setResultsCollector(ResultsCollector& results) {
	_results = &results;
}

/**
 * @param token the token to search.
 * @return the Coref span that covers the given token, if any.
 */
CorefSpan* DocumentProcessor::getCorefSpan(const Token *token)
{
	DocumentProcessor::CorefSpanFilter filter;
	CorefSpan *span = NULL;

	// Find any Idf spans at the token's start offset.
	Metadata::SpanList *list = _metadata->getCoveringSpans(token->getStartOffset(), &filter);
	if (list->length() > 0) {
		span = (CorefSpan *)((*list)[0]);
	}
	delete list;

	return span;
}

/**
 * @param token the token to search.
 * @return the Idf span that covers the given token, if any.
 */
IdfSpan* DocumentProcessor::getIdfSpan(const Token *token)
{
	DocumentProcessor::IdfSpanFilter filter;
	IdfSpan *span = NULL;

	// Find any Idf spans at the token's start offset.
	Metadata::SpanList *list = _metadata->getCoveringSpans(token->getStartOffset(), &filter);
	if (list->length() > 0) {
		span = (IdfSpan *)((*list)[0]);
	}
	delete list;

	return span;
}

/**
 * @param token the token to search.
 * @return the Idf name span that covers the given token, if any.
 */
IdfSpan* DocumentProcessor::getIdfNameSpan(const Token *token)
{
	DocumentProcessor::IdfNameSpanFilter filter;
	IdfSpan *span = NULL;

	// Find any Idf spans at the token's start offset.
	Metadata::SpanList *list = _metadata->getCoveringSpans(token->getStartOffset(), &filter);
	if (list->length() > 0) {
		span = (IdfSpan *)((*list)[0]);
	}
	delete list;

	return span;
}


/**
 * @param token the token to search.
 * @return the non-name Coref span that covers the given token, if any.
 */
CorefSpan* DocumentProcessor::getNonNameSpan(const Token *token)
{
	DocumentProcessor::NonNameSpanFilter filter;
	CorefSpan *span = NULL;

	// Find any non-name spans at the token's start offset.
	Metadata::SpanList *list = _metadata->getCoveringSpans(token->getStartOffset(), &filter);
	if (list->length() > 0) {
		span = (CorefSpan *)((*list)[0]);
	}
	delete list;

	return span;
}

/**
 * @param node the node to search.
 * @return the Coref span that exactly covers the given node, if any.
 */
CorefSpan* DocumentProcessor::getSpanByNode(const SynNode *node) {
	DocumentProcessor::CorefSpanFilter filter;
	CorefSpan *span = NULL;
	CorefSpan *result = NULL;
	
	// Is this node an only child of an NP node?
	if ((node->getParent() != NULL) && 
		(node->getParent()->getNChildren() == 1) &&
		(NodeInfo::isOfNPKind(node->getParent())))
		return NULL;

	// Is this node non-NP, non-Preterminal with a single child?
	if (!(NodeInfo::isOfNPKind(node)) &&
		(node->getNChildren() == 1) &&
		(!node->isPreterminal()))
		return NULL;

	const Token *startTok = _tokenSequence->getToken(node->getStartToken());
	const Token *endTok = _tokenSequence->getToken(node->getEndToken());

	// Find any Coref spans at the token's start offset.
	Metadata::SpanList *list = _metadata->getStartingSpans(startTok->getStartOffset(), &filter);

	for (int i = 0; i < list->length(); i++) {
		span = (CorefSpan *)((*list)[i]);
		if (span->getEndOffset() == endTok->getEndOffset()) {
			result = span;
			break;
		}
	}
	delete list;

	return result;
}

/**
 * @param doc the document to process.
 */
void DocumentProcessor::processNextDocument(Document *doc)
{
	
	_doc = doc;
	_docTheory = _new DocTheory(_doc);
	_metadata = _doc->getMetadata();
	SessionLogger::logger->updateContext(DOCUMENT_CONTEXT, _doc->getName().to_string());

	// Break the document into sentences.
	_sentenceBreaker->resetForNewDocument(_doc);
	int n_sentences = _sentenceBreaker->getSentences(
		appSentences, MAX_DOCUMENT_SENTENCES,
		_doc->getRegions(), _doc->getNRegions());
	_docTheory->setSentences(n_sentences, appSentences);
	_descriptorRecognizer->resetForNewDocument(_docTheory);
	
	(*_results) << L"(" << _doc->getName().to_string() << L"\n\n";

	// Process each sentence and output it to the sexp-file.
	for (int i = 0; i < n_sentences; i++) {
		
		char sent_str[11];
		sprintf(sent_str, "%d", i);
		SessionLogger::logger->updateContext(SENTENCE_CONTEXT, sent_str);
		// Tokenize the sentence.
		const LocatedString *source = appSentences[i]->getString();

		SessionLogger::logger->updateContext(STAGE_CONTEXT, "tokens");
		_tokenizer->resetForNewSentence(_doc, i);
		_tokenizer->getTokenTheories(&_tokenSequence, 1, source);
		PartOfSpeechSequence* emptyPartOfSpeechTheory = _new PartOfSpeechSequence(i, _tokenSequence->getNTokens());


		//  Load the name theory
		loadNameTheory();

		// Load constraints

		_constraints = _new Constraint[_metadata->get_span_count()];
		int n_constraints = loadConstraints();
		
		//  Parse the sentence with constraints
		SessionLogger::logger->updateContext(STAGE_CONTEXT, "parse");
		_parser->resetForNewSentence();

		_parser->getParses(&_parse, 1,  _tokenSequence, emptyPartOfSpeechTheory, _nameTheory, 0, _constraints, n_constraints);
		//Turn parse into a NPChunk parse if the npchunker is defined
	
		if(_npChunkFinder){

			_npChunkFinder->getNPChunkTheories(&_npChunkTheory, 1, _tokenSequence, _parse, _nameTheory);
			delete _parse;
			_parse = _npChunkTheory->getParse();
			
		}
		//  Predict descriptors
		
		SessionLogger::logger->updateContext(STAGE_CONTEXT, "mentions");
		_descriptorRecognizer->resetForNewSentence();
		_descriptorRecognizer->getDescriptorTheories(&_mentionSet, 1, emptyPartOfSpeechTheory, _parse, _nameTheory, _tokenSequence, i);
		//  Create new descriptor spans covering entire NP, not just heads
		
		addDescriptorNPSpans();
		(*_results) << printParse(_parse->getRoot(), 2);
		(*_results) << L"\n\n";
		delete emptyPartOfSpeechTheory;
		// Don't delete sentences here - DocTheory takes care of it.
		//delete appSentences[i];
		//appSentences[i] = NULL;
		delete _tokenSequence;
		delete _nameTheory;
		delete []_constraints;
		delete _parse;
		delete _mentionSet;
	}

	delete _docTheory;
	(*_results) << L")";
}

/**
 *  Loads all name spans in metadata for the current sentence into _nameTheory.
 */
void DocumentProcessor::loadNameTheory() {
	
	NameSpan **_nameSpanBuffer = _new NameSpan*[_metadata->get_span_count()];
	int name_count = 0;
	int j;

	IdfSpan *previousSpan = NULL;
	
	for (j = 0; j < _tokenSequence->getNTokens(); j++) {
		const Token *token = _tokenSequence->getToken(j);

		// Find any name tags covering this token.
		IdfSpan *currentSpan = getIdfNameSpan(token);
		
		if (currentSpan != previousSpan) {
			if (isValid(previousSpan)) {
				_nameSpanBuffer[name_count]->end = j - 1;
				name_count++;
			}
			if (isValid(currentSpan)) {	
				_nameSpanBuffer[name_count] = _new NameSpan();
				_nameSpanBuffer[name_count]->start = j;
				_nameSpanBuffer[name_count]->type = EntityType(_entityTypes->lookup(currentSpan->getType()));
			}
			// make head spans for enamex coref-indexed spans of invalid entity type
			else if ( (currentSpan != NULL) && 
				(currentSpan->getType() == Symbol(L"PRONOUN"))&&
				(currentSpan->getID() != CorefSpan::NO_ID)){
					Attributes attributes(NULL_SYM, currentSpan->getID());
					_metadata->newSpan(Symbol(L"PRONOUN"), 
									currentSpan->getStartOffset(),
									currentSpan->getEndOffset(), 
									&attributes);
			}
			
			else if (currentSpan != NULL && currentSpan->getID() != CorefSpan::NO_ID) {
				Attributes attributes(NULL_SYM, currentSpan->getID());			
				_metadata->newSpan(Symbol(L"HEAD"), 
								   currentSpan->getStartOffset(),
								   currentSpan->getEndOffset(),
								   &attributes);
			}
		} 
		previousSpan = currentSpan;
	}
	// close last name span
	if (isValid(previousSpan)) {
		_nameSpanBuffer[name_count]->end = j - 1;
		name_count++;
	}

	_nameTheory = _new NameTheory();
	_nameTheory->nameSpans = _new NameSpan*[name_count];
	_nameTheory->n_name_spans = name_count;
	_nameTheory->score = 1;
	for (int k = 0; k < name_count; k++) {
		_nameTheory->nameSpans[k] = _nameSpanBuffer[k];
	}
	delete []_nameSpanBuffer;
}

/**
 *  Loads all non-name coref spans in metadata for the current sentence 
 *  into _constraints.
 */
int DocumentProcessor::loadConstraints() {
	DocumentProcessor::NonNameSpanFilter filter;
	int constraint_count = 0;

	CorefSpan *previousSpan = NULL;
	for (int j = 0; j < _tokenSequence->getNTokens(); j++) {
		const Token *token = _tokenSequence->getToken(j);

		Metadata::SpanList *list = _metadata->getStartingSpans(token->getStartOffset(), &filter);
		for (int k = 0; k < list->length(); k++) {
			CorefSpan *span = (CorefSpan *)((*list)[k]);

			if (isValid(span)) {
				_constraints[constraint_count].left = j;
				for (int l = j; l < _tokenSequence->getNTokens(); l++) {
					if (span->getEndOffset() == _tokenSequence->getToken(l)->getEndOffset()) {
						_constraints[constraint_count].right = l;
						break;
					}
				}
				if (span->getSpanType() == ENAMEX_TYPE_SPAN && ((IdfSpan *)span)->isDescriptor())  {
					_constraints[constraint_count].type = LanguageSpecificFunctions::getNameLabel();
					_constraints[constraint_count].entityType = EntityType(_entityTypes->lookup(((IdfSpan *)span)->getType()));
				}
				if (span->getSpanType() == ENAMEX_TYPE_SPAN && ((IdfSpan *)span)->isPronoun())  {
					_constraints[constraint_count].type = NULL_SYM;		
					_constraints[constraint_count].entityType = EntityType(_entityTypes->lookup(((IdfSpan *)span)->getType()));
				}
				else {
					_constraints[constraint_count].type = NULL_SYM;			
					_constraints[constraint_count].entityType = EntityType::getOtherType();
				}
				constraint_count++;
			}
		}
		delete list;
	}
	return constraint_count;
}

/**
 *  Creates a new DescriptorNP span for each IDF descriptor span in the current sentence
 *  that can be associated with a mention of type DESC. Adds new spans to metadata.
 */
void DocumentProcessor::addDescriptorNPSpans() {
	for (int i = 0; i < _mentionSet->getNMentions(); i++) {
		Mention *ment = _mentionSet->getMention(i);
		const SynNode *node = ment->getNode();
		if (ment->getMentionType() == Mention::DESC) {
			// find the head word node
			const SynNode *head = node->getHeadPreterm();
			// find span corresponding to head word
			const Token *headTok = _tokenSequence->getToken(head->getStartToken());
			IdfSpan *span = getIdfSpan(headTok);
			// find the first parent of head that has a mention
			const SynNode *headMentionNode = head;
			while (!headMentionNode->hasMention()) {
				if (headMentionNode->getParent() != NULL && headMentionNode->getParent()->getHeadPreterm() == head)
					headMentionNode = headMentionNode->getParent();
				else
					break; // no head mention found!
			}
			// check to make sure there isn't an existing coref span in head chain
			const SynNode *child = node;
			CorefSpan *existingSpan = getSpanByNode(child);
			while (existingSpan == NULL && child != headMentionNode && child != head) {
				existingSpan = getSpanByNode(child);
				child = child->getHead();
			}
			if (span != NULL && existingSpan == NULL) {
				if(span->isDescriptor() && _entityTypes->lookup(span->getType()) != NONE_SYM) {
					Attributes attributes(span->getType(), span->getID());
					Span *newSpan = _metadata->newSpan(Symbol(L"DESCRIPTOR"), 
						               _tokenSequence->getToken(node->getStartToken())->getStartOffset(), 
									   _tokenSequence->getToken(node->getEndToken())->getEndOffset(),
									   &attributes);
					span->setParent((CorefSpan *)newSpan);
				} 
			}
		}
	}
}


/**
 * @param node the node to print.
 * @param indent the number of spaces to indent node. 
 * @return the string representing the pretty-printed node.
 */
std::wstring DocumentProcessor::printParse(const SynNode *node, int indent) {
	DocumentProcessor::NonConstitSpanFilter nonConstitFilter(node, _tokenSequence);
	bool inserted_NP = false;

	std::wstring result = L"(";

	CorefSpan *span = getSpanByNode(node);

	if (isValid(span) && !span->hasParent()) {
		
		result += getTypeString(span, node);
		// HACK: Add an extra NP layer if span is not associated with an NP
		if (!NodeInfo::isOfNPKind(node)) {
			inserted_NP = true;
			result += LanguageSpecificFunctions::getNPlabel().to_string();
			result += getIDString(span);
			result += L"\n";
			for (int i = 0; i < indent; i++)
				result += L" ";
			indent += 2;
			result += L"(";
			result += node->getTag().to_string();
		}
		else {
			result += node->getTag().to_string();
			result += getIDString(span);
		}
	}
	else {
		result += node->getTag().to_string();
	}

	
	for (int j = 0; j < node->getNChildren(); j++) {
		result += L" ";	
		const SynNode *child = node->getChild(j);
		if (child->isTerminal()) {
			result += child->getTag().to_string(); 
		}
		else {
			result += L"\n";
			for (int i = 0; i < indent; i++)
				result += L" ";

			// HACK: check to see if child j is the beginning of a non-constituent span
			int start = _tokenSequence->getToken(child->getStartToken())->getStartOffset();
			Metadata::SpanList *startList = _metadata->getStartingSpans(start, &nonConstitFilter);
			for (int k = 0; k < startList->length(); k++) {
			    CorefSpan *nonConstit = (CorefSpan *)((*startList)[k]);
				if (isValid(nonConstit) && !nonConstit->hasParent()) {
					result += L"(";
					result += getTypeString(nonConstit, child);
					result += LanguageSpecificFunctions::getNPlabel().to_string();
					result += getIDString(nonConstit);
					result += L"\n";
					indent += 2;
					for (int i = 0; i < indent; i++)
						result += L" ";
				}	
			}
			delete startList;

			result += printParse(child, indent+2);
			
			// HACK: check to see if child j is the end of a non-constituent span
			int end = _tokenSequence->getToken(child->getEndToken())->getEndOffset();
			Metadata::SpanList *endList = _metadata->getEndingSpans(end, &nonConstitFilter);
			for (int l = 0; l < endList->length(); l++) {
			    CorefSpan *nonConstit = (CorefSpan *)((*endList)[l]);
				if (isValid(nonConstit) && !nonConstit->hasParent()) {
					result += L")";
					indent -= 2;
				}
			}
			delete endList;
		}
	}

	if (inserted_NP) {
		result += L")";
	}

	result += L")";
	
	return result;
}

/**
 * @param span the span to process.
 * @param node the node associated with the span.
 * @return the string representing the EDT and Mention types of the span.
 */
std::wstring DocumentProcessor::getTypeString(CorefSpan *span, const SynNode *node) {
	std::wstring result = L"";
	Symbol spanType = span->getSpanType();
	if (spanType == ENAMEX_TYPE_SPAN ||
		spanType == NUMEX_TYPE_SPAN  ||
		spanType == TIMEX_TYPE_SPAN) 
	{
		IdfSpan *idfSpan = (IdfSpan *)span;
		Symbol tokenType = _entityTypes->lookup(idfSpan->getType());
		if (tokenType != NONE_SYM) {
			result += tokenType.to_string();
			if (idfSpan->isName()) 
				result += L"/NAME/";
			else if (idfSpan->isDescriptor())
				result += L"/DESC/";
			else if (idfSpan->isPronoun())
				result += L"/PRO/";
			else if (idfSpan->isPrenominal())
				result += L"/PRE/";
			else
				throw UnrecoverableException("DocumentProcessor::getTypeString()", 
											"IdFSpan mention type is not recognized");
		}
	}
	else if (spanType == DESC_TYPE_SPAN) {
		IdfSpan *idfSpan = (IdfSpan *)span;
		Symbol tokenType = _entityTypes->lookup(idfSpan->getType());
		if (tokenType != NONE_SYM) {
			result += tokenType.to_string();
			result += L"/DESC/";
		}
	}
	else if	(spanType == LIST_TYPE_SPAN) {
		Symbol type = Symbol(L"OTH");
		DocumentProcessor::IdfSpanFilter idfFilter;
		Mention *ment = _mentionSet->getMentionByNode(node);
		if (ment != NULL) {
			Mention *child = ment->getChild();
			while (child != NULL) {
				Span *childSpan = getSpanByNode(child->getNode());	
				if (childSpan != NULL && idfFilter.keep(childSpan)) {
					Symbol childType = _entityTypes->lookup(((IdfSpan *)childSpan)->getType());
					if (childType == NONE_SYM) {
						type = Symbol(L"OTH");
						break;
					}
					if (childType != type) {
						if (type == Symbol(L"OTH"))
							type = childType;
						else {
							type = Symbol(L"OTH");
							break;
						}
					}
				} else { 
					type = Symbol(L"OTH");
					break;
				}
				child = child->getNext();
			}
		}
		result += type.to_string();
		result += L"/LIST/";
	}
	else if (spanType == PRONOUN_TYPE_SPAN)
		result += L"UNDET/PRON/";
	else if (spanType == HEAD_TYPE_SPAN)
		result += L"OTH/DESC/";

	return result;
}

/**
 * @param span the span to process.
 * @return the string representing the coref id of the span.
 */
std::wstring DocumentProcessor::getIDString(CorefSpan *span) {
	std::wstring result = L"";
	wchar_t str_buf[5];

	int id = span->getID();
	if (id != CorefSpan::NO_ID) {
		result += L" \"ID=";
#if defined(WIN32) || defined(WIN64)
		result += _itow(id, str_buf, 10);
#else
		swprintf (str_buf, sizeof (str_buf)/sizeof (str_buf[0]),
			  L"%d", id);
		result += str_buf;
#endif
		result += L"\"";
	}
	return result;
}

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span 
 *         is a type of span we want to process,
 *         <code>false</code> if not.
 */
bool DocumentProcessor::isValid(Span *span) {
	if (span == NULL)
		return false;

	Symbol spanType = span->getSpanType();
	if (spanType == ENAMEX_TYPE_SPAN ||
		spanType == NUMEX_TYPE_SPAN  ||
		spanType == TIMEX_TYPE_SPAN){
			return ((_entityTypes->lookup(((IdfSpan *)span)->getType()) != NONE_SYM) &&
			(_entityTypes->lookup(((IdfSpan *)span)->getType()) != OTH_SYM));
		}
	else if (spanType == PRONOUN_TYPE_SPAN ||
			 /*spanType == HEAD_TYPE_SPAN    ||*/
			 spanType == LIST_TYPE_SPAN	   ||
			 spanType == DESC_TYPE_SPAN)
		return true;
	else
		return false;
}


/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is an Idf span,
 *         <code>false</code> if not.
 */
bool DocumentProcessor::IdfSpanFilter::keep(Span *span) const {
	if (span == NULL)
		return false;

	Symbol type = span->getSpanType();
	return ((type == ENAMEX_TYPE_SPAN) ||
		    (type == TIMEX_TYPE_SPAN)  ||
		    (type == NUMEX_TYPE_SPAN));
}

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is an Idf name span,
 *         <code>false</code> if not.
 */
bool DocumentProcessor::IdfNameSpanFilter::keep(Span *span) const {
	if (span == NULL)
		return false;

	Symbol type = span->getSpanType();
	if ((type == ENAMEX_TYPE_SPAN) ||
		(type == TIMEX_TYPE_SPAN)  ||
		(type == NUMEX_TYPE_SPAN))
	{
		IdfSpan *idfSpan = (IdfSpan *)span;
		if (idfSpan->isName())
		{
			return true;
		}
	}
	return false;
}

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is a Coref span,
 *         <code>false</code> if not.
 */
bool DocumentProcessor::CorefSpanFilter::keep(Span *span) const {
	if (span == NULL)
		return false;

	Symbol type = span->getSpanType();
	return (type == ENAMEX_TYPE_SPAN) ||
		   (type == TIMEX_TYPE_SPAN)  ||
		   (type == NUMEX_TYPE_SPAN)  ||
		   (type == LIST_TYPE_SPAN)   ||
		   (type == HEAD_TYPE_SPAN)   ||
		   (type == DESC_TYPE_SPAN)   ||
		   (type == PRONOUN_TYPE_SPAN);  
}

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is a valid non-name span,
 *         <code>false</code> if not.
 */
bool DocumentProcessor::NonNameSpanFilter::keep(Span *span) const {
	DocumentProcessor::IdfSpanFilter filter;

	if (span == NULL)
		return false;

	if (filter.keep(span)) { 
		if (((IdfSpan *)span)->isName())
			return false;
		else
			return true;
	}
	
	Symbol type = span->getSpanType();
	return (type == LIST_TYPE_SPAN)   ||
	       (type == HEAD_TYPE_SPAN)   ||
	       (type == DESC_TYPE_SPAN)   ||
	       (type == PRONOUN_TYPE_SPAN);

}

DocumentProcessor::NonConstitSpanFilter::NonConstitSpanFilter(const SynNode *node, 
															  const TokenSequence *tokens) 
{
	_node = node;
	_tokens = tokens;
	_start = _tokens->getToken(_node->getStartToken())->getStartOffset();
	_end = _tokens->getToken(_node->getEndToken())->getEndOffset();
}

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is a valid non-constituent span,
 *         <code>false</code> if not.
 */
bool DocumentProcessor::NonConstitSpanFilter::keep(Span *span) const {
	DocumentProcessor::CorefSpanFilter filter;
	int child_start, child_end;

	if (_node == NULL)
		return false;

	if (filter.keep(span)) {
		int start_span = span->getStartOffset();
		int end_span = span->getEndOffset();
		if (_start <= start_span && start_span <= _end && _start <= end_span && end_span <= _end &&
			((start_span > _start  && end_span < _end) ||
			 (start_span == _start && end_span < _end) ||
			 (start_span > _start  && end_span == _end)))
		{
			for (int i = 0; i < _node->getNChildren(); i++) {
				const SynNode *child = _node->getChild(i);
				child_start = _tokens->getToken(child->getStartToken())->getStartOffset();
				child_end = _tokens->getToken(child->getEndToken())->getEndOffset();
				if (start_span == child_start && end_span > child_end)
					return true;
				else if (start_span < child_start)
					break;
			}
		}
	}
	return false;

}


} // namespace DataPreprocessor

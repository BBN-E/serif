// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <locale>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Arabic/relations/ar_PotentialRelationCollector.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/preprocessors/EntityTypes.h"
#include "Generic/preprocessors/DocumentInputStream.h"
#include "Generic/preprocessors/ResultsCollector.h"
#include "Generic/preprocessors/IdfSpan.h"
#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/trainers/EnamexDocumentReader.h"
#include "Generic/trainers/FullAnnotatedParseReader.h"
#include "Arabic/relations/ar_PotentialRelationExtractor.h"
#include "Generic/theories/LexicalToken.h"
#include "Arabic/parse/ar_STags.h"

using namespace DataPreprocessor;

class Metadata;

static const Symbol ENAMEX_TYPE_SPAN = Symbol(L"Enamex");
static const Symbol TIMEX_TYPE_SPAN = Symbol(L"Timex");
static const Symbol NUMEX_TYPE_SPAN = Symbol(L"Numex");
static const Symbol PRONOUN_TYPE_SPAN = Symbol(L"PRONOUN");
static const Symbol NONE_SYM = Symbol(L"NONE");
static const Symbol NULL_SYM = Symbol();


const float targetLoadingFactor = static_cast<float>(0.7);

PotentialRelationExtractor::PotentialRelationExtractor(const EntityTypes *entityTypes,
													   ArabicPotentialRelationCollector *collector):
													  _relationCollector(0)
{
	_entityTypes = entityTypes;
	_reverseSubstitutionMap = NULL;

	_primary_parse = ParamReader::getParam(Symbol(L"primary_parse"));
	if (!SentenceTheory::isValidPrimaryParseValue(_primary_parse))
		throw UnrecoverableException("ar_PotentialRelationExtractor:PotentialRelationExtractor()",
			"Parameter 'primary_parse' has an invald value");

	std::string file_name = ParamReader::getRequiredParam("reverse_subst");
	_reverseSubstitutionMap = _new SymbolSubstitutionMap(file_name.c_str());

	_relationCollector = collector;



}
PotentialRelationExtractor::~PotentialRelationExtractor() {
	if (_reverseSubstitutionMap != NULL) {
		delete _reverseSubstitutionMap;
	}
}

void PotentialRelationExtractor::processDocument(char *input_file,
												 char *parse_file,
												 char *output_dir,
												 char *output_prefix)
{
	cerr << "Reading parse file `" << parse_file << "'" << endl;
	//*SessionLogger::logger << "Reading parse file '" << parse_file << "'\n";

	FullAnnotatedParseReader parseReader(parse_file);
	parseReader.readAllParses(_parses);

	cerr << "Reading input file `" << input_file << "'" << endl;
	//*SessionLogger::logger << "Reading input file '" << input_file << "'\n";
	DocumentInputStream in(input_file);
	EnamexDocumentReader reader(in);

	_document = reader.readNextDocument();
	if (_document == 0)
		throw UnexpectedInputException("PotentialRelationExtractor::processDocument()",
										"No document found in input file.");
	_metadata = _document->getMetadata();
	_docTheory = _new DocTheory(_document);

	Sentence *sentences[MAX_DOCUMENT_SENTENCES];
	TokenSequence *tokenSequences[MAX_DOCUMENT_SENTENCES];

	// initialize coref ID hash
	int numBuckets = static_cast<int>(_metadata->get_span_count() / targetLoadingFactor);
	numBuckets = (numBuckets >= 2 ? numBuckets : 5);
	_idTypeMap = _new IntegerMap(numBuckets);

	// do sentence breaking
	int n_sentences = getSentences(sentences, tokenSequences, MAX_DOCUMENT_SENTENCES,
									_document->getRegions(), _document->getNRegions());
	_docTheory->setSentences(n_sentences,sentences);

	for (int i = 0; i < n_sentences; i++) {
		SentenceTheory *sentTheory = _new SentenceTheory(sentences[i], _primary_parse, _document->getName());
		// set the sentence theory
		_docTheory->setSentenceTheory(i, sentTheory);
		// adopt token sequence
		sentTheory->adoptSubtheory(SentenceTheory::TOKEN_SUBTHEORY, tokenSequences[i]);
		// get and adopt name theory
		NameTheory *nameTheory = getNameTheory(tokenSequences[i]);
		sentTheory->adoptSubtheory(SentenceTheory::NAME_SUBTHEORY, nameTheory);
		sentTheory->adoptSubtheory(SentenceTheory::PARSE_SUBTHEORY, _parses[i]);

		// create new parse with NPPs
		NPChunkTheory *npChunkTheory = constructMaximalNPChunkTheory(_parses[i], nameTheory);
		sentTheory->adoptSubtheory(SentenceTheory::NPCHUNK_SUBTHEORY, npChunkTheory);

		// initialize and load mention set
		MentionSet *mentionSet = _new MentionSet(npChunkTheory->getParse(), i);
		loadMentionSet(mentionSet, nameTheory);
		sentTheory->adoptSubtheory(SentenceTheory::MENTION_SUBTHEORY, mentionSet);
	}


	string output_prefix_as_string(output_prefix);
	wstring output_prefix_as_wstring(output_prefix_as_string.begin(), output_prefix_as_string.end());
	string output_dir_as_string(output_dir);
	wstring output_dir_as_wstring(output_dir_as_string.begin(), output_dir_as_string.end());

	// collect and output relations
	_relationCollector->loadDocTheory(_docTheory);
	_relationCollector->produceOutput(output_dir_as_wstring.c_str(), output_prefix_as_wstring.c_str());

	/*char dump_file[500];
	sprintf(dump_file, "%s/%s.dump", output_dir, output_prefix);
	ofstream out;
	out.open(dump_file);
	out << *_docTheory << "\n";
	out.close();*/

	delete _document;
	delete _docTheory;
	while (_parses.length() > 0) {
		Parse *p = _parses.removeLast();
		delete p;
	}
	delete _idTypeMap;

}


int PotentialRelationExtractor::getSentences(Sentence **sentences, TokenSequence **tokenSequences,
												   int max_sentences, const Region* const* regions, int num_regions)
{
	Symbol terminals[MAX_SENTENCE_TOKENS];
	Token *tokenBuf[MAX_SENTENCE_TOKENS];
	int end = -1, start = 0;
	int curr_region = 0;
	const LocatedString *region = regions[curr_region]->getString();

	for (int i = 0; i < _parses.length(); i++) {
		const SynNode *root = _parses[i]->getRoot();
		int n_terms = root->getTerminalSymbols(terminals, MAX_SENTENCE_TOKENS);

		if (end >= region->length() && curr_region < num_regions - 1) {
				region = regions[++curr_region]->getString();
				start = 0;
				end = -1;
		}

		for (int j = 0; j < n_terms; j++) {
			if (_reverseSubstitutionMap->contains(terminals[j]))
				terminals[j] = _reverseSubstitutionMap->replace(terminals[j]);
			const wchar_t *str = terminals[j].to_string();
			int new_end = region->indexOf(str, end);
			// check to make sure there are no valid EDT chars between last token and new token
			bool bad_match = false;
			if (new_end != -1) {
				bool in_tag = false;
				int k = 0;
				for (k = end + 1; k < new_end; k++) {
					if (region->charAt(k) == '<')
						in_tag = true;
					if (!in_tag && !iswspace(region->charAt(k))) {
						bad_match = true;
						break;
					}
				}
				if (region->charAt(k) == '>')
					in_tag = false;
			}
			// couldn't find the entire word - seach for pieces separated by spaces
			if (new_end == -1 || bad_match) {
				wchar_t one_char[2];
				one_char[1] = '\0';
				for (int k = 0; k < static_cast<int>(wcslen(str)); k++) {
					one_char[0] = str[k];
					int ch = region->indexOf(one_char, end);
					if (ch == -1) {
						char message[501];
						sprintf(message, "Cannot match parse token %s in source file", terminals[j].to_debug_string());
						throw UnexpectedInputException("PotentialRelationExtractor::getSentences()",
															message);
					}
					if (k == 0) {
						new_end = ch;
						for (int l = end + 1; l < new_end; l++) {
							if (!iswspace(region->charAt(l)) && end != 0) {
								char message[501];
								sprintf(message, "Cannot match parse token %s in source file", terminals[j].to_debug_string());
								throw UnexpectedInputException("PotentialRelationExtractor::getSentences()",
															message);
							}
						}
					}
					end = ch;
				}
				tokenBuf[j] = _new LexicalToken(region, new_end, end, terminals[j]);
			}
			else {
				end = new_end + static_cast<int>(wcslen(str));
				tokenBuf[j] = _new LexicalToken(region, new_end, end-1, terminals[j]);
			}
		}

		sentences[i] = _new Sentence(_document, regions[curr_region], i, region->substring(start, end));
		tokenSequences[i] = _new LexicalTokenSequence(i, n_terms, tokenBuf);

		start = end;
	}

	return _parses.length();

}

NameTheory* PotentialRelationExtractor::getNameTheory(TokenSequence *tokenSequence) {
	locale loc ( "English_USA" );
	NameSpan **nameSpanBuffer = _new NameSpan*[_metadata->get_span_count()];
	int name_count = 0;

	IdfSpan *previousSpan = NULL;
	int j = 0;
	for (j = 0; j < tokenSequence->getNTokens(); j++) {
		const Token *token = tokenSequence->getToken(j);

		// Find any name tags covering this token.
		IdfSpan *currentSpan = getIdfSpan(token);

		if (currentSpan != previousSpan) {
			if (isValid(previousSpan)) {
				nameSpanBuffer[name_count]->end = j - 1;
				name_count++;
			}
			if (isValid(currentSpan)) {
				nameSpanBuffer[name_count] = _new NameSpan();
				nameSpanBuffer[name_count]->start = j;
				nameSpanBuffer[name_count]->type = EntityType(_entityTypes->lookup(currentSpan->getType()));
			}
		}
		previousSpan = currentSpan;
	}
	// close last name span
	if (isValid(previousSpan)) {
		nameSpanBuffer[name_count]->end = j - 1;
		name_count++;
	}

	NameTheory *nameTheory = _new NameTheory(tokenSequence, name_count);
	nameTheory->setScore(1);
	for (int k = 0; k < name_count; k++) {
		// if there are more than one tokens in a name, check for clitics that are still attached
		if (nameSpanBuffer[k]->end > nameSpanBuffer[k]->start) {
			int start = nameSpanBuffer[k]->start;
			int end = nameSpanBuffer[k]->end;
			if (tokenSequence->getToken(start + 1)->getStartEDTOffset() ==
				EDTOffset(tokenSequence->getToken(start)->getEndEDTOffset().value() + 1))
			{
				// don't split off punctuation
				if (!isPunctuation(tokenSequence->getToken(start)->getSymbol())) {
					// if the name is only 2 tokens long, we have to check to see which token is
					// the clitic.  If the first token is 1 character, we'll assume that's it.
					if (start + 1 != end ||
						wcslen(tokenSequence->getToken(start)->getSymbol().to_string()) == 1)
					{
						nameSpanBuffer[k]->start = start + 1;
					}
				}
			}
			if (nameSpanBuffer[k]->end > nameSpanBuffer[k]->start &&
			   (tokenSequence->getToken(end)->getStartEDTOffset() ==
				EDTOffset(tokenSequence->getToken(end - 1)->getEndEDTOffset().value() + 1)))
			{
				// don't split off punctuation
				if (!isPunctuation(tokenSequence->getToken(end)->getSymbol())) {
					// if the name is more than two tokens long, we could be dealing with
					// a clitic in the middle of the name.  In that case, we don't want to
					// split off anything. If the second to last token is only 1 char,
					// we'll assume that's the clitic.
					if (wcslen(tokenSequence->getToken(end - 1)->getSymbol().to_string()) > 1)
						nameSpanBuffer[k]->end = end - 1;
				}
			}
		}
		nameTheory->takeNameSpan(nameSpanBuffer[k]);
	}
	delete [] nameSpanBuffer;

	return nameTheory;
}

NPChunkTheory* PotentialRelationExtractor::constructMaximalNPChunkTheory(Parse *curr_parse, NameTheory *nameTheory) {
	_nodeIDGenerator = IDGenerator(0);
	SynNode* maxchunk_root =convertToSynNodeWithMentions(curr_parse->getRoot(), 0, nameTheory, 0);
	NPChunkTheory* npChunkTheory = _new NPChunkTheory(curr_parse->getTokenSequence());
	npChunkTheory->setParse(maxchunk_root);
	//add the top level nps as npchunk spans
	for(int i=0; i<maxchunk_root->getNChildren(); i++){
		if(LanguageSpecificFunctions::isNPtypeLabel(maxchunk_root->getChild(i)->getTag())){
			npChunkTheory->npchunks[npChunkTheory->n_npchunks][0]=
				maxchunk_root->getChild(i)->getStartToken();
			npChunkTheory->npchunks[npChunkTheory->n_npchunks][1]=
				maxchunk_root->getChild(i)->getEndToken();
			npChunkTheory->n_npchunks++;
		}
	}
	return npChunkTheory;


}

SynNode *PotentialRelationExtractor::convertToSynNodeWithMentions(const SynNode* parse,
																SynNode* parent,
																NameTheory *nameTheory,
																int start_token)
{
	if (parse->isTerminal()) {
		SynNode *snode = _new SynNode(_nodeIDGenerator.getID(), parent,
			parse->getTag(), 0);
		snode->setTokenSpan(start_token, start_token);
		return snode;
	}

	int *child_spans = _new int[parse->getNChildren()];
	for (int init = 0; init < parse->getNChildren(); init++)
		child_spans[init] = 0;

	bool found_exact_mention = false;
	for (int i = 0; i < nameTheory->getNNameSpans(); i++) {
		NameSpan *span = nameTheory->getNameSpan(i);
		if (span->start == parse->getStartToken() && span->end == parse->getEndToken()) {
			found_exact_mention = true;
		}
		else if (span->start >= parse->getStartToken() && span->end <= parse->getEndToken()) {
			int start = -1, end = -1;
			for (int j = 0; j < parse->getNChildren(); j++) {
				if (parse->getChild(j)->getStartToken() == span->start)
					start = j;
				if (parse->getChild(j)->getEndToken() == span->end)
					end = j;
			}
			if (start != -1 && end != -1) {
				child_spans[start] = end - start;
			}
		}
	}

	if (parse->getTag() == ArabicSTags::SBAR ||
		parse->getTag() == ArabicSTags::VP ||
		parse->getTag() == ArabicSTags::S ||
		parse->getTag() == ArabicSTags::S_ADV)
	{
		for (int m = 0; m < parse->getNChildren(); m++) {
			if (parse->getChild(m)->getTag() == ArabicSTags::NP) {
				int start = m;
				while (m + 1 < parse->getNChildren() && parse->getChild(m + 1)->getTag() == ArabicSTags::PP)
					m++;
				if (child_spans[start] == 0)
					child_spans[start] = m - start;
				else
					child_spans[start] = child_spans[start] > m - start ? child_spans[start] : m - start;
			}
		}
	}

	if (ArabicSTags::isPronoun(parse->getTag()))
		found_exact_mention = true;

	int n_new_children = 0;
	for (int k = 0; k < parse->getNChildren(); k++) {
		n_new_children++;
		k += child_spans[k];
	}


	SynNode *nameNode = 0;
	SynNode *snode;
	if (found_exact_mention && !LanguageSpecificFunctions::isNPtypeLabel(parse->getTag()) &&
		!(LanguageSpecificFunctions::isNPtypeLabel(parent->getTag()) && parent->getNChildren() == 1))
	{
		nameNode = _new SynNode(_nodeIDGenerator.getID(), parent,
			LanguageSpecificFunctions::getNameLabel(), 1);
		nameNode->setHeadIndex(0);
		nameNode->setTokenSpan(parse->getStartToken(), parse->getEndToken());
		snode = _new SynNode(_nodeIDGenerator.getID(), nameNode,
							parse->getTag(), n_new_children);
		nameNode->setChild(0, snode);

	}
	else {
		snode = _new SynNode(_nodeIDGenerator.getID(), parent,
			parse->getTag(), n_new_children);
	}

	snode->setHeadIndex(parse->getHeadIndex());

	int token_index = start_token;

	// create children
	int new_index = 0;
	for (int child_index = 0; child_index < parse->getNChildren(); child_index++) {
		if (child_spans[child_index] > 0) {
			SynNode *nameNode = _new SynNode(_nodeIDGenerator.getID(), parent,
				LanguageSpecificFunctions::getNameLabel(), child_spans[child_index] + 1);
			for (int i = 0; i <= child_spans[child_index]; i++) {
				nameNode->setChild(i,
					convertToSynNodeWithMentions(parse->getChild(child_index + i), nameNode, nameTheory, token_index));
				token_index = nameNode->getChild(i)->getEndToken() + 1;
			}
			nameNode->setTokenSpan(nameNode->getChild(0)->getStartToken(),
								   nameNode->getChild(nameNode->getNChildren() - 1)->getEndToken());
			nameNode->setHeadIndex(0);
			snode->setChild(new_index, nameNode);
			child_index += child_spans[child_index];
		}
		else {
			snode->setChild(new_index,
				convertToSynNodeWithMentions(parse->getChild(child_index), snode, nameTheory, token_index));
		}
		token_index = snode->getChild(new_index)->getEndToken() + 1;
		new_index++;
	}

	snode->setTokenSpan(start_token, token_index - 1);

	delete [] child_spans;

	return nameNode == 0 ? snode : nameNode;

}

void PotentialRelationExtractor::loadMentionSet(MentionSet *mentionSet, NameTheory *nameTheory) {
	TokenSequence *tokens = _docTheory->getSentenceTheory(mentionSet->getSentenceNumber())->getTokenSequence();
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		for (int j = 0; j < nameTheory->getNNameSpans(); j++) {
			if (mention->getNode()->getStartToken() == nameTheory->getNameSpan(j)->start &&
				mention->getNode()->getEndToken() == nameTheory->getNameSpan(j)->end)
			{
				IdfSpan *currentSpan = getIdfSpan(tokens->getToken(mention->getNode()->getStartToken()));
				if (currentSpan->isDescriptor())
					mention->mentionType = Mention::DESC;
				else
					mention->mentionType = Mention::NAME;
				mention->setEntityType(nameTheory->getNameSpan(j)->type);
				break;
			}
		}
		if (mention->getNode()->getNTerminals() == 1 &&
			ArabicSTags::isPronoun(mention->getNode()->getHeadPreterm()->getTag()))
		{
			mention->mentionType = Mention::PRON;
			IdfSpan *span = getPronounSpan(tokens->getToken(mention->getNode()->getStartToken()));
			if (span != 0 && (span->getID() != CorefSpan::NO_ID) && (_idTypeMap->get(span->getID()) != 0))
			{
				mention->setEntityType(*_idTypeMap->get(span->getID()));
			}
			else if (WordConstants::is2pPronoun(mention->getNode()->getHeadWord()) ||
					 WordConstants::is1pPronoun(mention->getNode()->getHeadWord()))
			{
				mention->setEntityType(EntityType::getPERType());
			}
		}
		// check here for coref ids - add to ID->Type hash
		IdfSpan *span = getIdfSpan(tokens->getToken(mention->getNode()->getStartToken()));
		if (span != 0 &&
			span->getStartOffset() == tokens->getToken(mention->getNode()->getStartToken())->getStartEDTOffset() &&
			span->getEndOffset() == tokens->getToken(mention->getNode()->getEndToken())->getEndEDTOffset())
		{
			int id = span->getID();
			Symbol type = _entityTypes->lookup(span->getType());
			if (id != CorefSpan::NO_ID && EntityType::isValidEntityType(type))
				(*_idTypeMap)[id] = EntityType(type);
		}
	}
}


/**
 * @param token the token to search.
 * @return the Idf name span that covers the given token, if any.
 */
IdfSpan* PotentialRelationExtractor::getIdfSpan(const Token *token)
{
	PotentialRelationExtractor::IdfSpanFilter filter;
	IdfSpan *span = NULL;

	// Find any Idf spans at the token's start offset.
	Metadata::SpanList *list = _metadata->getCoveringSpans(token->getStartEDTOffset(), &filter);
	if (list->length() > 0) {
		span = (IdfSpan *)((*list)[0]);
	}
	delete list;

	return span;
}

IdfSpan* PotentialRelationExtractor::getPronounSpan(const Token *token)
{
	PotentialRelationExtractor::PronounSpanFilter filter;
	IdfSpan *span = NULL;

	// Find any Idf spans at the token's start offset.
	Metadata::SpanList *list = _metadata->getStartingSpans(token->getStartEDTOffset(), &filter);
	if (list->length() > 0) {
		span = (IdfSpan *)((*list)[0]);
	}
	delete list;

	return span;

}

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span
 *         is a type of span we want to process,
 *         <code>false</code> if not.
 */
bool PotentialRelationExtractor::isValid(Span *span) {
	if (span == NULL)
		return false;

	Symbol spanType = span->getSpanType();
	if (spanType == ENAMEX_TYPE_SPAN ||
		spanType == NUMEX_TYPE_SPAN  ||
		spanType == TIMEX_TYPE_SPAN)
		return (_entityTypes->lookup(((IdfSpan *)span)->getType()) != NONE_SYM);
	/*else if (spanType == PRONOUN_TYPE_SPAN)
			 spanType == HEAD_TYPE_SPAN    ||
			 spanType == LIST_TYPE_SPAN	   ||
			 spanType == DESC_TYPE_SPAN)
		return true;*/
	else
		return false;
}


/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is an Idf span,
 *         <code>false</code> if not.
 */
bool PotentialRelationExtractor::IdfSpanFilter::keep(Span *span) const {
	if (span == NULL)
		return false;

	Symbol type = span->getSpanType();
	return ((type == ENAMEX_TYPE_SPAN) ||
		    (type == TIMEX_TYPE_SPAN)  ||
		    (type == NUMEX_TYPE_SPAN));
}

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is an Pronoun span,
 *         <code>false</code> if not.
 */
bool PotentialRelationExtractor::PronounSpanFilter::keep(Span *span) const {
	if (span == NULL)
		return false;

	Symbol type = span->getSpanType();
	return (type == ENAMEX_TYPE_SPAN &&
			((IdfSpan *)span)->getType() == PRONOUN_TYPE_SPAN);
}

void PotentialRelationExtractor::outputPacketFile(const char *output_dir,
										  			   const char *packet_name)
{
	_relationCollector->outputPacketFile(output_dir, packet_name);
}

bool PotentialRelationExtractor::isPunctuation(Symbol word) {
	return (
		(word == Symbol(L"-RRB-")) ||
		(word == Symbol(L"-LRB-")) ||
		(word == Symbol(L"-FSLASH-")) ||
		(word == Symbol(L"-BSLASH-")) ||
		(word == Symbol(L"''")) ||
		(word == Symbol(L"\"")) ||
		(word == Symbol(L"/")) ||
		(word == Symbol(L"(")) ||
		(word == Symbol(L";")) ||
		(word == Symbol(L":")) ||
		(word == Symbol(L")")) ||
		(word == Symbol(L"\\")) ||
		(word == Symbol(L"+")) ||
		(word == Symbol(L"?")) ||
		(word == Symbol(L".")) ||
		(word == Symbol(L"-")) ||
		(word == Symbol(L",")));
}

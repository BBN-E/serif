// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

//#define ARABIC_LANGUAGE
#include "Generic/common/Symbol.h"
#include "Arabic/sentences/ar_SentenceBreaker.h"
#include "Generic/driver/ParseLinkBuilder.h"
#include "Generic/theories/Sentence.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
//#include "Generic/common/Symbol.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/theories/Parse.h"
//#include "Generic/edt/NameLinker.h"
//#include "Generic/edt/TreeSearch.h"
#include "Generic/edt/ReferenceResolver.h"
#include "Generic/common/SymbolConstants.h"
#include "Arabic/token/ab_txt_xmltokenizer.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Document.h"
#include "Arabic/reader/DocumentReader.h"
#include "Generic/results/ResultCollector.h"
#include <wchar.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <boost/scoped_ptr.hpp>

const int MAX_DOC_LENGTH = 20000; //TODO: remove this
const int MAX_SENTENCES = 100;
const int MAX_NAMES = 25;

int main(int argc, char **argv) {

	//if(argc < 1){
	//	std::cerr<<"ArabicSerif.exe should be called with 1 arguments - par_file"<<std::endl;
	//	return 0;
	//}
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	//std::cout<<argv[1]<<"\n"<<argv[2]<<"\n"<<argv[3]<<std::endl;


	//char* paramFile = argv[1];
	//char* infile = argv[2];
	//char* outfile =argv[3];
	char* infile ="c:\\SERIF\\data\\Arabic\\nltests\\key\\20000915_AFP_ARB.0031.out.sgm.link";
	char* outfile = "c:\\SERIF\\data\\Arabic\\nltests\\TEST31.out";
	char* paramFile ="c:\\SERIF\\data\\Arabic\\nltests\\ar_namelink.par";
	ParamReader::readParamFile(paramFile);


	//ParamReader::getParam("input_file",infile, 256);
	//std::cout<<filename<<std::endl;


	//initialize and reuse one nametheory, namelinker,
	ReferenceResolver* _referenceResolver = _new ReferenceResolver();
	int _max_entity_sets = 1;
	/*
	NameTheory* names = _new NameTheory();
	names->nameSpans = _new NameSpan*[MAX_NAMES];
	names-> n_name_spans = 0;

	EntitySet** _entitySetBuf = _new EntitySet*[_max_entity_sets];
	*/
	DocTheory* docTheory;

	uis.open(infile);
	//get each sentence, don't use ArabicDocumentReader b/c we need the xml tags for name's
	/*
	wchar_t text[MAX_DOC_LENGTH+1];
	int cnt = 0;
	while ((!uis.eof()) && (cnt < MAX_DOC_LENGTH)) {
		text[cnt++] = 	uis.get();
	}
	text[cnt] = L'\0';
	LocatedString *lString = _new LocatedString(text, 0);
	Document* doc = _new Document(Symbol(L"dummyDoc"),1,&lString);

	*/
	ArabicDocumentReader* docReader = _new ArabicDocumentReader();
	Document* doc = docReader->readDocument(uis);

	uis.close();
	docTheory = _new DocTheory(doc);
	//Sentence Breaking
	ArabicSentenceBreaker breaker;
	Sentence** sentenceSequence = _new Sentence*[MAX_SENTENCES];
	breaker.resetForNewDocument(0);		//this should be doc
    int n_sentences = breaker.getSentences(sentenceSequence, MAX_SENTENCES, doc->getRegions(), 1);
	docTheory->setSentences(n_sentences, sentenceSequence);
	int numToks = 0;
	ArabicTokenizer* tk = _new ArabicTokenizer();


	LocatedString* sent;
	ParseLinkBuilder *pnBuilder= _new ParseLinkBuilder();
	EntitySet** _entitySetBuf = _new EntitySet*[_max_entity_sets];


	for (int i = 0; i < n_sentences; i++) {
		std::cout<<"Sentence "<<i<<std::endl;
		//_debugOut<<"------------------------Start Sentence ----------------------\n";
		sent = sentenceSequence[i]->getString();
		//tokenize
		//TokenSequence* tokSeq = tk->makeTokenSequence(sent, i);
		tk->makeTokenBuffer(sent,i);
		//build parse tree and name theories
		int currTag = 0;
		int j;
		//int numToks = tokSeq->getNTokens();
		int numToks = tk->getNToks();
		for(j=0; j<numToks; j++){
			//Symbol sym = (tokSeq->getToken(j))->getSymbol();
			Symbol sym = (tk->getToken(j))->getSymbol();
			const wchar_t* word=sym.to_string();
			if(word[0]!=L'<'){
				pnBuilder->addWord(sym);
			}
			else{
				currTag = pnBuilder->processTag(sym);
				pnBuilder->addTag(currTag);
			}
		}

		//Copy NameSpans and ParseTrees into MentionSet
		NameTheory* names = _new NameTheory();
		names->nameSpans = _new NameSpan*[pnBuilder->_name_count];

		int k;
		for( k=0; k<pnBuilder->_name_count; k++){
			pnBuilder->getNameSpan(names->nameSpans[k], k);
		}
		names->n_name_spans = k;
		SynNode* s_result = pnBuilder->convertParseToSynNode( 0, 0);
		Parse *parse = _new Parse(s_result, 1.0);
		MentionSet *results = _new MentionSet(parse->getRoot(), i);
		names->putInMentionSet(results);

		//Resolve References using referenceResolver
		//_referenceResolver->resetForNewSentence(docTheory, i);
		//int _n_entity_sets = _referenceResolver->getEntityTheories( _entitySetBuf,
		//	_max_entity_sets, parse, results);
		if(i==0)
			_entitySetBuf[0] = _new EntitySet(results, docTheory->getNSentences());
		else
			_entitySetBuf[0]->loadMentionSet(results);
		pnBuilder->buildEntitySet(results,_entitySetBuf[0]);

		SentenceTheory* st = _new SentenceTheory(sentenceSequence[i]);
		st->adoptSubtheory(SentenceTheory::PARSE_SUBTHEORY,parse);
		st->adoptSubtheory(SentenceTheory::ENTITY_SUBTHEORY,_entitySetBuf[0]);
		st->adoptSubtheory(SentenceTheory::NAME_SUBTHEORY, names);
		st->adoptSubtheory(SentenceTheory::TOKEN_SUBTHEORY,tk->makeNoTagsTokenSequence());
		docTheory->setSentenceTheory(i, st);

		names->n_name_spans = 0;
		pnBuilder->Reset();


	}
	ResultCollector* r = _new ResultCollector(docTheory);
	r->produceAPFOutput(outfile);
	delete docTheory;
	delete pnBuilder;

	std::cout <<"done";
}





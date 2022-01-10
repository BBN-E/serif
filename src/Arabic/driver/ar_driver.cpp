// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

//#define ARABIC_LANGUAGE
#include "Generic/common/Symbol.h"
#include "Arabic/sentences/ar_SentenceBreaker.h"
#include "Generic/driver/ParseNameBuilder.h"
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
	//char* paramFile =argv[1];
	//char* filename = argv[2];
	//char* outfile =argv[3];

#ifdef ENABLE_LEAK_DETECTION
	std::cout<<"leak detection "<<std::endl;
	std::cout<<__FILE__;
#endif

	char* filename  ="c:\\SERIF\\data\\Arabic\\nltests\\test\\20000915_AFP_ARB.0034.out.sgm.nosub";
	char* outfile = "c:\\SERIF\\data\\Arabic\\nltests\\TEST34.out";
	char* paramFile ="c:\\SERIF\\data\\Arabic\\nltests\\ar_namelink.par";
	ParamReader::readParamFile(paramFile);
	//char filename[500];
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	if(argc < 1){
		std::cerr<<"ArabicSerif.exe should be called with 1 arguments - par_file"<<std::endl;
		return 0;
	}
	//ParamReader::getParam("input_file",filename, 500);
	std::cout<<filename<<std::endl;

	ReferenceResolver* _referenceResolver = _new ReferenceResolver();
	DebugStream& _debugOut = DebugStream::referenceResolverStream;
	_debugOut<<"\n--------------FILE : "<<filename<<" -----------------------\n";

	int _max_entity_sets = 1;
	DocTheory* docTheory;
	//Document Reading
	uis.open(filename);
	ArabicDocumentReader* docReader = _new ArabicDocumentReader();
	Document* doc = docReader->readDocument(uis);
	uis.close();

	docTheory = _new DocTheory(doc);
	//Sentence Breaking
	ArabicSentenceBreaker breaker;
	Sentence** sentenceSequence = _new Sentence*[MAX_SENTENCES];
	breaker.resetForNewDocument(doc);		//this should be doc
	int n_sentences = breaker.getSentences(sentenceSequence, MAX_SENTENCES, doc->getRegions(), 1);
	docTheory->setSentences(n_sentences, sentenceSequence);
	int numToks = 0;
	ArabicTokenizer* tk = _new ArabicTokenizer();


	LocatedString* sent;
	ParseNameBuilder *pnBuilder= _new ParseNameBuilder();
	for (int i = 0; i < n_sentences; i++) {
		std::cout<<"Sentence "<<i<<std::endl;
		//_debugOut<<"------------------------Start Sentence ----------------------\n";
		sent = sentenceSequence[i]->getString();
		//Don't make tokenSequence b/c with tags may be over MAX_SENT_TOK
		tk->makeTokenBuffer(sent,i);
		//build parse tree and name theories
		int currTag = 0;
		int j;
		int numToks = tk->getNToks();
		for(j=0; j<numToks; j++){
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
		EntitySet** _entitySetBuf = _new EntitySet*[_max_entity_sets];
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

		//Do actual Name Linking
		_referenceResolver->resetForNewSentence(docTheory, i);
		int _n_entity_sets = _referenceResolver->getEntityTheories( _entitySetBuf,
			_max_entity_sets, parse, results);

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





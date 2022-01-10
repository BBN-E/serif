// Copyright 2007 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/CASerif/driver/CADocumentDriver.h"
#include "Generic/CASerif/driver/CASentenceDriver.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/results/ResultCollector.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/SentenceDriver.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/sentences/SentenceBreaker.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/generics/GenericsFilter.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapStatus.h"
#include "Generic/values/DocValueProcessor.h"
#include "Generic/generics/GenericsFilter.h"
#include "Generic/common/IStringStream.h"
#include "Generic/preprocessors/Attributes.h"
#include "Generic/preprocessors/EnamexSpanCreator.h"
#include "Generic/preprocessors/HeadSpanCreator.h"
#include "Generic/preprocessors/PronounSpanCreator.h"
#include "Generic/preprocessors/DescriptorNPSpanCreator.h"
#include "Generic/preprocessors/TimexSpanCreator.h"
#include "Generic/docRelationsEvents/DocRelationEventProcessor.h"
#include "Generic/docentities/DocEntityLinker.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"

#include <string>
#include <stdio.h>
#include <iostream>
#if defined(WIN32) || defined(WIN64)
#include <direct.h>
#endif
#include <time.h>


using namespace std;
using namespace DataPreprocessor;

CADocumentDriver::CADocumentDriver(const SessionProgram *sessionProgram, ResultCollector *resultCollector) 
									: DocumentDriver(0, 0, _new CASentenceDriver())
{
	PRINT_AUGMENTED_PARSES = ParamReader::isParamTrue("print_augmented_parses");
	PRINT_NAME_TRAINING = ParamReader::isParamTrue("print_name_training");
	PRINT_DESC_TRAINING = ParamReader::isParamTrue("print_desc_training");
	PRINT_VALUE_TRAINING = ParamReader::isParamTrue("print_value_training");

	_correctAnswers = &CorrectAnswers::getInstance();

	// call beginBatch from here, rather than the DocumentDriver constructor, so
	// that the correct versions of load...Models methods get used
	beginBatch(sessionProgram, resultCollector); 
}

Document *CADocumentDriver::loadDocument(const wchar_t *document_file) {
	Document *doc = DocumentDriver::loadDocument(document_file);
	loadCAMetadata(doc);
	return doc;
}

void CADocumentDriver::loadDocEntityModels() {
	_docEntityLinker = _new DocEntityLinker(_correctAnswers->usingCorrectCoref());
	_docEntityLinker->setCorrectAnswers(_correctAnswers);
}

void CADocumentDriver::loadDocRelationsEventsModels() {
	_docRelationEventProcessor = _new DocRelationEventProcessor(
			_correctAnswers->usingCorrectRelations(),
			_correctAnswers->usingCorrectEvents());
	_docRelationEventProcessor->setCorrectAnswers(_correctAnswers);
}

void CADocumentDriver::loadCAMetadata(Document *doc) {
	CorrectDocument *correctDoc = _correctAnswers->getCorrectDocument(doc->getName());
	Metadata *metadata = doc->getMetadata();
	Symbol enamexSymbol = Symbol(L"ENAMEX");
	Symbol pronounSymbol = Symbol(L"PRONOUN");
	Symbol headSymbol = Symbol(L"HEAD");
	Symbol descSymbol = Symbol(L"DESCRIPTOR");
	Symbol timexSymbol = Symbol(L"TIMEX");

	metadata->addSpanCreator(enamexSymbol, _new EnamexSpanCreator());
	metadata->addSpanCreator(pronounSymbol, _new PronounSpanCreator());
	metadata->addSpanCreator(headSymbol, _new HeadSpanCreator());
	metadata->addSpanCreator(descSymbol, _new DescriptorNPSpanCreator());
	metadata->addSpanCreator(timexSymbol, _new TimexSpanCreator());

	for (int i = 0; i < correctDoc->getNEntities(); i++) {
		CorrectEntity *correctEnt = correctDoc->getEntity(i);
		for (int j = 0; j < correctEnt->getNMentions(); j++) {
			CorrectMention *correctMent = correctEnt->getMention(j);
			Attributes attributes(correctEnt->getEntityType()->getName(), correctEnt->getSystemEntityID());
			if (correctMent->isName()) {
				metadata->newSpan(enamexSymbol, correctMent->getHeadStartOffset(), correctMent->getHeadEndOffset(), &attributes);
				metadata->newSpan(descSymbol, correctMent->getStartOffset(), correctMent->getEndOffset(), &attributes);
			}
			else if (correctMent->isNominal()) {
				metadata->newSpan(headSymbol, correctMent->getHeadStartOffset(), correctMent->getHeadEndOffset(), &attributes);
				metadata->newSpan(descSymbol, correctMent->getStartOffset(), correctMent->getEndOffset(), &attributes);

			}
			else if (correctMent->isPronoun()) {
				metadata->newSpan(pronounSymbol, correctMent->getHeadStartOffset(), correctMent->getHeadEndOffset(), &attributes);
				metadata->newSpan(descSymbol, correctMent->getStartOffset(), correctMent->getEndOffset(), &attributes);

			}
		}
	}
	if (PRINT_VALUE_TRAINING) {
		for (int v = 0; v < correctDoc->getNValues(); v++) {
			CorrectValue *correctVal = correctDoc->getValue(v);
			Attributes attributes(correctVal->getTypeSymbol(), correctVal->getSystemValueMentionID().toInt());
			metadata->newSpan(timexSymbol, correctVal->getStartOffset(), correctVal->getEndOffset(), &attributes);
		}
	}
}

void CADocumentDriver::outputResults(const wchar_t *document_file,
								     DocTheory *docTheory,
								     const wchar_t *output_dir)
{
	_resultCollector->loadDocTheory(docTheory);
	_resultCollector->produceOutput(output_dir, document_file);
}

void CADocumentDriver::outputResults(std::wstring *results,
								   DocTheory *docTheory,
								   const wchar_t *output_dir)
{
	_resultCollector->loadDocTheory(docTheory);
	_resultCollector->produceOutput(results);
}

void CADocumentDriver::dumpDocumentTheory(DocTheory *docTheory,
										const wchar_t *document_filename)
{
	
	DocumentDriver::dumpDocumentTheory(docTheory, document_filename);

	wstring dump_dir = _sessionProgram->getDumpDir();

	if (PRINT_AUGMENTED_PARSES) {
		// OLD STYLE augmented parses
		wstringstream parse_file;
		parse_file << dump_dir << LSERIF_PATH_SEP;
		parse_file << L"aug-parses-" << wstring(document_filename) << L".txt";

		UTF8OutputStream p_out;
		p_out.open(parse_file.str().c_str());
		Symbol docname = docTheory->getDocument()->getName();
		p_out << L"(" << docname.to_string() << "\n";
		CorrectDocument *correctDoc = _correctAnswers->getCorrectDocument(docname);
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			SentenceTheory *st = docTheory->getSentenceTheory(i);
			printAugmentedParse(p_out, st, correctDoc, st->getPrimaryParse()->getRoot(), 2);
			p_out << L"\n";
		}
		p_out << L")\n";
		p_out.close();
	}
	
	if (PRINT_NAME_TRAINING){
		// IdF/PIdF sexp training
		wstringstream names_file;
		names_file << dump_dir << LSERIF_PATH_SEP;
		names_file << L"nametraining-" << wstring(document_filename) << L".txt";

		UTF8OutputStream n_out;
		n_out.open(names_file.str().c_str());
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			SentenceTheory *st = docTheory->getSentenceTheory(i);
			TokenSequence* toks = st->getTokenSequence();
			NameTheory* nt = st->getNameTheory();
			n_out <<L"(";
			int nextname = 0;
			
			int k =0;
			wchar_t buffer[100];
			while(k< toks->getNTokens()){
				Symbol name_tag = Symbol();
				if((nextname< nt->getNNameSpans() ) && 
					(k == nt->getNameSpan(nextname)->start))
				{
						NameSpan* span = nt->getNameSpan(nextname);
						wcscpy(buffer, span->type.getName().to_string());
						wcscat(buffer, L"-ST");
						n_out <<L"("<<toks->getToken(k)->getSymbol().to_string()
							<<" "<<buffer<<L") ";
						k++;
						wcscpy(buffer, span->type.getName().to_string());
						wcscat(buffer, L"-CO");
						while(k <= span->end){
							n_out <<L"("<<toks->getToken(k)->getSymbol().to_string()
							<<" "<<buffer<<L") ";
							k++;
						}
						nextname++;
		
				}
				else{
					n_out <<L"("<<toks->getToken(k)->getSymbol().to_string()<<L" NONE-ST) ";
					k++;
				}
			}
			n_out<<L")\n";
		}
		n_out.close();
	}

	if (PRINT_DESC_TRAINING){
		// IdF/PIdF sexp training
		wstringstream desc_file;
		desc_file << dump_dir << LSERIF_PATH_SEP;
		desc_file << L"desctraining-" << wstring(document_filename) << L".txt";

		UTF8OutputStream d_out;
		d_out.open(desc_file.str().c_str());
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			SentenceTheory *st = docTheory->getSentenceTheory(i);
			printDescriptorTraining(d_out, st);
		}
		d_out.close();
	}

	if (PRINT_VALUE_TRAINING){
		// IdF/PIdF sexp training
		wstringstream value_file;
		value_file << dump_dir << LSERIF_PATH_SEP;
		value_file << L"valuetraining-" << wstring(document_filename) << L".txt";

		UTF8OutputStream v_out;
		v_out.open(value_file.str().c_str());
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			SentenceTheory *st = docTheory->getSentenceTheory(i);
			printValueTraining(v_out, st);
		}
		v_out.close();
	}
}

void CADocumentDriver::printAugmentedParse(UTF8OutputStream &out, SentenceTheory *sentTheory,
										   CorrectDocument *correctDoc,
										   const SynNode *node, int indent)
{
	// OLD STYLE augmented parses

	if (node->isTerminal()) {
		out << node->getTag().to_string();
		return;
	}
	out << L"\n";
	for (int i = 0; i < indent; i++) {
		out << L" ";
	}
	out << L"(";
	bool print_mention = false;
	bool desc_mention = false;
	Symbol entID;
    if (node->hasMention()) {
		const Mention *m = sentTheory->getMentionSet()->getMentionByNode(node);
		CorrectMention *cm = correctDoc->getCorrectMentionFromMentionID(m->getUID());
		// Nested name mentions should map to CorrectMention of their parent
		if (cm == 0 && m->getMentionType() == Mention::NONE && m->getParent() != 0 &&
			node->getTag() == LanguageSpecificFunctions::getNameLabel())
		{	
			cm = correctDoc->getCorrectMentionFromMentionID(m->getParent()->getUID());
		}
		if (m->getEntityType().isRecognized() && cm != 0) {
			if ((m->getMentionType() == Mention::NAME && m->getChild() == 0) ||
				(m->getMentionType() == Mention::NONE &&
				 node->getTag() == LanguageSpecificFunctions::getNameLabel()))
			{
				print_mention = true;
				out << m->getEntityType().getName().to_string();
				if (EntitySubtype::subtypesDefined()) {
					EntitySubtype subtype = m->getEntitySubtype();
					if (m->getMentionType() == Mention::NONE && 
						!subtype.isDetermined() &&
						m->getParent() != 0)
					{
						subtype = m->getParent()->getEntitySubtype();
					}
					out << "." << subtype.getName().to_string();
				}
				out << "/NAME/";
			} else if (m->getMentionType() == Mention::PRON) {
				print_mention = true;
				out << m->getEntityType().getName().to_string();
				if (EntitySubtype::subtypesDefined())
					out << "." << m->getEntitySubtype().getName().to_string();
				out << "/PRO/";
			} else if (m->getMentionType() == Mention::DESC) {
				print_mention = true;
				desc_mention = true;
				out << m->getEntityType().getName().to_string();
				if (EntitySubtype::subtypesDefined())
					out << "." << m->getEntitySubtype().getName().to_string();
				out << "/DESC/";
			}
			if (print_mention) {
				entID = correctDoc->getCorrectEntityFromCorrectMention(cm)->getAnnotationEntityID();
			}
		}
	}
	Symbol tag = node->getTag();
	// NB: THIS IS ENGLISH SPECIFIC!
	if (print_mention && desc_mention && node->getTag() == LanguageSpecificFunctions::getNameLabel())
		out << L"NPA ";
	else out << tag.to_string() << L" ";
	if (print_mention) {
		std::wstring str = entID.to_string();
		size_t dash_e = str.find(L"-E");
		size_t dash = str.rfind(L"-");
		size_t last_non_number = str.find_last_not_of(L"0123456789");
		if (dash_e > 0 && dash_e <  str.length())
			out << "\"ID=" << str.substr(dash_e + 2) << "\" ";
		else if (dash > 0 && dash <  str.length() && dash == last_non_number)
			out << "\"ID=" << str.substr(dash + 1) << "\" ";
		else if (last_non_number > 0 && last_non_number <  str.length())
			out << "\"ID=" << str.substr(last_non_number + 1) << "\" ";
		else 
			std::cerr << "Weird entity id: " << entID.to_debug_string() << "\n";
	}
	for (int j = 0; j < node->getNChildren(); j++) {
		printAugmentedParse(out, sentTheory, correctDoc, node->getChild(j), indent + 2);
	}
	out << L")";
}

void CADocumentDriver::printDescriptorTraining(UTF8OutputStream &out, 
											 SentenceTheory *sentTheory)
{
	TokenSequence* toks = sentTheory->getTokenSequence();
	MentionSet* ms = sentTheory->getMentionSet();
	
	GrowableArray <Mention *> sortedDescs;
	// sort the mentions by head start token and weed out all but recognized DESCs
	for (int i = 0; i < ms->getNMentions(); i++) {
		Mention *ment = ms->getMention(i);
		
		if (ment->getMentionType() != Mention::DESC ||
			!ment->isOfRecognizedType())
		{
			continue;
		}

		int headStart = ment->getHead()->getStartToken();
		
		// find where the mention should be added
		bool inserted = false;
		for (int j = 0; j < sortedDescs.length(); j++) {
			if (headStart < sortedDescs[j]->getHead()->getStartToken()) {
				// move the last one down one
				sortedDescs.add(sortedDescs[sortedDescs.length()-1]);
				for (int k = sortedDescs.length()-2; k >= j; k--)
					sortedDescs[k+1] = sortedDescs[k];
				sortedDescs[j] = ment;
				inserted = true;
				break;
			}
		}
		if (!inserted)
			sortedDescs.add(ment);
	}

	out << L"( ";
	
	int nextdesc = 0;
	int k = 0;
	wchar_t buffer[100];
	while (k < toks->getNTokens()) {
		if ((nextdesc < sortedDescs.length()) && 
			(k == sortedDescs[nextdesc]->getHead()->getStartToken()))
		{	
			Mention *nextDesc = sortedDescs[nextdesc];
			wcscpy(buffer, nextDesc->getEntityType().getName().to_string());
			wcscat(buffer, L"_DESC");
			wcscat(buffer, L"-ST");
			out << L"(" << toks->getToken(k)->getSymbol().to_string()
					<< L" " << buffer << L") ";
			k++;
			wcscpy(buffer, nextDesc->getEntityType().getName().to_string());
			wcscat(buffer, L"_DESC");
			wcscat(buffer, L"-CO");
			while (k <= nextDesc->getHead()->getEndToken()) {
				out << L"(" << toks->getToken(k)->getSymbol().to_string()
					<< L" " << buffer << L") ";
				k++;
			}
			nextdesc++;

		}
		else {
			out << L"(" <<toks->getToken(k)->getSymbol().to_string() << L" NONE-ST) ";
			k++;
		}
	}
	out<< L")\n";
}

void CADocumentDriver::printValueTraining(UTF8OutputStream &out, 
										 SentenceTheory *sentTheory)
{
	TokenSequence* toks = sentTheory->getTokenSequence();
	ValueMentionSet* vms = sentTheory->getValueMentionSet();

	out << L"( ";
	
	int nextval = 0;
	int k = 0;
	wchar_t buffer[100];
	while (k < toks->getNTokens()) {

		// skip over any values that we don't care about (event only values)
		// or that we've already passed (this can happen if values are illegally nested)
		while ((nextval < vms->getNValueMentions()) && 
			   ((vms->getValueMention(nextval)->getStartToken() < k) ||
			    (vms->getValueMention(nextval)->getFullType().isForEventsOnly()))) 
		{
			nextval++;
		}

		if ((nextval < vms->getNValueMentions()) && 
			(k == vms->getValueMention(nextval)->getStartToken()))
		{	
			ValueMention *nextVal = vms->getValueMention(nextval);

			// check for nested values with same start token - we want to use the largest span
			while ((nextval + 1 < vms->getNValueMentions()) &&
				   (nextVal->getStartToken() == vms->getValueMention(nextval+1)->getStartToken()))
			{
				if ((nextVal->getEndToken() < vms->getValueMention(nextval+1)->getEndToken()) &&
					(!vms->getValueMention(nextval+1)->getFullType().isForEventsOnly()))
				{
					nextVal = vms->getValueMention(nextval+1);
				}
				nextval++;
			}

			wcscpy(buffer, nextVal->getFullType().getNicknameSymbol().to_string());
			wcscat(buffer, L"-ST");
			out << L"(" << toks->getToken(k)->getSymbol().to_string()
					<< L" " << buffer << L") ";
			k++;
			wcscpy(buffer, nextVal->getFullType().getNicknameSymbol().to_string());
			wcscat(buffer, L"-CO");
			while (k <= nextVal->getEndToken()) {
				out << L"(" << toks->getToken(k)->getSymbol().to_string()
					<< L" " << buffer << L") ";
				k++;
			}
			nextval++;
			
			
		}
		else {
			out << L"(" <<toks->getToken(k)->getSymbol().to_string() << L" NONE-ST) ";
			k++;
		}
	}
	out<< L")\n";
}







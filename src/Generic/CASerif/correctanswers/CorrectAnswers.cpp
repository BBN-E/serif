// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/NestedNameSpan.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/parse/ParserTags.h"
#include <boost/scoped_ptr.hpp>


/*#ifndef WIN32
#include <netinet/in.h>
#endif
#include <fcntl.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <DEFT/adept-thrift-cpp/Serializer.h>
#include "DEFTTester.h"


using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace thrift::adept::serialization;
using namespace thrift::adept::common; 

using namespace boost;
*/


// Only needed when testing the DescriptorLinker(see getEntityTheories)
//#include "edt/StatDescLinker.h"
//#include "edt/PreLinker.h"

using namespace std;

/* As of 9/7/05, correct answers are loaded from a file that looks like this:
(
(CNN_CF_20030304
  (Entities
    (PER Individual SPC FALSE CNN_CF_20030304-E1
      (NAME NONE 1106 1117 1106 1117 CNN_CF_20030304-E1-4)
      (PRONOUN NONE 1124 1124 1124 1124 CNN_CF_20030304-E1-31)
    )
  )
  (Values
	(Numeric.Money CNN_CF_20030304-V1
      (CNN_CF_20030304-V1-1 253 260)
    )
	(TIMEX2 CNN_CF_20030304-T1
      (CNN_CF_20030304-T1-1 371 375)
    )
  )
  (Relations
    (PART-WHOLE.Geographical EXPLICIT CNN_CF_20030304-R1
      (CNN_CF_20030304-E21-60 CNN_CF_20030304-E22-61 
	   CNN_CF_20030304-T1-1 CNN_CF_20030304-R1-1)
	)
  )
  (Events
    (Conflict.Attack Other Specific Future Positive CNN_CF_20030304.1900.06-2-EV2
      (CNN_CF_20030304.1900.06-2-EV2-1 1217 1222 1217 1230
        (CNN_CF_20030304.1900.06-2-E6-40 Place)
      )
      (CNN_CF_20030304.1900.06-2-EV2-2 1485 1487 1476 1487
        (CNN_CF_20030304.1900.06-2-E4-43 Attacker)
      )
    )
  )
)
)
*/

CorrectAnswers::CorrectAnswers() {
	cout << "Loading Correct Answers...\n";

	char buffer[CORRECT_ANSWER_PARAM_BUFFER_SIZE];
	_debug = ParamReader::getNarrowParam(buffer, Symbol(L"correct_answer_debug"), CORRECT_ANSWER_PARAM_BUFFER_SIZE);
	if (_debug) {
		_debugStream.open(buffer);
	}

	ParamReader::getRequiredNarrowParam(buffer, Symbol(L"correct_answer_file"), CORRECT_ANSWER_PARAM_BUFFER_SIZE);

	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build(buffer));
	UTF8InputStream& instream(*instream_scoped_ptr);
	Sexp *correctAnswerSexp = new Sexp(instream);

	if (!correctAnswerSexp->isList())
		throw UnexpectedInputException("CorrectAnswers::CorrectAnswers()",
									   "correctAnswerSexp does not contain a list of documents");

	_n_documents = correctAnswerSexp->getNumChildren();
	_documents = _new CorrectDocument[_n_documents];

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_relations")))
		_use_correct_relations = true;
	else _use_correct_relations = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_events")))
		_use_correct_events = true;
	else _use_correct_events = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_subtypes")))
		_use_correct_subtypes = true;
	else _use_correct_subtypes = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_types")))
		_use_correct_types = true;
	else _use_correct_types = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_coref")))
		_use_correct_coref = true;
	else _use_correct_coref = false;
	_add_ident_relations = false;
	if(ParamReader::getParam("add_ident_relations", buffer, CORRECT_ANSWER_PARAM_BUFFER_SIZE)){
		if(strcmp(buffer, "true") == 0){
			_add_ident_relations = true;
		}
	}

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"unify_appositives")))
		_unify_appositives = true;
	else _unify_appositives = false;

	// allow some event types to have arguments with UNDET entity-type
	std::vector<Symbol> allowUndetVec = ParamReader::getSymbolVectorParam("eventTypesAllowUndet");
     	for(unsigned i=0; i<allowUndetVec.size(); i++) {
		_eventTypesAllowUndet.insert(allowUndetVec[i]);
	}

	int i;
	for (i = 0; i < _n_documents; i++) {
		Sexp* docSexp = correctAnswerSexp->getNthChild(i);
		_documents[i].loadFromSexp(docSexp);
	}

	_documentEntitySet = NULL;
	delete correctAnswerSexp;

	//_descLinker = _new StatDescLinker();
}

/*CorrectAnswers::CorrectAnswersAdept() {
	cout << "Loading Correct Answers From ERE Data...\n";

    shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
    shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    SerializerClient client(protocol);

	char buffer[500];
	_debug = ParamReader::getNarrowParam(buffer, Symbol(L"correct_answer_debug"), 256);
	if (_debug) {
		_debugStream.open(buffer);
	}

	 ParamReader::getRequiredNarrowParam(buffer, Symbol(L"correct_answer_file_list"), 256);
    cout << buffer <<"\n";
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build(buffer));
	UTF8InputStream& tempFileListStream(*instream_scoped_ptr);
	
	UTF8Token token;
	std::vector<HltContentContainer*> HltContainers;

    while(!tempFileListStream.eof()) {
		tempFileListStream >> token;
		
		std::wstring wideFile = token.chars();
		std::string narrowFile( wideFile.begin(), wideFile.end() );
		cout << narrowFile << "\n";
		boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build(narrowFile.c_str()));
	    UTF8InputStream& instream(*instream_scoped_ptr);
		LocatedString source(instream, false);
	    const LocatedString* text= &source;
	
	    wstring y=text->toWString();
	    string simpleString;
	    simpleString.assign(y.begin(), y.end());
		HltContentContainer	temp;
	 
        client.deserializeString(temp,simpleString);
		HltContainers.push_back(&temp);
	     
	}
	
	_documents = _new CorrectDocument[HltContainers.size()];

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_relations")))
		_use_correct_relations = true;
	else _use_correct_relations = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_events")))
		_use_correct_events = true;
	else _use_correct_events = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_subtypes")))
		_use_correct_subtypes = true;
	else _use_correct_subtypes = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_types")))
		_use_correct_types = true;
	else _use_correct_types = false;

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"use_correct_coref")))
		_use_correct_coref = true;
	else _use_correct_coref = false;
	_add_ident_relations = false;
	if(ParamReader::getParam("add_ident_relations", buffer, 500)){
		if(strcmp(buffer, "true") == 0){
			_add_ident_relations = true;
		}
	}

	if (ParamReader::getRequiredTrueFalseParam(Symbol(L"unify_appositives")))
		_unify_appositives = true;
	else _unify_appositives = false;

	int i;
	for (i = 0; i < HltContainers.size(); i++) {
		_documents[i].loadFromAdept(HltContainers[i]);
	}

	_documentEntitySet = NULL;

	//_descLinker = _new StatDescLinker();
}*/

CorrectAnswers& CorrectAnswers::getInstance() {
	static CorrectAnswers instance;
	return instance;
}

/*CorrectAnswers& CorrectAnswers::getInstanceAdept() {
	static CorrectAnswers instance;
	instance.CorrectAnswersAdept();
	return instance;
}
*/

CorrectAnswers::~CorrectAnswers() { 
	delete [] _documents;
	delete _documentEntitySet;
	//delete _descLinker;
}


int CorrectAnswers::getNameTheories(NameTheory **results, 
									TokenSequence *tokenSequence, 
									Symbol docName) 

{
	// just in case this is our first stage of processing
	setSentAndTokenNumbersOnCorrectMentions(tokenSequence, docName);

	CorrectDocument *correctDocument = getCorrectDocument(docName);
	int num_tokens = tokenSequence->getNTokens();
	
	// First go through and choose which CorrectMentions we are going 
	// to treat as names for this sentence
	std::vector<CorrectMention *> possibleNames;

	int i, j;
	for (i = 0; i < correctDocument->getNEntities(); i++) {
		CorrectEntity *correctEntity = correctDocument->getEntity(i);

		for (j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention *correctMention = correctEntity->getMention(j);
			if (correctMention->getSentenceNumber() != tokenSequence->getSentenceNumber())
				continue;
			
			if (correctMention->getMentionType() == Mention::NAME)
			{
				possibleNames.push_back(correctMention);
			}
		}
	}
				
	// Any mention that is nested inside another name should be treated as a DESC
	// Back when we used to change DESCs into NAMES in the above loop, this sometimes
	// happened. I doubt it will happen now, but we'll still check for it, why not.
	for (size_t a = 0; a < possibleNames.size(); a++) {
		CorrectMention *cm1 = possibleNames[a];
		for (size_t b = 0; b < possibleNames.size(); b++) {
			CorrectMention *cm2 = possibleNames[b];
			if (cm1 == cm2) continue;
			if (cm1->getMentionType() != Mention::NAME || cm2->getMentionType() != Mention::NAME) continue;
			if ((cm1->getHeadStartToken() >= cm2->getHeadStartToken() && cm1->getHeadStartToken() <= cm2->getHeadEndToken()) ||
				(cm1->getHeadEndToken() >= cm2->getHeadStartToken() && cm1->getHeadEndToken() <= cm2->getHeadEndToken()))
			{	
				cm1->setMentionType(Mention::DESC);
				if (_debug) {
					_debugStream << "DOC: " << docName.to_debug_string();
					_debugStream << " SENT: " << tokenSequence->getSentenceNumber() << "\n";
					_debugStream << "Nested \"names\": " << cm1->getAnnotationID() << " and " << cm2->getAnnotationID() << "\n";
					_debugStream << "   --> changed " << cm1->getAnnotationID() << " to DESC\n";
				}
			}
		}
	}

	std::vector<NameSpan*> nameSpans;
	for (i = 0; i < correctDocument->getNEntities(); i++) {
		CorrectEntity *ce = correctDocument->getEntity(i);

		for (j = 0; j < ce->getNMentions(); j++) {
			CorrectMention *cm = ce->getMention(j);
			if (cm->getSentenceNumber() != tokenSequence->getSentenceNumber())
				continue;

			if (cm->getMentionType() == Mention::NAME) {
				EntityType *entityType = ce->getEntityType();
				int start = cm->getHeadStartToken();
				int end = cm->getHeadEndToken();
				NameSpan *nameSpan = _new NameSpan(start, end, *entityType);

				// insert in the correct order
				bool inserted = false;
				for (vector<NameSpan*>::iterator it = nameSpans.begin(); it != nameSpans.end(); ++it) {
					if (nameSpan->start < (*it)->start) {
						nameSpans.insert(it, nameSpan);
						inserted = true;
						break;
					}
				}
				if (!inserted)
					nameSpans.push_back(nameSpan);
			}
		}
	}

	results[0] = _new NameTheory(tokenSequence, nameSpans);
	return 1;
}

// TODO , added 03/03/2015 : place-holder, so that CASentenceDriver can use this.
int CorrectAnswers::getNestedNameTheories(NestedNameTheory **results, TokenSequence *tokenSequence, Symbol docName, NameTheory *nameTheory) {
	std::vector<NestedNameSpan*> nestedNameSpans;

	results[0] = _new NestedNameTheory(tokenSequence, nameTheory, nestedNameSpans);
	return 1;
}

int CorrectAnswers::getValueTheories(ValueMentionSet **results, 
									TokenSequence *tokenSequence, 
									Symbol docName) 

{
	// just in case this is our first stage of processing
	setSentAndTokenNumbersOnCorrectMentions(tokenSequence, docName);

	CorrectDocument *correctDocument = getCorrectDocument(docName);
	
	// First go through and choose which CorrectMentions we are going 
	// to treat as names for this sentence
	std::vector<CorrectValue *> possibleValues;

	int i;
	for (i = 0; i < correctDocument->getNValues(); i++) {
		CorrectValue *correctValue = correctDocument->getValue(i);
		if (correctValue->getSentenceNumber() != tokenSequence->getSentenceNumber())
			continue;
		if (correctValue->getStartToken() < 0 || correctValue->getEndToken() < 0) {
			SessionLogger::warn("invalid_ca_value_offsets") 
					<< "Ignoring CorrectValue with invalid offsets: "
					<< correctValue->to_string();
			continue;
		}
		possibleValues.push_back(correctValue);
	}

	// sort the values (not sure if this is necessary...)
	for (size_t a = 0; a < possibleValues.size(); a++) {
		for (size_t b = 0; b < a; b++) {
			if (possibleValues[a]->getStartToken() < possibleValues[b]->getStartToken()) {
				CorrectValue *temp = possibleValues[a];
				for (size_t c = a; c > b; c--) {
					possibleValues[c] = possibleValues[c-1];
				}
				possibleValues[b] = temp;
				break;
			}
		}
	}

	// now we've gone through all the values in the document, make ValueMentions out of them 
	// (and make sure you then tell the CorrectValue where it is pointing to)
	ValueMentionSet *valueSet;
	valueSet = _new ValueMentionSet(tokenSequence, static_cast<int>(possibleValues.size()));	

	for (i = 0; i < static_cast<int>(possibleValues.size()); i++) {
		// NOT wrapping this in a try block; if it fails due to too many value mentions
		//   in a sentence, the system will crash, but that might be correct?
		ValueMention::UID uid = ValueMention::makeUID(tokenSequence->getSentenceNumber(), i);				
		ValueMention *val = _new ValueMention(tokenSequence->getSentenceNumber(),
			uid, possibleValues[i]->getStartToken(),
			possibleValues[i]->getEndToken(),
			possibleValues[i]->getTypeSymbol());
		valueSet->takeValueMention(i, val);

		// this will have to be reset later, in case we restart at the mentions stage
		possibleValues[i]->setSystemValueMentionID(val->getUID());		
	}

	results[0] = valueSet;
	return 1;
}

void CorrectAnswers::hookUpCorrectValuesToSystemValues(int sentno,
													   ValueMentionSet *valueMentionSet, 
													   Symbol docName)
{
	CorrectDocument *correctDocument = getCorrectDocument(docName);
	
	int i,j;
	for (i = 0; i < correctDocument->getNValues(); i++) {
		CorrectValue *correctValue = correctDocument->getValue(i);
		if (correctValue->getSentenceNumber() != sentno)
			continue;

		for (j = 0; j < valueMentionSet->getNValueMentions(); j++) {
			ValueMention *vm = valueMentionSet->getValueMention(j);
			if (vm->getStartToken() == correctValue->getStartToken() &&
				vm->getEndToken() == correctValue->getEndToken())
				correctValue->setSystemValueMentionID(vm->getUID());
		}		
	}
}

void CorrectAnswers::assignCorrectTimexNormalizations(DocTheory* docTheory) {
	CorrectDocument *correctDocument = getCorrectDocument(docTheory->getDocument()->getName());
	ValueSet *vs = docTheory->getValueSet();

	int i,j;
	for (i = 0; i < correctDocument->getNValues(); i++) {
		CorrectValue *correctValue = correctDocument->getValue(i);
		if (!correctValue->isTimexValue())
			continue;

		for (j = 0; j < vs->getNValues(); j++) {
			Value *v = vs->getValue(j);
			if (!v->isTimexValue()) continue;
		
			if (correctValue->getSystemValueMentionID() == v->getValMentionID()) {
				v->setTimexVal(correctValue->getTimexVal());
				v->setTimexAnchorVal(correctValue->getTimexAnchorVal());
				v->setTimexAnchorDir(correctValue->getTimexAnchorDir());
				v->setTimexSet(correctValue->getTimexSet());
				v->setTimexMod(correctValue->getTimexMod());
				v->setTimexNonSpecific(correctValue->getTimexNonSpecific());
			}
		}
	}
}

void CorrectAnswers::assignSynNodesToCorrectMentions(int sentence_number, Parse *parse, 
													Symbol docName) 
{
	// assumes sentence/token numbers are set on the mentions (done in CASentenceDriver)

    CorrectDocument *correctDocument = getCorrectDocument(docName);
	const SynNode *parseRoot = parse->getRoot();
	
	// for each CorrectMention, we want to set the field bestHeadSynNode
	//  we essentially take either a node that is equal to the span of the head,
	//  or, failing that, a node that has the same head token as the head
	// we search from bottom up, so we should get the lowest such non-terminal node
	for (int i = 0; i < correctDocument->getNEntities(); i++) {
		CorrectEntity *correctEntity = correctDocument->getEntity(i);
		for (int j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention *correctMention = correctEntity->getMention(j);
			if (correctMention->getSentenceNumber() != sentence_number)
				continue;
			if (_debug) {
				_debugStream << "DOC: " << docName.to_debug_string();
				_debugStream << " SENT: " << sentence_number << "\n";
				_debugStream << " Extent (tokens): " << correctMention->getStartToken() << "-->";
				_debugStream << correctMention->getEndToken() << " Head Extent: ";
				_debugStream << correctMention->getHeadStartToken() << "-->";
				_debugStream << correctMention->getHeadEndToken() << "\n";
			}
			correctMention->setBestHeadSynNode(getBestNodeMatch(correctMention, parseRoot));
		}
	}
}

// utility called by assignSynNodesToCorrectMentions
const SynNode *CorrectAnswers::getBestNodeMatch(CorrectMention *correctMention, 
												const SynNode *root) 
{
	const SynNode *bestMatch = getBestNodeHeadMatch(correctMention, root);
	if (bestMatch != 0 && _debug)
		_debugStream << "Found head match: " << bestMatch->toTextString() << "\n";
	
	if (bestMatch == 0) {
		bestMatch = getBestNodeHeadTokenMatch(correctMention, root);
		if (bestMatch != 0 && _debug) {
			_debugStream << "Found head token match: " << bestMatch->toTextString() << "\n";
			_debugStream << root->toTextString() << "\n";
			_debugStream << root->toPrettyParse(0) << "\n";
		}

	}

	if (bestMatch == 0 && _debug) {
		_debugStream << "***NOT found***\n";
		_debugStream << "Parse: " << root->toTextString() << "\n";
	}

	if (_debug) _debugStream << "\n";			

	return bestMatch;
}

// utility called by getBestNodeMatch
const SynNode *CorrectAnswers::getBestNodeHeadMatch(CorrectMention *correctMention, 
												const SynNode *node) 
{
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *bestMatch = getBestNodeHeadMatch(correctMention, node->getChild(i));
		if (bestMatch != 0)
			return bestMatch;
	}

	if (node->isTerminal())
		return 0;

	if (correctMention->getHeadStartToken() == node->getStartToken() && 
		correctMention->getHeadEndToken() == node->getEndToken())
		return node;
	else return 0;
}

// utility called by getBestNodeMatch
const SynNode *CorrectAnswers::getBestNodeHeadTokenMatch(CorrectMention *correctMention, 
												const SynNode *node) 
{
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *bestMatch = getBestNodeHeadTokenMatch(correctMention, node->getChild(i));
		if (bestMatch != 0)
			return bestMatch;
	}

	if (node->isTerminal())
		return 0;

	if (LanguageSpecificFunctions::matchesHeadToken(node, correctMention))
		return node;
	else return 0;
}

/* Correct the mentions and create a map between predicted mention and correct mentions
 * if use_correct_types is false the map is created by mention->getEntityType() and 
 * mention->getEntitySubtype() are not corrected.
 */
int CorrectAnswers::correctMentionTheories(MentionSet *results[], int sentence_number,
									       TokenSequence *tokenSequence, Symbol docName
										   , bool use_correct_types) 
{
	CorrectDocument *correctDocument = getCorrectDocument(docName);
	MentionSet * startingSet = results[0];
	
	// Clear out all entity types of mentions
	// For NAMEs, DESCs, and PRONs, clear out mention type
	// mention types aren't being cleared, is that correct?
	int index;
	for (index = 0; index < startingSet->getNMentions(); index++) {
		Mention * mention = startingSet->getMention(index);

		if (use_correct_types) {
			mention->setEntityType(EntityType::getOtherType());
			mention->setEntitySubtype(EntitySubtype::getUndetType());
		}
	}
	
	// Find the best mention for each CorrectMention:
	//   We do this by finding a real mention (NAME, DESC, PRON, PART) whose
	//   node is headed by the CorrectMention's bestHeadSynNode. There really
	//   ought only to be one of these for any given bestHeadSynNode, if the
	//   MentionSet is constructed correctly (as I understand it). 
	//   Also, two CorrectMentions should never share the same bestHeadSynNode,
	//   since overlapping heads are ACE-illegal. In the case of bad
	//   annotation (or metonymy, sigh), we will lose one of the correct mentions.
	//   This is important, because getCorrectMentionFromMentionID() should be
	//   well-defined, which it would not be if two CorrectMentions shared a mention!
	//
	//	In Chinese there are often nested list type structures that allow a larger
	//	  node to mistakenly appear as if it's headed by the final element of the 
	//	  list.  We want to prevent the bestHeadSynNode for the list element from
	//	  being mapped to the larger mention, so we need to disallow LISTs in the
	//	  head chain.  JSM 09/01/2005
	//
	int i, j;
	for (i = 0; i < correctDocument->getNEntities(); i++) {
		CorrectEntity *correctEntity = correctDocument->getEntity(i);

		EntityType *entityType = correctEntity->getEntityType();
		EntitySubtype entitySubtype = correctEntity->getEntitySubtype();

		for (j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention *correctMention = correctEntity->getMention(j);
			if (correctMention->getSentenceNumber() != sentence_number)
				continue;
			if(_debug) {
				_debugStream << "DOC: " << docName.to_debug_string();
				_debugStream << " SENT: " << sentence_number << "\n";
				_debugStream << "Looking for NP for " << entityType->getName().to_debug_string();
				if (correctMention->getMentionType() == Mention::NAME) 
					_debugStream << " NAME";
				else if (correctMention->getMentionType() == Mention::DESC)
					_debugStream << " DESC";
				else if (correctMention->getMentionType() == Mention::PRON)
					_debugStream << " PRON";
				_debugStream << "\n Extent (tokens): " << correctMention->getStartToken() << "-->";
				_debugStream << correctMention->getEndToken() << " Head Extent: ";
				_debugStream << correctMention->getHeadStartToken() << "-->";
				_debugStream << correctMention->getHeadEndToken() << "\n";
				_debugStream << " Extent (offsets): " << correctMention->getStartOffset() << "-->";
				_debugStream << correctMention->getEndOffset() << " Head Extent: ";
				_debugStream << correctMention->getHeadStartOffset() << "-->";
				_debugStream << correctMention->getHeadEndOffset() << "\n";
				_debugStream << " ID: " << correctMention->getAnnotationID().to_string() << "\n";
				int l;
				_debugStream <<" Mention Text: ";
				for (l = correctMention->getStartToken(); l <= correctMention->getEndToken(); l++) 
					_debugStream << tokenSequence->getToken((int)l)->getSymbol().to_debug_string() << " ";
				_debugStream << "\n";
				_debugStream <<" Mention Head Text: ";
				for (l = correctMention->getHeadStartToken(); l <= correctMention->getHeadEndToken(); l++) 
					_debugStream << tokenSequence->getToken((int)l)->getSymbol().to_debug_string() << " ";
				_debugStream << "\n";
			}

			bool match_found = false;
			for (int mentid = 0; mentid < startingSet->getNMentions(); mentid++) {
				Mention *mention = startingSet->getMention(mentid);
				if (mention->getMentionType() != Mention::NAME &&
					mention->getMentionType() != Mention::DESC &&
					mention->getMentionType() != Mention::PRON &&
					mention->getMentionType() != Mention::PART)
					continue;

				const SynNode *node = mention->getNode();
				while (!node->isTerminal()) {
					if (node == correctMention->getBestHeadSynNode())
						match_found = true;
					node = node->getHead();
					// check to make sure the next node in the chain isn't a list
					if (node->getMentionIndex() != -1) {
						Mention *m = startingSet->getMention(node->getMentionIndex());
						if (m->getMentionType() == Mention::LIST)
							break;
					}
				}

				if (match_found && (mention->getEntityType() != EntityType::getOtherType() && use_correct_types)) {
					if(_debug){
						_debugStream <<" Overlapping heads? This mention already has a "
							<<"correct mention attached to it: "
							<<mention->getNode()->toDebugString(0).c_str()
							<<"\n\n";
					}
					match_found = false;
				}

				// TB: the mapping may be different with different values of use_correct_types
				// but I don't have time to implement this better.
				if (match_found) {					
					if (use_correct_types || !mention->isOfRecognizedType()) {
						mention->setEntityType(*entityType);
						mention->setEntitySubtype(entitySubtype);
					}
					correctMention->addSystemMentionId(mention->getUID());
					if(_debug){
						_debugStream <<" Match Correct Mention to Mention: "
							<<mention->getNode()->toDebugString(0).c_str()
							<<"\n\n";
					}
					break;
				}
			}
			if (!match_found && _debug) {
				_debugStream << "\nNo match found!\n";
			}
			
		}
	}

	// "Make sure all children of appositives are in the same entity as their parent."
	// This seems to be for the cases where a child of an appositive has not been
	// assigned a CorrectMention: you should give it its parent's ID.
	// I am struggling to think of why this would happen. TODO: test it out
	for (index = 0; (int)index < startingSet->getNMentions(); index++) {
		Mention * mention = startingSet->getMention(index);
		if (mention->getParent() != NULL &&
			mention->getParent()->getMentionType() == Mention::APPO &&
			correctDocument->getCorrectMentionFromMentionID(mention->getUID()) == NULL) 
		{
			CorrectMention *parentsCM = 
				correctDocument->getCorrectMentionFromMentionID(mention->getParent()->getUID());
			if (parentsCM != NULL) {
				parentsCM->addSystemMentionId(mention->getUID());
				mention->setEntityType(mention->getParent()->getEntityType());
			}
		}
	}

	// I think we ought to promote the type of an appositive's members up to the appositive
	// TODO: should we promote the type of a list's members up to the list?
	for (index = 0; (int)index < startingSet->getNMentions(); index++) {
		Mention * mention = startingSet->getMention(index);
		if (mention->getMentionType() == Mention::APPO && mention->getChild() != 0)
		{
			mention->setEntityType(mention->getChild()->getEntityType());
			// also, if we are unifying appositives, I think the appositive mention
			// should get linked to the same CorrectMention as its first child.
			// (Is that correct? Or should it be the named child?)
			if (_unify_appositives) {
				CorrectMention *childCM = 
					correctDocument->getCorrectMentionFromMentionID(mention->getChild()->getUID());
				if (childCM != NULL) 
					childCM->addSystemMentionId(mention->getUID());				
			}
		}
	}

	// Make sure all nested children of names and descriptors get the correct type.
	for (index = 0; (int)index < startingSet->getNMentions(); index++) {
		Mention * mention = startingSet->getMention(index);
		if (mention->getMentionType() == Mention::NONE  && mention->getParent() != 0) {
			mention->setEntityType(mention->getParent()->getEntityType());
			mention->setEntitySubtype(mention->getParent()->getEntitySubtype());
		}
	}


	return 1;		

}

// This is fairly straightforward. We know what CorrectMention each
// mention corresponds to, so we know which entity it should belong to.
// It's just a matter of walking through and constructing the EntitySet.
EntitySet *CorrectAnswers::getEntityTheory(const MentionSet *mentionSet, 
					  size_t n_sentences,
					  Symbol docName)

{
	CorrectDocument *correctDocument = getCorrectDocument(docName);

	if (_documentEntitySet == NULL) {
		_documentEntitySet = new EntitySet(static_cast<int>(n_sentences));
	}
	_documentEntitySet->loadMentionSet(mentionSet);

	int i;
	for (i = 0; i < mentionSet->getNMentions(); i++) {
		Mention* mention = mentionSet->getMention(i);
		CorrectMention *correctMention 
			= correctDocument->getCorrectMentionFromMentionID(mention->getUID());
		if (!mention->getEntityType().isRecognized() || correctMention == 0)
			continue;

		CorrectEntity *correctEntity = correctDocument->getCorrectEntityFromCorrectMention(correctMention);

		if (_debug) {
			_debugStream << "ENTITIES STAGE: \n";
			_debugStream << "Working with " << mention->node->toDebugString(0).c_str() << "\n";
			_debugStream << mention->getUID() << L" --> " << correctMention->getAnnotationID().to_string();
			_debugStream << "\n";
			_debugStream << "CorrectEntity has " << (int)correctEntity->getNMentions() << " mentions\n";
		}

		int entity_id = correctEntity->getSystemEntityID();

		if (entity_id != -1) {
			_documentEntitySet->add(mention->getUID(), entity_id);
			if (_debug) {
				_debugStream << "adding to " << entity_id << "\n\n";
			}
		}
		else {
			_documentEntitySet->addNew(mention->getUID(), mention->getEntityType());
			Entity * newEntity = _documentEntitySet->getEntityByMention(mention->getUID());
			correctEntity->setSystemEntityID(newEntity->getID());
			if (_debug) {
				_debugStream << "Made new entity " << newEntity->getID() << "\n\n";
			}
		}
		
	}
	// Make a copy to keep reference counting straight
	EntitySet *eset = _new EntitySet(*_documentEntitySet);
	return eset;
}

/* //Alternate version of getEntityTheories - more closely follows the version in the regular
 //  Serif path.  Useful for testing the desc linker on correct mentions. - JCS 3/26/04 
int CorrectAnswers::getEntityTheories(EntitySet *results[], 
					  const MentionSet *mentionSet, 
					  size_t n_sentences,
					  Symbol docName,
					  PropositionSet *propSet)

{
	CorrectDocument *correctDocument = getCorrectDocument(docName);
	
	if (mentionSet->getSentenceNumber() == 0) 
		_descLinker->resetForNewDocument(docName);
	_descLinker->resetForNewSentence();

	if (_documentEntitySet == NULL) {
		_documentEntitySet = new EntitySet(n_sentences);
	}
	_documentEntitySet->loadMentionSet(mentionSet);

	// set up pre-linked mention array
	const Mention **preLinks = _new const Mention*[mentionSet->getNMentions()];
	for (int i = 0; i < mentionSet->getNMentions(); i++)
		preLinks[i] = 0;

	PreLinker::preLinkAppositives(preLinks, mentionSet);
	PreLinker::preLinkCopulas(preLinks, mentionSet, propSet);
	PreLinker::preLinkSpecialCases(preLinks, mentionSet, propSet);

	GrowableArray <int> names = GrowableArray <int> (mentionSet->getNMentions());
	GrowableArray <int> descriptors = GrowableArray <int> (mentionSet->getNMentions());
	GrowableArray <int> pronouns = GrowableArray <int> (mentionSet->getNMentions());

	for (int j = 0; j < mentionSet->getNMentions(); j++) {
		if (preLinks[j] == 0) {
			Mention *thisMention = mentionSet->getMention(j);
			if (thisMention->mentionType == Mention::NAME) 
				names.add(thisMention->getUID());
			else if (thisMention->mentionType == Mention::DESC)
				descriptors.add(thisMention->getUID());
			else if (thisMention->mentionType == Mention::PRON)
				pronouns.add(thisMention->getUID());
		}
	}

	linkMentions(names, mentionSet, correctDocument);
	linkPreLinks(_documentEntitySet, mentionSet, preLinks, correctDocument, false);
	linkMentions(descriptors, mentionSet, correctDocument);
	linkPreLinks(_documentEntitySet, mentionSet, preLinks, correctDocument, false);
	linkMentions(pronouns, mentionSet, correctDocument);
	linkPreLinks(_documentEntitySet, mentionSet, preLinks, correctDocument, true);

	delete [] preLinks;

	results[0] = _documentEntitySet;
	return 1;
}

void CorrectAnswers::linkMentions(GrowableArray <int> &mentions, 
								  const MentionSet *mentionSet, 
								  CorrectDocument *correctDocument) 
{
	 size_t i;
	for (i = 0; (int)i < mentions.length(); i++) {
		Mention* mention = mentionSet->getMention(mentions[i]);
		CorrectMention *correctMention = correctDocument->getCorrectMentionFromMentionID(mention->getUID());

		if (!mention->getEntityType().isRecognized()) {
			continue;
		}

		CorrectEntity *correctEntity = correctDocument->getCorrectEntityFromCorrectMention(correctMention);

		if (_debug) {
			_debugStream << "ENTITIES STAGE: \n";
			_debugStream << "Working with " << mention->node->toDebugString(0).c_str() << "\n";
			_debugStream << "CorrectEntity has " << (int)correctEntity->getNMentions() << " mentions\n";
		}

		int entity_id = correctEntity->getEntityID();

		if (mention->getMentionType() == Mention::DESC) {
			_descLinker->correctAnswersLinkMention(_documentEntitySet, mention->getUID(), mention->getEntityType());
			_descLinker->printCorrectAnswer(entity_id);
		}

		if (entity_id != -1) {
			_documentEntitySet->add(mention->getUID(), entity_id);
			if (_debug) {
				_debugStream << "adding to " << entity_id << "\n\n";
			}
		}
		else {
			_documentEntitySet->addNew(mention->getUID(), mention->getEntityType());
			Entity * newEntity = _documentEntitySet->getEntityByMention(mention->getUID());
			correctEntity->setEntityID(newEntity->getID());
			if (_debug) {
				_debugStream << "Made new entity " << newEntity->getID() << "\n\n";
			}
		}

	}
}

void CorrectAnswers::linkPreLinks(EntitySet *entitySet,
								  const MentionSet *mentionSet,
								  const Mention **preLinks,
								  CorrectDocument *correctDocument,
								  bool final)
{
	int n_mentions = mentionSet->getNMentions();

	for (int i = 0; i < n_mentions; i++) {
		if ((preLinks[i] &&
			// only link if the two mentions are not both names -- linking
			// names is dangerous, and should only be attempted by the
			// namelinker -- SRS 
			!(mentionSet->getMention(i)->getMentionType() == Mention::NAME &&
			  preLinks[i]->getMentionType() == Mention::NAME)) ||
			 (preLinks[i] && final))
		{
			Mention *mention = mentionSet->getMention(i);
			Entity *entity = entitySet->getEntityByMention(
											preLinks[i]->getUID());
			
			if (entity || final) {

				if (mention->getMentionType() == Mention::APPO)
					continue;

				CorrectMention *correctMention = correctDocument->getCorrectMentionFromMentionID(mention->getUID());

				CorrectEntity *correctEntity = correctDocument->getCorrectEntityFromCorrectMention(correctMention);

				if (_debug) {
					_debugStream << "ENTITIES STAGE: \n";
					_debugStream << "Working with " << mention->node->toDebugString(0).c_str() << "\n";
					_debugStream << "CorrectEntity has " << (int)correctEntity->getNMentions() << " mentions\n";
				}

				int entity_id = correctEntity->getEntityID();

				if (entity && entity->getID() != entity_id && _debug)
					_debugStream << "CorrectEntity for Mention " << mention->getUID() << " does not match prelink.\n";


				if (entity_id != -1) {
					entitySet->add(mention->getUID(), entity_id);
					if (_debug) {
						_debugStream << "adding to " << entity_id << "\n\n";
					}
				}
				else {
					entitySet->addNew(mention->getUID(), mention->getEntityType());
					Entity * newEntity = entitySet->getEntityByMention(mention->getUID());
					correctEntity->setEntityID(newEntity->getID());
					if (_debug) {
						_debugStream << "Made new entity " << newEntity->getID() << "\n\n";
					}
				}

				// now make sure we don't try to link this again
				preLinks[i] = 0;
			}
			
			
		}
	}
}*/

RelMentionSet *CorrectAnswers::correctRelationTheory(const Parse *parse, MentionSet *mentionSet,
												 EntitySet *entitySet, const PropositionSet *propSet,
												 ValueMentionSet *valueMentionSet, Symbol docName)
{
	CorrectDocument *correctDocument = getCorrectDocument(docName);
	RelMentionSet *result = _new RelMentionSet();

	int i, j, k;
	for (i = 0; i < correctDocument->getNRelations(); i++) {
		CorrectRelation *correctRelation = correctDocument->getRelation(i);

		if (!correctRelation->isExplicit())
			continue;

		for (j = 0; j < correctRelation->getNMentions(); j++) {
			CorrectRelMention *correctMention = correctRelation->getMention(j);

			bool foundMatch = correctMention->setMentionArgs(mentionSet, correctDocument);				
			if (!foundMatch) continue;

			correctMention->setTimeArg(valueMentionSet, correctDocument);

			RelMention *r = _new RelMention(correctMention->getLeftMention(), 
							correctMention->getRightMention(), correctMention->getType(), 
							correctMention->getSentenceNumber(), result->getNRelMentions(), 1.0);
			r->setTimeArgument(correctMention->getTimeRole(), 
															  correctMention->getTimeArg());
			r->setModality(correctRelation->getModality());
			r->setTense(correctRelation->getTense());
			result->takeRelMention(r);
		}

	}	

	//Add an identity relation for any pair of mentions that are coreferent in the sentence
	if(_add_ident_relations == true){
		if(mentionSet->getNMentions() > 0){
			//hacky way of getting the current sentence #
			int sent_num  = mentionSet->getMention(0)->getSentenceNumber();
			for(i = 0; i< entitySet->getNEntities(); i++){
				Entity* ent = entitySet->getEntity(i);
				for(j = 0; j < ent->getNMentions(); j++){
					if(entitySet->getMention(ent->getMention(j))->getSentenceNumber() != sent_num)				
						continue;
					Mention* ment1 = mentionSet->getMention(ent->getMention(j));
					if(!ment1->isOfRecognizedType())
						continue;
		
					for(k = j+1; k < ent->getNMentions(); k++){
						if(entitySet->getMention(ent->getMention(k))->getSentenceNumber() != sent_num)				
							continue;
						Mention* ment2 = mentionSet->getMention(ent->getMention(k));
						if(!ment2->isOfRecognizedType())
							continue;
						if(	entitySet->getMention(ent->getMention(k))->getSentenceNumber() == sent_num){				
							std::cout<<"\t\tadding ident relation: "<<result->getNRelMentions()<<std::endl;
							result->takeRelMention(_new RelMention(ment1, ment2, 
								Symbol(L"IDENT"), sent_num, result->getNRelMentions(), 1.0));
						}
					}
				}
				
			}
		}
	}

	return result;
}

EventMentionSet *CorrectAnswers::correctEventTheory(TokenSequence *tokenSequence,
													ValueMentionSet *valueMentionSet,
													const Parse *parse, 
													MentionSet *mentionSet,
													PropositionSet *propSet,
													Symbol docName)
{
	CorrectDocument *correctDocument = getCorrectDocument(docName);
	EventMentionSet *result = _new EventMentionSet(parse);

	int i, j;
	for (i = 0; i < correctDocument->getNEvents(); i++) {
		CorrectEvent *correctEvent = correctDocument->getEvent(i);
		
		for (j = 0; j < correctEvent->getNMentions(); j++) {
			CorrectEventMention *correctMention = correctEvent->getMention(j);

			if (!isWithinTokenSequence(tokenSequence, correctMention->getAnchorStart(), 
				correctMention->getAnchorEnd())) 
				continue;

			int sTrigger;
			int eTrigger;
			if (!findTokens(tokenSequence, 
							correctMention->getAnchorStart(), correctMention->getAnchorEnd(),
							sTrigger, eTrigger)) 
			{
				SessionLogger::logger->beginWarning();
				*SessionLogger::logger << "\nLost event anchor (";
				*SessionLogger::logger << correctMention->getAnnotationID().to_debug_string() << "): ";
				*SessionLogger::logger << correctMention->getAnchorStart() << " ";
				*SessionLogger::logger << correctMention->getAnchorEnd() << "\n";
				continue;
			}

			correctMention->setMentionArgs(mentionSet, correctDocument, _eventTypesAllowUndet);
			correctMention->setValueArgs(valueMentionSet, correctDocument);
			
			EventMention *systemEvent = _new EventMention(mentionSet->getSentenceNumber(), result->getNEventMentions());
			systemEvent->setEventType(correctMention->getType());
			systemEvent->setAnnotationID(correctMention->getAnnotationID());
			for (int slot = 0; slot < correctMention->getNArgs(); slot++) {
				if (correctMention->getNthMention(slot) != 0) {
					systemEvent->addArgument(correctMention->getNthRole(slot),
						correctMention->getNthMention(slot));
				} else if (correctMention->getNthValueMention(slot) != 0) {
					systemEvent->addValueArgument(correctMention->getNthRole(slot),
						correctMention->getNthValueMention(slot));
				} else {
					SessionLogger::logger->beginWarning();
					*SessionLogger::logger << "\nLost event argument (";
					*SessionLogger::logger << correctMention->getAnnotationID().to_debug_string() << "): ";
					*SessionLogger::logger << correctMention->getNthRole(slot).to_debug_string() << "\n";
				}
			}

			// pick best trigger word -- use function inside EventMention to do so
			const Proposition *bestProp = 0;
			int bestTrigger = sTrigger;
			for (int word = sTrigger; word <= eTrigger; word++) {
				const SynNode *anchor = parse->getRoot()->getNthTerminal(word);
				if (anchor->getParent() != 0) 
					anchor = anchor->getParent();
				systemEvent->setAnchor(anchor, propSet);
				if (systemEvent->getAnchorProp() != 0) {
					if (bestProp == 0 ||
						(bestProp->getPredType() != Proposition::VERB_PRED &&
						 systemEvent->getAnchorProp()->getPredType() == Proposition::VERB_PRED))
					{
						// for now, prefer verbs?
						bestProp = systemEvent->getAnchorProp();
						bestTrigger = word;
					}
				}
			}
			
			// for real now, set the anchor
			const SynNode *anchor = parse->getRoot()->getNthTerminal(bestTrigger);
			// we want the anchor to be the preterminal node of the trigger word
			if (anchor->getParent() != 0) 
				anchor = anchor->getParent();
			systemEvent->setAnchor(anchor, propSet);
			systemEvent->setModality(correctEvent->getModality());
			systemEvent->setGenericity(correctEvent->getGenericity());
			systemEvent->setPolarity(correctEvent->getPolarity());
			systemEvent->setTense(correctEvent->getTense());
			result->takeEventMention(systemEvent);
		}

	}

	return result;
}

CorrectDocument * CorrectAnswers::getCorrectDocument(Symbol docName) 
{
	//std::cout << L"param: " << docName.to_debug_string() << "\n";
	int i;
	for (i = 0; i < _n_documents; i++) {
		//std::cout << L"inside if " << _documents[i].getID().to_debug_string() << "\n";
		if (_documents[i].getID() == docName) {
			return &(_documents[i]);
		}
	}

	//exit(0);
	// if we can't find it, it might have had a space in it (eeeevil Ace2004)
	std::wstring newname = docName.to_string();
	size_t index = newname.find(L" ");
	const wchar_t* underscore = L"_";
	while (index <= newname.length()) {
        newname.replace(index, 1, underscore);
		index = newname.find(L" ", index + 1);
	}
	Symbol newnameSym(newname.c_str());

	for (i = 0; i < _n_documents; i++) {
		if (_documents[i].getID() == newnameSym) {
			return &(_documents[i]);
		}
	}

	char tempBuffer[8192];
	sprintf(tempBuffer, "Couldn't find (correct-answers) document %s!\n", docName.to_debug_string());
	throw InternalInconsistencyException("CorrectAnswers::getCorrectDocument", tempBuffer);
	
	return 0;
}

void CorrectAnswers::adjustParseScores(Parse *results[], size_t n_parses, 
									   TokenSequence *tokenSequence, Symbol docName) 
{
	size_t i;
	for (i = 0; i < n_parses; i++) {
		float n_matched_mentions = getNumberOfMatchedCorrectMentions(results[i], tokenSequence, docName);
		results[i]->setScore(results[i]->getScore() + (float)10000.0 * n_matched_mentions);
	}
}

float CorrectAnswers::getNumberOfMatchedCorrectMentions(Parse *parse, TokenSequence *tokenSequence, Symbol docName) {
	
	float num_matched_mentions = (float)0.0;
	CorrectDocument *correctDocument = getCorrectDocument(docName);

	int i, j;
	for (i = 0; i < correctDocument->getNEntities(); i++) {
		CorrectEntity *correctEntity = correctDocument->getEntity(i);

		for (j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention *correctMention = correctEntity->getMention(j);
			if (correctMention->getSentenceNumber() != tokenSequence->getSentenceNumber())
				continue;
			
			_matched = false;
			_exact_match = false;
			_too_large_match = false;
			checkIfNodeMatches(parse->getRoot(), correctMention);
			if (_matched == true) num_matched_mentions += (float)1.0;
			if (_exact_match && !_too_large_match) num_matched_mentions += (float)0.1;
		}
	}
	return num_matched_mentions;
}

void CorrectAnswers::checkIfNodeMatches(const SynNode *node, CorrectMention *cm) 
{
	if (node->isTerminal())
		return;

	// nominal premods 
	if (cm->isNominalPremod() ||
		(cm->isUndifferentiatedPremod() && cm->getMentionType() == Mention::DESC))
	{
		if (NodeInfo::isNominalPremod(node) &&
			node->getStartToken() == cm->getStartToken() && 
			node->getEndToken() == cm->getEndToken()) 
		{
			_matched = true;
			_exact_match = true;
		}
	} else {
		if (NodeInfo::isReferenceCandidate(node)) {
			if (node->getStartToken() == cm->getStartToken() && node->getEndToken() == cm->getEndToken()) {
				_matched = true;
				_exact_match = true;
			} else if (node->getHeadPreterm()->getEndToken() == cm->getHeadEndToken()) {
				_matched = true;
				if (node->getStartToken() < cm->getStartToken() || node->getEndToken() > cm->getEndToken()) 
					_too_large_match = true;
			}
		}
	}

	for (int i = 0; i < node->getNChildren(); i++) 
		checkIfNodeMatches(node->getChild(i), cm);

	return;
}

void CorrectAnswers::setSentAndTokenNumbersOnCorrectMentions(TokenSequence *tk, Symbol docName)
{
	CorrectDocument *cd = getCorrectDocument(docName);
	int i, j;
	for (i = 0; i < cd->getNEntities(); i++) {
		CorrectEntity *ce = cd->getEntity(i);

		for (j = 0; j < ce->getNMentions(); j++) {
			CorrectMention *cm = ce->getMention(j);
			if (cm->getSentenceNumber() == -1) {
				if (cm->setStartAndEndTokens(tk)) {
					cm->setSentenceNumber(tk->getSentenceNumber());
				}
			}
		}
	}

	for (i = 0; i < cd->getNValues(); i++) {
		CorrectValue *cv = cd->getValue(i);
		if (cv->setStartAndEndTokens(tk))
			cv->setSentenceNumber(tk->getSentenceNumber());
	}
	
}

Symbol CorrectAnswers::getEventID(EventMention *em, Symbol docName) 
{
	CorrectDocument *cd = getCorrectDocument(docName);
	for (int i = 0; i < cd->getNEvents(); i++) {
		CorrectEvent *correctEvent = cd->getEvent(i);

		for (int j = 0; j < correctEvent->getNMentions(); j++) {
			CorrectEventMention *cem = correctEvent->getMention(j);
			if (cem->getAnnotationID() == em->getAnnotationID()) {
				return correctEvent->getAnnotationEventID();
			}
		}
	}
	return Symbol();
}

void CorrectAnswers::resetForNewDocument() {
	_documentEntitySet = NULL;
}

std::vector<Constraint> CorrectAnswers::getConstraints(TokenSequence *tokenSequence, Symbol docName) {
	std::vector<Constraint> constraints; // The return value

	CorrectDocument *correctDocument = getCorrectDocument(docName);

	int i, j;
	for (i = 0; i < correctDocument->getNEntities(); i++) {
		CorrectEntity *correctEntity = correctDocument->getEntity(i);

		for (j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention *correctMention = correctEntity->getMention(j);
			if (correctMention->getMentionType() == Mention::DESC &&
				correctMention->getSentenceNumber() == tokenSequence->getSentenceNumber())
			{                
				if (correctMention->getStartToken() == correctMention->getEndToken() &&
					!correctMention->isNominalPremod()) 
				{
					// e.g. "/skiers/ enjoy winter"
					constraints.push_back(
						Constraint(correctMention->getHeadStartToken(),
									correctMention->getHeadEndToken(),
									LanguageSpecificFunctions::getCoreNPlabel(),
									*correctEntity->getEntityType()));
				} else if (correctMention->getHeadStartToken() == correctMention->getHeadEndToken()) {
					// e.g. "/happy skiers in Colorado/" --> skiers
					// e.g. "the /station/ chief" --> station
					constraints.push_back(
						Constraint(correctMention->getHeadStartToken(),
									correctMention->getHeadEndToken(),
									ParserTags::HEAD_CONSTRAINT,
									EntityType::getOtherType()));
				}
			}

		}
	}
	return constraints;

}

std::vector<CharOffset> CorrectAnswers::getMorphConstraints(const Sentence *sentence, Symbol docName) {
	std::vector<CharOffset> constraints; // The return value

	CorrectDocument *correctDocument = getCorrectDocument(docName);
	const LocatedString *sentString = sentence->getString();

	int i, j;
	for (i = 0; i < correctDocument->getNEntities(); i++) {
		CorrectEntity *correctEntity = correctDocument->getEntity(i);

		for (j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention *correctMention = correctEntity->getMention(j);
			if (correctMention->getStartOffset() >= sentString->firstStartOffsetStartingAt<EDTOffset>(0) &&
				correctMention->getEndOffset() <= sentString->lastEndOffsetEndingAt<EDTOffset>(sentString->length() - 1))
			{
				constraints.push_back(sentString->convertStartOffsetTo<CharOffset>(correctMention->getStartOffset()));
				constraints.push_back(sentString->convertEndOffsetTo<CharOffset>(correctMention->getEndOffset()));
				constraints.push_back(sentString->convertStartOffsetTo<CharOffset>(correctMention->getHeadStartOffset()));
				constraints.push_back(sentString->convertEndOffsetTo<CharOffset>(correctMention->getHeadEndOffset()));
			}

		}
	}
	return constraints;
}

bool CorrectAnswers::isWithinTokenSequence(TokenSequence *tokenSequence, EDTOffset start, EDTOffset end) 
{
	int num_tokens = tokenSequence->getNTokens();
	if (num_tokens < 1) 
		return false;

	return (start >= tokenSequence->getToken(0)->getStartEDTOffset() &&
		   start <= tokenSequence->getToken(num_tokens - 1)->getEndEDTOffset()) ||
		   (end >= tokenSequence->getToken(0)->getStartEDTOffset() &&
		   end <= tokenSequence->getToken(num_tokens - 1)->getEndEDTOffset());
}

bool CorrectAnswers::isWithinTokenSequence(TokenSequence *tokenSequence, CorrectMention *correctMention) 
{
	return isWithinTokenSequence(tokenSequence, correctMention->getHeadStartOffset(), correctMention->getHeadEndOffset());
}

bool CorrectAnswers::findTokens(TokenSequence *tokenSequence, EDTOffset s_offset, EDTOffset e_offset,
							  int& s_token, int& e_token) {

	if (!isWithinTokenSequence(tokenSequence, s_offset, e_offset)) return false;
		
	int num_tokens = tokenSequence->getNTokens();

	bool found_start = false;
	for (int k = 0; k < num_tokens; k++) {
		const Token *token = tokenSequence->getToken(k);		
		if (found_start) {
			if (e_offset < token->getStartEDTOffset()) 
				break;
			else 
				e_token = k;
		}
		if (!found_start) {
			if (s_offset <= token->getEndEDTOffset() && 
				e_offset >= token->getStartEDTOffset()) {
				found_start = true;
				s_token = k;
				e_token = k;
			}
		}
	}

	if (!found_start) {
	//	throw InternalInconsistencyException("CorrectAnswers::findToken",
	//		"mention specifies tokens not in token sequence");
		s_token = -1;
		e_token = -1;
		return false;
	}

	return true;
}

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/DescriptorRecognizer.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/EvalSpecificRules.h"

#include "Generic/descriptors/ClassifierTreeSearch.h"
#include "Generic/descriptors/DescriptorClassifier.h"
#include "Generic/descriptors/NoneClassifier.h"
#include "Generic/descriptors/PartitiveClassifier.h"
#include "Generic/descriptors/AppositiveClassifier.h"
#include "Generic/descriptors/ListClassifier.h"
#include "Generic/descriptors/NestedClassifier.h"
#include "Generic/descriptors/PronounClassifier.h"

#include "Generic/preprocessors/NamedSpan.h"

#include <boost/algorithm/string.hpp>

using namespace std;

DescriptorRecognizer::DescriptorRecognizer()
	: _descriptorClassifier(DescriptorClassifier::build()), _partitiveClassifier(),
	  _appositiveClassifier(), _listClassifier(),
	  _nestedClassifier(), _pronounClassifier(PronounClassifier::build()), _subtypeClassifier(),
	  _searcher(true), _usePIdFDesc(false), _pdescDecoder(0),
	  _debug(Symbol(L"desc_rec_debug")), _use_correct_answers(false)
{
	_doStage = true;
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
}

DescriptorRecognizer::DescriptorRecognizer(bool doStage)
: _noneClassifier(), _descriptorClassifier(DescriptorClassifier::build()), _partitiveClassifier(),
_appositiveClassifier(), _listClassifier(), _nomPremodClassifier(NomPremodClassifier::build()),
	  _nestedClassifier(), _pronounClassifier(PronounClassifier::build()), _subtypeClassifier(),
	  _searcher(true), _usePIdFDesc(false), _pdescDecoder(0),
	  _debug(Symbol(L"desc_rec_debug")), _use_correct_answers(false)
{
	_doStage = doStage;
	_usePIdFDesc = ParamReader::isParamTrue("use_pidf_desc");
	if(_usePIdFDesc){
		
		std::string tag_set_file = ParamReader::getRequiredParam("pdesc_tag_set_file");
		std::string features_file = ParamReader::getRequiredParam("pdesc_features_file");
		std::string model_file = ParamReader::getRequiredParam("pdesc_model_file");
		_pdescDecoder = _new PIdFModel(PIdFModel::DECODE,
			tag_set_file.c_str(), features_file.c_str(), model_file.c_str(), 0);
	}
	_useNPChunkParse = ParamReader::isParamTrue("do_np_chunk");
	_nationalityDescriptorsTreatedAsNames = ParamReader::isParamTrue("nat_desc_are_names");
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
}


DescriptorRecognizer::~DescriptorRecognizer() {
	delete _descriptorClassifier;
	delete _pronounClassifier;
	delete _nomPremodClassifier;
	delete _pdescDecoder;
}

void DescriptorRecognizer::resetForNewSentence() {
}

void DescriptorRecognizer::cleanup() {
	_descriptorClassifier->cleanup();
	_nomPremodClassifier->cleanup();
}

void DescriptorRecognizer::resetForNewDocument(DocTheory *docTheory) 
{
	_docTheory = docTheory;
}

int DescriptorRecognizer::getDescriptorTheories(MentionSet *results[],
											    int max_theories,
											    const PartOfSpeechSequence* partOfSpeechSeq,
											    const Parse *parse,
											    const NameTheory *nameTheory,
											    const NestedNameTheory *nestedNameTheory,
											    TokenSequence* tokenSequence,
											    int sentence_number)
{
	// before any desc class, the mentionSet is unclassified mentions and
	// name mentions
	MentionSet* startingSet = _new MentionSet(parse, sentence_number);
	// don't find mentions in POSTDATE region, since it only contains a timex expression
	if (_docTheory != 0 && _docTheory->isPostdateSentence(sentence_number)) {
		results[0] = startingSet;
		return 1;
	}

	// don't find mention when sentence wasn't parsed
	if (parse->isDefaultParse()) {
		results[0] = startingSet;
		return 1;
	}
	
	putNameTheoryInMentionSet(nameTheory, startingSet);
	putNestedNameTheoryInMentionSet(nestedNameTheory, startingSet);

	if(_usePIdFDesc){

		NameTheory* descAsNames[1];
		_pdescDecoder->getNameTheories(descAsNames,1, tokenSequence);
		adjustDescTheoryBoundaries(descAsNames[0], startingSet);
		removeNamesFromDescTheory(nameTheory, descAsNames[0]);
		putDescTheoryInMentionSet(descAsNames[0], startingSet);
		delete descAsNames[0];

	}
	//short cut to using POS theory in _subtypeClassifier, if it is helpful,
	//POSTheory should become an argument of all classifiers
	if(partOfSpeechSeq == 0){
		SessionLogger::info("SERIF")<<"pos is 0 in DescRec"<<std::endl;
	}
	_subtypeClassifier.setPOSSequence(partOfSpeechSeq);
	processTheory(startingSet, parse, max_theories);
	_subtypeClassifier.setPOSSequence(0);


	// now, the results are in the searcher, so extract them
	int nresults = _searcher.transferLeaves(results);
	//std::cout<<"Mention Set before second findPartitives"<<std::endl;
	//results[0]->dump(std::cout);
	//std::cout<<std::endl;

	if(_useNPChunkParse){
		for(int i=0; i<nresults; i++){
			//std::cout<<"Call findParitivesInNPChunkParse on: "<<std::endl;
			//results[i]->dump(std::cout);
			//std::cout<<std::endl;
			//parse->dump(std::cout);
			//std::cout<<std::endl;
			findPartitvesInNPChunkParse(results[i]);
		}
	}
	//std::cout<<"Mention Set before after findPartitives"<<std::endl;
	//results[0]->dump(std::cout);
	//std::cout<<std::endl;

	for (int i = 0; i < nresults; i++) {
		reprocessNominalPremods(results[i], parse->getRoot());
		// some nominal premods didn't have entity types when subtypes were last generated
		regenerateSubtypes(results[i], parse->getRoot()); 
	}

	//Forces an entity type on the first mention in the first sentence (i.e. first in the document, hopefully).
	//This is an ad hoc way of dealing with document-initial subject labels, for example in an SEC document
	//the company name is typically the first mention and should always be tagged as an ORG.
	if (ParamReader::getParam("entity_type_for_first_mention") != "" && sentence_number == 0 && results[0]->getNMentions() == 1) {
		std::string first_entity_type = ParamReader::getParam("entity_type_for_first_mention");
		std::wstring type_for_mention_0 = std::wstring(first_entity_type.begin(), first_entity_type.end());
		Symbol initial_etype = Symbol(type_for_mention_0);
		if (EntityType::isValidEntityType(initial_etype) && results[0]->getMention(0)->getEntityType().getName() != initial_etype)
			results[0]->getMention(0)->setEntityType(EntityType(initial_etype));
	}

	return nresults;
}


void DescriptorRecognizer::processTheory(MentionSet *mentionSet,
										 const Parse *parse,
										 int branch)
{
	// first time into the sentence, so initialize the searcher
	_searcher.resetSearch(mentionSet, branch);

	processNode(parse->getRoot());
	if (EntitySubtype::subtypesDefined())
		generateSubtypes(parse->getRoot());
}

void DescriptorRecognizer::generateSubtypes(const SynNode *node)
{
	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		generateSubtypes(node->getChild(i));

	// process this node only if it has a mention
	if (!node->hasMention())
		return;

	_searcher.performSearch(node, _subtypeClassifier);

}

void DescriptorRecognizer::regenerateSubtypes(MentionSet *mset, const SynNode *node)
{
	if (!EntitySubtype::subtypesDefined())
		return;

	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		regenerateSubtypes(mset, node->getChild(i));

	// process this node only if it has a mention
	if (!node->hasMention())
		return;

	MentionSet *results[1];

	Mention *currMention = mset->getMentionByNode(node);
	if ((!currMention->getEntitySubtype().isDetermined()) || _use_correct_answers )
		_subtypeClassifier.classifyMention(mset, currMention, results, 1, false);

}


void DescriptorRecognizer::reprocessNominalPremods(MentionSet *mentionSet, const SynNode *node)
{
	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		reprocessNominalPremods(mentionSet, node->getChild(i));

	if (!_doStage || !node->hasMention())
		return;

	MentionSet *results[1];
	_nomPremodClassifier->classifyMention(mentionSet, mentionSet->getMentionByNode(node),
		results, 1, false);

}

void DescriptorRecognizer::processNode(const SynNode *node)
{
	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		processNode(node->getChild(i));

	// process this node only if it has a mention
	if (!node->hasMention())
		return;


	
	if (_debug.isActive())
		_debug << "Recognizing:\n" << node->toDebugString(0).c_str() << "\n";


	// Test 1: See if it's a partitive
	_searcher.performSearch(node, _partitiveClassifier);

	// Test 2: See if it's an appositive
	// won't do anything if it's already partitive
	// no appositive classification unless desc classification
	if (_doStage)
		_searcher.performSearch(node, _appositiveClassifier);

	// Test 3: See if it's a list
	// won't do anything if it's part or appos
	_searcher.performSearch(node, _listClassifier);

	// Test 4: See if it's a nested mention
	// won't do anything if it's part, appos, or list
	// note that being nested doesn't preclude later classification

	_searcher.performSearch(node, _nestedClassifier);

	// classify as none here unless desc classification
	if (!_doStage) {
		_searcher.performSearch(node, _noneClassifier);
		return;
	}


	// test 5: See if it's a pronoun (and mark it but don't classify till later)
	// won't do anything if it's part, appos, list, or nested

	_searcher.performSearch(node, *_pronounClassifier);


	// finally...
	// test 6: classify the entity type.
	// won't do anything if it's partitive, appositive, list, pronoun, or name
	// might classify something nested
	_searcher.performSearch(node, *_nomPremodClassifier);

	_searcher.performSearch(node, *_descriptorClassifier);

	return;

}


void DescriptorRecognizer::putNameTheoryInMentionSet(
	const NameTheory *nameTheory,
	MentionSet *mentionSet)
{
	int n_unassigned_names = 0;

	bool fix_ace_inconsistencies = ParamReader::getOptionalTrueFalseParamWithDefaultVal("fix_ace_inconsistencies", false);
	for (int i = 0; i < nameTheory->getNNameSpans(); i++) {
		int start = nameTheory->getNameSpan(i)->start;
		int end = nameTheory->getNameSpan(i)->end;
		EntityType etype = nameTheory->getNameSpan(i)->type;
		bool assigned = false;

		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *mention = mentionSet->getMention(j);
			if (mention->node->getStartToken() == start &&
				mention->node->getEndToken() == end &&
				mention->node->getTag() == LanguageSpecificFunctions::getNameLabel())
			{
				if (mention->isPopulated()) {
					throw InternalInconsistencyException(
						"DescriptorRecognizer::putNameTheoryInMentionSet()",
						"Name mention populated before name added!");
				}

				if (NationalityRecognizer::isNamePersonDescriptor(mention->node) && _doStage)
				{
					/*
					if(NationalityRecognizer::useRuleNationalities()){
						EntityType ent_type =  
							NationalityRecognizer::getNationalityEntType(mention->node);
						Mention::Type ment_type = 
							NationalityRecognizer::getNationalityMentType(mention->node);
						

					}*/
					if(_nationalityDescriptorsTreatedAsNames){
						mention->mentionType = Mention::NAME;
						mention->setEntityType( EntityType::getPERType() );
					} else {
						enterPersonDescriptor(mention, mentionSet);
					}
				} else if (fix_ace_inconsistencies) {
					EvalSpecificRules::NamesToNominals(mentionSet->getParse()->getRoot(), mention, etype);
				} else {
					mention->mentionType = Mention::NAME;
					mention->setEntityType(etype);

					// Check if there's an explicit name span with a subtype in the document metadata
					TokenSequence* tokens = _docTheory->getSentenceTheory(mention->getSentenceNumber())->getTokenSequence();
					const DataPreprocessor::NamedSpan* startSpan = dynamic_cast<DataPreprocessor::NamedSpan*>(_docTheory->getMetadata()->getStartingSpan(tokens->getToken(mention->getNode()->getStartToken())->getStartEDTOffset(), Symbol(L"Name")));
					const DataPreprocessor::NamedSpan* endSpan = dynamic_cast<DataPreprocessor::NamedSpan*>(_docTheory->getMetadata()->getEndingSpan(tokens->getToken(mention->getNode()->getEndToken())->getEndEDTOffset(), Symbol(L"Name")));
					if (startSpan != NULL && endSpan != NULL && startSpan == endSpan) {
						Symbol mappedType = startSpan->getMappedType();
						Symbol mappedSubtype = startSpan->getMappedSubtype();
						if (!mappedType.is_null() && !mappedSubtype.is_null())
							mention->setEntitySubtype(EntitySubtype(mappedType, mappedSubtype));
					}
				}
				assigned = true;
				break;
			}
		}

		if (!assigned){
			n_unassigned_names++;
			ostringstream ostr;
			ostr << "Name was ignored when MentionSet created: ";
			const SynNode* root = mentionSet->getParse()->getRoot();
			Symbol terminals[500];
			int nterm = root->getTerminalSymbols(terminals, 500);
			for(int nindex = start; nindex<=end; nindex++){
				if(nindex < nterm){
					ostr <<terminals[nindex].to_debug_string() << " " ;
				}
			}
			ostr <<"\n";
			SessionLogger::warn("mention_set_creation") << ostr.str();
		}

	}
	if (n_unassigned_names > 0) {
		SessionLogger::warn("mention_set_creation") << n_unassigned_names
			   << " name mention spans had no corresponding Mention.\n";

	}

	mentionSet->setNameScore(nameTheory->getScore());
}

void DescriptorRecognizer::putNestedNameTheoryInMentionSet(
	const NestedNameTheory *nestedNameTheory,
	MentionSet *mentionSet)
{
	int n_unassigned_names = 0;

	for (int i = 0; i < nestedNameTheory->getNNameSpans(); i++) {
		int start = nestedNameTheory->getNameSpan(i)->start;
		int end = nestedNameTheory->getNameSpan(i)->end;
		EntityType etype = nestedNameTheory->getNameSpan(i)->type;
		bool assigned = false;

		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *mention = mentionSet->getMention(j);
			if (mention->node->getStartToken() == start &&
				mention->node->getEndToken() == end &&
				mention->node->getTag() == LanguageSpecificFunctions::getNameLabel())
			{
				if (mention->isPopulated()) {
					throw InternalInconsistencyException(
						"DescriptorRecognizer::putNestedNameTheoryInMentionSet()",
						"Name mention populated before name added!");
				}
				
				mention->mentionType = Mention::NEST;
				mention->setEntityType(etype);

				assigned = true;
				break;
			}
		}

		if (!assigned){
			n_unassigned_names++;
			ostringstream ostr;
			ostr << "Nested name was ignored when MentionSet created: ";
			const SynNode* root = mentionSet->getParse()->getRoot();
			Symbol terminals[500];
			int nterm = root->getTerminalSymbols(terminals, 500);
			for(int nindex = start; nindex<=end; nindex++){
				if(nindex < nterm){
					ostr <<terminals[nindex].to_debug_string() << " " ;
				}
			}
			ostr <<"\n";
			SessionLogger::warn("mention_set_creation") << ostr.str();
		}

	}
	if (n_unassigned_names > 0) {
		SessionLogger::warn("mention_set_creation") << n_unassigned_names
			   << " nested name mention spans had no corresponding Mention.\n";

	}
}


void DescriptorRecognizer::enterPersonDescriptor(Mention *mention,
												 MentionSet *mentionSet)
{
	mention->mentionType = Mention::DESC;
	mention->setEntityType(EntityType::getPERType());

	// if this is the head of a larger NP, then make that NP a per desc too
	if (mention->node->getParent() != 0 &&
		mention->node->getParent()->hasMention() &&
		NationalityRecognizer::isNamePersonDescriptor(
			mention->node->getParent()))
	{
		const SynNode *parent = mention->node->getParent();
		enterPersonDescriptor(mentionSet->getMentionByNode(parent),
							  mentionSet);
	}
}
/*	if (mention->node->getParent() != 0 &&
		mention->node->getParent()->hasMention())
	{
		const SynNode *parent = mention->node->getParent();

		bool other_reference_found = false;
		for (int i = 0; i < parent->getNChildren(); i++) {
			if (parent->getChild(i)->hasMention() ||
				NodeInfo::canBeNPHeadPreterm(parent->getChild(i)))
			{
				if (parent->getChild(i) != mention->node) {
					other_reference_found = true;
					break;
				}
			}
		}
		if (!other_reference_found) {
			enterPersonDescriptor(mentionSet->getMentionByNode(parent),
								  mentionSet);
		}
	}
}
*/
void DescriptorRecognizer::adjustDescTheoryBoundaries(
	NameTheory *nameTheory,	MentionSet *mentionSet)
{
	//adjust the name/desc theory in an attempt to remove clitics from names
	Symbol pos[MAX_SENTENCE_TOKENS];
	int npos = mentionSet->getParse()->getRoot()->getPOSSymbols(pos, MAX_SENTENCE_TOKENS);
	for(int i=0; i< nameTheory->getNNameSpans(); i++){
		int start = nameTheory->getNameSpan(i)->start;
		int end = nameTheory->getNameSpan(i)->end;
		if(start == end){
			continue;
		}

		//while((start < end ) && !((pos[end]  == Symbol(L"NOUN")) ||
		//		(pos[end]  == Symbol(L"NOUN_PROP")) ||
		//		(pos[end]  == Symbol(L"ADJ"))))
		//{
		//		end--;

		//}
		//while((start < end ) && !((pos[start]  == Symbol(L"NOUN")) ||
		//		(pos[start]  == Symbol(L"NOUN_PROP")) ||
		//		(pos[start]  == Symbol(L"ADJ"))))
		//{
		//		start++;
		//}
		//if(start == end){
		//	if((pos[start]  == Symbol(L"NOUN")) ||
		//		(pos[start]  == Symbol(L"NOUN_PROP")) ||
		//		(pos[start]  == Symbol(L"ADJ"))){
		//			SessionLogger::warn("desc_theory_boundaries")
		//			    <<"Adjusting descriptor boundary: start-"
		//				<<nameTheory->getNameSpan(i)->start
		//				<<" -> " <<start<<" end- "
		//				<<nameTheory->getNameSpan(i)->end
		//				<<" -> " <<end<<"\n";

		//			nameTheory->getNameSpan(i)->start = start;
		//			nameTheory->getNameSpan(i)->end = end;


		//	}
		//}

//use new POS Tags
		while((start < end ) && 
			!(LanguageSpecificFunctions::isNoun(pos[end]) ||
			LanguageSpecificFunctions::isAdjective(pos[end])))
		{
				end--;

		}
		while((start < end ) && 
			!(LanguageSpecificFunctions::isNoun(pos[start]) ||
			LanguageSpecificFunctions::isAdjective(pos[start])))
		{
				start++;

		}
		if(start == end){
			if(LanguageSpecificFunctions::isNoun(pos[start]) ||
				LanguageSpecificFunctions::isAdjective(pos[start]))
			{
				SessionLogger::warn("desc_theory_boundaries") <<"Adjusting descriptor boundary: start-"
						<<nameTheory->getNameSpan(i)->start
						<<" -> " <<start<<" end- "
						<<nameTheory->getNameSpan(i)->end
						<<" -> " <<end<<"\n";

				nameTheory->getNameSpan(i)->start = start;
				nameTheory->getNameSpan(i)->end = end;
			}
		}
	}
}

			
void DescriptorRecognizer::putDescTheoryInMentionSet(
	NameTheory *nameTheory,
	MentionSet *mentionSet)

{
	int n_unassigned_names = 0;

	for (int i = 0; i < nameTheory->getNNameSpans(); i++) {
		int start = nameTheory->getNameSpan(i)->start;
		int end = nameTheory->getNameSpan(i)->end;
		EntityType etype = nameTheory->getNameSpan(i)->type;
		bool assigned = false;

		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *mention = mentionSet->getMention(j);
			if (mention->node->getStartToken() == start &&
				mention->node->getEndToken() == end )
				//mention->node->getTag() == LanguageSpecificFunctions::getNameLabel())
			{
				if (mention->isPopulated()) {
					ostringstream ostr;
					ostr<<"Populated Mention: "<<std::endl;
					mention->dump(ostr);
					ostr<<std::endl;
					ostr<<start<<" "<<end<<std::endl;
					SessionLogger::info("SERIF") << ostr.str();
					throw InternalInconsistencyException(
						"DescriptorRecognizer::putDescTheoryInMentionSet()",
						"Desc mention populated before name added!");
				}
				mention->mentionType = Mention::DESC;
				mention->setEntityType(etype);
				assigned = true;
				break;
			}
			else if(mention->node->getHead()->getStartToken() ==start &&
				mention->node->getHead()->getEndToken() == end )
			{
				if (mention->isPopulated()) {
					continue;
				}
				mention->mentionType = Mention::DESC;
				mention->setEntityType(etype);
				assigned = true;
				break;
			}
		}

		if (!assigned){
			n_unassigned_names++;
			ostringstream ostr;
			ostr <<"Descriptor was ignored when MentionSet created: ";
			const SynNode* root = mentionSet->getParse()->getRoot();
			Symbol terminals[500];
			Symbol pos[500];
			int nterm = root->getTerminalSymbols(terminals, 500);
			int npos = root->getPOSSymbols(pos, 500);
			int maxindex = nterm >= npos ? nterm : npos;
			for(int nindex = start; nindex<=end; nindex++){
				if(nindex < maxindex){
					ostr <<"( "
						<<pos[nindex].to_debug_string()<< " "
						<<terminals[nindex].to_debug_string() << " ) " ;
				}
			}
			ostr <<"\n";
			SessionLogger::warn("mention_set_creation") << ostr.str();

		}
	}
	if (n_unassigned_names > 0) {
		SessionLogger::warn("mention_set_creation") << n_unassigned_names
			<< " desc mention spans had no corresponding Mention.\n"
			<< "Perhaps the MentionSet was truncated?\n";
	}


	mentionSet->setDescScore(nameTheory->getScore());
}


void DescriptorRecognizer::removeNamesFromDescTheory(const NameTheory *nameTheory,
													 NameTheory*& descTheory)
{
	NameTheory* origDescTheory = descTheory;
	//get a count of how many names you keep
	int nkept = 0;
	for(int i=0;i < origDescTheory->getNNameSpans(); i++){
		int dstart  = origDescTheory->getNameSpan(i)->start;
		int dend  = origDescTheory->getNameSpan(i)->end;
		bool keep= true;
		for(int j = 0; j< nameTheory->getNNameSpans(); j++){
			int nstart  = nameTheory->getNameSpan(j)->start;
			int nend  = nameTheory->getNameSpan(j)->end;
			if((nstart == dstart) && (dend == nend)){
				keep =false;
				break;
			}
		}
		if(keep){
			nkept++;
		}

	}

	std::vector<NameSpan*> nameSpansToKeep;
	for(int k=0;k < origDescTheory->getNNameSpans(); k++){
		int dstart  = origDescTheory->getNameSpan(k)->start;
		int dend  = origDescTheory->getNameSpan(k)->end;
		bool keep= true;
		for(int j = 0; j< nameTheory->getNNameSpans(); j++){
			int nstart  = nameTheory->getNameSpan(j)->start;
			int nend  = nameTheory->getNameSpan(j)->end;
			if((nstart == dstart) && (dend == nend)){
				keep =false;
				break;
			}
		}
		if(keep){
			nameSpansToKeep.push_back(_new NameSpan(*(origDescTheory->getNameSpan(k))));
		}
		else{
			SessionLogger::warn("mention_set_creation") <<L" Deleting Descriptor that overlaps with name"<<
				dstart<<L" "<<dend<<L" "<<
				origDescTheory->getNameSpan(k)->type.getName().to_string()
				<<"\n";
		}
	}

	const TokenSequence* tokSequence = nameTheory->getTokenSequence();
	NameTheory* newDescTheory = _new NameTheory(tokSequence, nameSpansToKeep);

	delete origDescTheory;

	descTheory = newDescTheory;
}

//since the Partitive classifier doesn't work on NP chunk parses.....
//all that needs to be done is setting the partitive entity type to the
//type of the whole
void DescriptorRecognizer::findPartitvesInNPChunkParse(MentionSet* mentionSet){
	int nment = mentionSet->getNMentions();
	for(int i =0; i< nment; i++){
	Mention* partment = mentionSet->getMention(i);
	if((partment->getMentionType() == Mention::PART)){
		if(partment->getEntityType() == EntityType::getOtherType()){
			//std::cout<<"Partitive with undetermined type: "<<std::endl;
			//partment->dump(std::cout);
			//std::cout<<std::endl;
			Mention* child = partment->getChild();
			if(child->getEntityType() != EntityType::getOtherType()){
				//std::cout<<"Set to child type: "<<std::endl;
				//child->dump(std::cout);
				//std::cout<<std::endl;
				partment->setEntityType(child->getEntityType());
			}
			while((child != 0) && (child->getEntityType() == EntityType::getOtherType())){
				child = child->getChild();
			}
			if(child != 0){
				//std::cout<<"Set to decendant type: "<<std::endl;
				//child->dump(std::cout);
				//std::cout<<std::endl;
				partment->setEntityType(child->getEntityType());
			}


		}
	}


}
//
//void DescriptorRecognizer::findPartitvesInNPChunkParse(MentionSet* mentionSet){
//	int nment = mentionSet->getNMentions();
//	std::cout<<"Calling findPartitivesInNPChunkParse() nmentions: "<<nment<<std::endl;
//	CompoundMentionFinder* cmpf = CompoundMentionFinder::getInstance();
//
//	for(int i =0; i< nment; i++){
//		Mention* partment = mentionSet->getMention(i);
//		if((partment->getMentionType() == Mention::PART) ||
//			(partment->getMentionType() == Mention::NAME) ){
//				std::cout<<"\tskip known mention "<<std::endl;
//				partment->dump(std::cout);
//				std::cout<<std::endl;
//				continue;
//			}
//	//
//			std::cout<<"\tcall findPartitiveWholeMention() to Look for Partitive in: "<<std::endl;
//			partment->dump(std::cout);
//			std::cout<<std::endl;
//			partment->getNode()->dump(std::cout);
//			std::cout<<std::endl;
//	//
//			Mention* wholement = cmpf->findPartitiveWholeMention(mentionSet, partment);
//			std::cout<<"\tReturn From findPartitiveWholeMention()"<<std::endl;
//		if(wholement !=0){
//	//
//			std::cout<<"Found Partitive"<<std::endl;
//			std::cout<<"Childmention: "<<std::endl;
//			wholement->dump(std::cout);
//			std::cout<<std::endl;
//			wholement->getNode()->dump(std::cout);
//			std::cout<<std::endl;
//	//
//			//can a mention be a child even if it isn't a syntactic child?
//
//			Mention* temppart = partment;
//			if(temppart->getChild() != 0){
//				std::cout<<"\twarning making partitive child of sub mention......"<<std::endl;
//				temppart = temppart->getChild();
//				while(temppart->getChild() !=0 ){
//					temppart = temppart->getChild();
//				}
//			}
//
//			Mention* tempwhole = wholement;
//			if(tempwhole->getParent() !=0){
//				std::cout<<"\twarning making partitive whole mention from parent mention...."<<std::endl;
//
//				while(tempwhole->getParent() != 0){
//					tempwhole = tempwhole->getParent();
//				}
//			}
//			if(temppart->getEntityType().isDetermined()){
//				std::cout<<"\twarning changing entity type for partitive: from "
//					<<temppart->getEntityType().getName().to_debug_string()<<" "
//					<<tempwhole->getEntityType().getName().to_debug_string()<<std::endl;
//			}
//			temppart->mentionType = Mention::PART;
//			temppart->setEntityType(tempwhole->getEntityType());
//			tempwhole->makeOnlyChildOf(temppart);
//
//
//		}
//	}
//
}






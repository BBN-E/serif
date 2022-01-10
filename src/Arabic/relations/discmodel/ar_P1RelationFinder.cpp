// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/relations/discmodel/ar_P1RelationFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/common/ParamReader.h"

#include "Generic/discTagger/P1Decoder.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Arabic/relations/discmodel/ar_P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Arabic/common/ar_WordConstants.h"

#include "Arabic/parse/ar_STags.h"
#include "Generic/theories/SynNode.h"

#include "Generic/relations/VectorModel.h"
#include "Arabic/relations/ar_PotentialRelationInstance.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/SymbolUtilities.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Generic/maxent/MaxEntModel.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

#ifdef _WIN32
#define snprintf _snprintf
#endif

UTF8OutputStream ArabicP1RelationFinder::_debugStream;
bool ArabicP1RelationFinder::DEBUG = false;


ArabicP1RelationFinder::ArabicP1RelationFinder() {

	ArabicP1RelationFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	std::string debug_buffer = ParamReader::getParam("p1_debug");
	if (!debug_buffer.empty()) {
		_debugStream.open(debug_buffer.c_str());
		DEBUG = true;
	}

	std::string tag_set_file = ParamReader::getRequiredParam("relation_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	
	std::string features_file = ParamReader::getRequiredParam("relation_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1RelationFeatureType::modeltype);

	_overgen_percentage = ParamReader::getRequiredFloatParam("p1_relation_overgen_percentage");

	std::string model_file = ParamReader::getRequiredParam("relation_model_file");
	
	/*These changes should really mean creating a "combo relation finder", do this afte the eval! */
	
	std::string maxent_model_file = ParamReader::getParam("maxent_relation_model_file");
	if (!maxent_model_file.empty()) {
		_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_maxentWeights, maxent_model_file.c_str(), P1RelationFeatureType::modeltype);
		_maxentDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);
	} else {
		_maxentDecoder = 0;
		_maxentWeights = 0;
	}
	_p1Weights = _new DTFeature::FeatureWeightMap(50000);
	DTFeature::readWeights(*_p1Weights, model_file.c_str(), P1RelationFeatureType::modeltype);

	_decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, _overgen_percentage);

	//mrf - The relation validation string is used to determine which 
	//relation-type/argument types the decoder allows.  RelationObservation calls 
	//the language specific RelationUtilities::get()->isValidRelationEntityTypeCombo().  
	std::string validation_str = ParamReader::getRequiredParam("p1_relation_validation_str");
	_observation = _new RelationObservation(validation_str.c_str());

	_inst = _new PotentialRelationInstance();
	//parameter to set pronoun linking behavior
	_doGenderMatch = ParamReader::isParamTrue("relation_do_gender_match");

	//parameter to set pronoun behavior
	_allowPronTypeChange = Symbol(L"none");
	Symbol temp = ParamReader::getParam(Symbol(L"allow_pron_type_change"));
	if(temp == Symbol(L"all")){
		_allowPronTypeChange = temp;
	}
	else if(temp == Symbol(L"pron-ment")){
		_allowPronTypeChange = temp;
	}
	_femSing = _new SymbolHash(1000);
	_mascSing = _new SymbolHash(1000);

	if((_allowPronTypeChange == Symbol(L"pron-ment")) || (_allowPronTypeChange == Symbol(L"pron-ment"))){

		std::string fn_buffer = ParamReader::getParam("name_gender");
		if (fn_buffer != "") {
			std::string masc_sing_file = fn_buffer + "/mascsing.txt";
			loadSymbolHash(_mascSing, masc_sing_file.c_str());
			std::string fem_sing_file = fn_buffer + "/femsing.txt";
			loadSymbolHash(_femSing, fem_sing_file.c_str());
		}
	}


	//parameter to set relation reverse behavior
	_reverseRelations = Symbol(L"none");
	temp = ParamReader::getParam(Symbol(L"reverse_relations"));
	if(temp == Symbol(L"both")){
		_reverseRelations = temp;
	}
	else if(temp == Symbol(L"rule")){
		_reverseRelations = temp;
	}
	else if(temp == Symbol(L"model")){
		_reverseRelations = temp;
	}

	// Only require vector model if it's going to be used
	_vectorModel = 0;
	if (_reverseRelations == Symbol(L"both") || _reverseRelations == Symbol(L"model")) {
		//vector model used for predicting argument order
		RelationTypeSet::initialize();	
		std::string vector_model_file = ParamReader::getRequiredParam("relation_vector_model_file");
		_vectorModel = _new VectorModel(vector_model_file.c_str());
	}

	std::string exec_file = ParamReader::getParam("exec_head_file");
	if (!exec_file.empty()) {
		_execTable = _new SymbolHash(exec_file.c_str());
	} else _execTable = 0;

	std::string staff_file = ParamReader::getParam("staff_head_file");
	if (!staff_file.empty()) {
		_staffTable = _new SymbolHash(staff_file.c_str());
	} else _staffTable = 0;

	// Better to fail now than fail later
	RelationUtilities::get()->getRelationCutoff();
	RelationUtilities::get()->getAllowableRelationDistance();

}

ArabicP1RelationFinder::~ArabicP1RelationFinder() {
	delete _observation;
	delete _decoder;
}

void ArabicP1RelationFinder::cleanup() {
}


void ArabicP1RelationFinder::resetForNewSentence() {
	_n_relations = 0;
}

RelMentionSet *ArabicP1RelationFinder::getRelationTheory(EntitySet *entitySet,
											   const Parse *parse,
											   MentionSet *mentionSet,
											const ValueMentionSet *valueMentionSet,
											   PropositionSet *propSet,
											   const Parse* secondaryParse)
{
	_currentSentenceIndex = mentionSet->getSentenceNumber();
	/*
	std::cout<<"\n*****************Relation Finder: "<<_currentSentenceIndex<< "**************\n";
	std::cout<<"\tStarting Mention and Entity Sets\n";
	mentionSet->dump(std::cout, 8);
	std::cout<<std::endl;
	entitySet->dump(std::cout, 8);
	std::cout<<std::endl;

	*/

	_observation->resetForNewSentence(parse, mentionSet, valueMentionSet, propSet, secondaryParse );
	if (_decoder->DEBUG) {
		_decoder->_debugStream << L"Sentence Number: " << _currentSentenceIndex << "\n\n";
		_decoder->_debugStream.flush();
	}
	int nmentions = mentionSet->getNMentions();
	//for max_ent decoding,
	double* max_ent_scores = _new double[_tagSet->getNTags()];
	//once a relation is found, a pronoun can't change type.
	EntityType* knownTypes = _new EntityType[nmentions];
	for(int n=0; n<nmentions; n++){
		knownTypes[n] = EntityType::getUndetType();
		if(mentionSet->getMention(n)->getMentionType() == Mention::PRON)
		{
			if(((WordConstants::is2pPronoun(mentionSet->getMention(n)->getNode()->getHeadWord()))||
			((WordConstants::is1pPronoun(mentionSet->getMention(n)->getNode()->getHeadWord())))))
			{
				knownTypes[n] = EntityType::getPERType();
			}
		}
		else{
			knownTypes[n] = mentionSet->getMention(n)->getEntityType();
		}

	}

	for (int i = 0; i < nmentions; i++) {


		if (
			mentionSet->getMention(i)->getMentionType() == Mention::NONE ||
			mentionSet->getMention(i)->getMentionType() == Mention::APPO ||
			mentionSet->getMention(i)->getMentionType() == Mention::LIST)
			continue;
		else if(!mentionSet->getMention(i)->isOfRecognizedType() &&
			((_allowPronTypeChange == Symbol(L"none") ||
			mentionSet->getMention(i)->getMentionType() != Mention::PRON)))
			continue;

		for (int j = i + 1; j < nmentions; j++) {
			//allow pronouns that don't have a type

			// EMB 6/22/05: This is commented out because Marjorie says she doesn't think
			//   we need it, so I am disinclined to try to figure out how to make it work
			//   in the new coreference infrastructure.

/*			if(	_allowPronTypeChange != Symbol(L"none") &&
				((mentionSet->getMention(j)->getMentionType() == Mention::PRON)||
				(mentionSet->getMention(i)->getMentionType() == Mention::PRON)))
			{
				EntityType newEntityType1;
				EntityType newEntityType2;
				Mention* ment1 = mentionSet->getMention(i);
				Mention* ment2 = mentionSet->getMention(j);



				Symbol relationtype = findPronounRelation(ment1, ment2, knownTypes[i], knownTypes[j],
					newEntityType1, newEntityType2);

				if (relationtype != _tagSet->getNoneTag()) {

					//std::cout<<"\tAdding Relation : "<<relationtype.to_debug_string();
					//ment1->dump(std::cout);
					//std::cout<<" ->"<<newEntityType1.getName().to_debug_string()<<" \t ";
					//ment2->dump(std::cout);
					//std::cout<<" ->"<<newEntityType2.getName().to_debug_string()<<std::endl;

					//deal with new pronoun type/links
					//this breaks multi beam serif
					Entity* ent1 = entitySet->getEntityByMention(ment1->getUID(), ment1->getEntityType());
					Entity* ent2 = entitySet->getEntityByMention(ment2->getUID(), ment2->getEntityType());
					if(newEntityType1 != ment1->getEntityType()){
						ment1->setEntityType(newEntityType1;
						linkPronounToEntity(ment1, ent1, ent2, entitySet, mentionSet);
						knownTypes[i] = newEntityType1;
					}
					if(newEntityType2 != ment2->getEntityType()){
						ment2->setEntityType(newEntityType2;
						linkPronounToEntity(ment2, ent2, ent1, entitySet, mentionSet);
						knownTypes[j] = newEntityType2;
					}

					bool reverse = _reverseRelation(relationtype, _observation);
					if (reverse){
						//std::cout<<"Reversing a relation: "<<relationtype.to_debug_string()
						//	<<"Mention 1: ";
						//_observation->getMention1()->dump(std::cout);
						//std::cout<<" Mention 2: ";
						//_observation->getMention2()->dump(std::cout);
						if (_decoder->DEBUG) {
							_decoder->_debugStream<<"Reversing a relation: "<<relationtype.to_debug_string()
							<<"Mention 1: "<<
							_observation->getMention1()->getEntityType().getName().to_debug_string();
							_decoder->_debugStream<<" Mention 2: "<<
							_observation->getMention2()->getEntityType().getName().to_debug_string()<<"\n";
						}

						addRelation(_observation->getMention2(), _observation->getMention1(), relationtype);
					}
					else addRelation(_observation->getMention1(), _observation->getMention2(), relationtype);

				}



				if (_decoder->DEBUG) {
					_decoder->_debugStream << L"\n";
					_decoder->_debugStream.flush();
				}
				continue;
			}*/

			if (!mentionSet->getMention(j)->isOfRecognizedType() ||
				mentionSet->getMention(j)->getMentionType() == Mention::NONE ||
				mentionSet->getMention(j)->getMentionType() == Mention::APPO ||
				mentionSet->getMention(j)->getMentionType() == Mention::LIST)
				continue;
			if (!RelationUtilities::get()->validRelationArgs(mentionSet->getMention(i), mentionSet->getMention(j)))
				continue;
			/*
			//EVIL EVIL hack b/c the LDC marked "prime minister" as a relatin and Valorem did not!!!!
			//this seems to be marked inconsistently in valorem's annotation, removing these lowers scores 
			//slightly overall....
			Symbol head = ArabicSymbol(L"r}ys");
			Symbol minister = ArabicSymbol(L"wzrA'");
			Symbol the_minister = ArabicSymbol(L"AlwzrA'");
			Symbol h1 = mentionSet->getMention(i)->getHead()->getHeadWord();
			Symbol h2 = mentionSet->getMention(j)->getHead()->getHeadWord();
			if(((h1 == head) && ((h2 == minister) || (h2 == the_minister))) ||
				((h2 == head) && ((h1 == minister) || (h1 == the_minister))))
			{
					continue;
			}
			*/


			//partitives are pronouns in the training data, temporarily change the mention type
			bool mentIIsPartitive = false;
			bool mentJIsPartitive = false;
			if(mentionSet->getMention(i)->getMentionType() == Mention::PART){
				mentIIsPartitive = true;
				mentionSet->getMention(i)->mentionType = Mention::PRON;
			}
			if(mentionSet->getMention(j)->getMentionType() == Mention::PART){
				mentJIsPartitive = true;
				mentionSet->getMention(j)->mentionType = Mention::PRON;
			}

			_observation->populate(i, j);

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L" * " << mentionSet->getMention(i)->getUID() << " " << mentionSet->getMention(i)->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << mentionSet->getMention(i)->getNode()->toPrettyParse(3) << L"\n";
				_decoder->_debugStream << L" * " << mentionSet->getMention(j)->getUID() << " " << mentionSet->getMention(j)->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << mentionSet->getMention(j)->getNode()->toPrettyParse(3) << L"\n";
				_decoder->_debugStream.flush();
			}
			// These are currently NOT parameterized through the ParamReader, but they could be
			// This has not been rigorously tested, you might want to check my work if you use
			// something other than the current path through this logic.
			double p1_undergen = 0.6;
			double maxent_threshold = 0.6;
			bool use_p1_only = false;
			bool use_maxent_only = false;
			bool take_intersection = false; // can only be true if use_p1_only and use_maxent_only are false
			bool prefer_p1_model = true; // determines which to prefer when operating in "union" mode
			
			// Get raw answers (using precision thresholds)
			_decoder->setUndergenPercentage(p1_undergen);
			Symbol p1_answer = _decoder->decodeToSymbol(_observation);
			int max_ent_besttag = 0;
			int n_maxent = _maxentDecoder->decodeToDistribution(_observation, max_ent_scores, 
				_tagSet->getNTags(), &max_ent_besttag);
			Symbol maxent_answer = _tagSet->getTagSymbol(max_ent_besttag);
			if (max_ent_scores[max_ent_besttag] < maxent_threshold)
				maxent_answer = _tagSet->getNoneTag();

			// Combine answers intelligently
			Symbol answer = _tagSet->getNoneTag();
			if (use_p1_only)
				answer = p1_answer;
			else if (use_maxent_only)
				answer = maxent_answer;
			else if (take_intersection) {
				if (maxent_answer != p1_answer)
					answer = _tagSet->getNoneTag();
			} else {
				// union
				if (prefer_p1_model) {
					// ******* CURRENT APPROACH ********
					answer = p1_answer;
					if (answer == _tagSet->getNoneTag())
						answer = maxent_answer;
				} else {
					answer = maxent_answer;
					if (answer == _tagSet->getNoneTag())
						answer = p1_answer;
				}
			} 
			/*//this is only marginally helpful, leave it off for now
			else if((answer != _tagSet->getNoneTag()) && (max_ent_sym != _tagSet->getNoneTag())){
					answer = answer;
			}
			else{
				double best_score = 0;
				double second_best = 0;
				int second = 0;
				int first = 0;
				for(int s = 0; s < _tagSet->getNTags(); s++){
					if(max_ent_scores[s] > best_score){
						second_best = best_score;
						second = first;
						best_score = max_ent_scores[s];
						first = s;
					}
				}
				if(_tagSet->getTagSymbol(second) == answer){
					answer = answer;
				}
				else{
					answer = _tagSet->getNoneTag();
				}
			}
			*/

			if (answer != _tagSet->getNoneTag()) {
					bool reverse = _reverseRelation(answer, _observation);
					if (reverse){
						/*std::cout<<"Reversing a relation: "<<answer.to_debug_string()
							<<"Mention 1: ";
						_observation->getMention1()->dump(std::cout);
						std::cout<<" Mention 2: ";
						_observation->getMention2()->dump(std::cout);*/

						addRelation(_observation->getMention2(), _observation->getMention1(), answer);
					}
					else addRelation(_observation->getMention1(), _observation->getMention2(), answer);
				}
			if(mentIIsPartitive)
				mentionSet->getMention(i)->mentionType = Mention::PART;
			if(mentJIsPartitive)
				mentionSet->getMention(j)->mentionType = Mention::PART;

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"\n";
				_decoder->_debugStream.flush();
			}


		}
	}

	RelMentionSet *result = _new RelMentionSet();
	for (int j = 0; j < _n_relations; j++) {
		result->takeRelMention(_relations[j]);
		_relations[j] = 0;
	}
	delete [] knownTypes;
	delete [] max_ent_scores;
	/*
	std::cout<<"\tEnding Mention and Entity Sets\n";
	mentionSet->dump(std::cout, 8);
	std::cout<<std::endl;
	entitySet->dump(std::cout, 8);
	std::cout<<std::endl;
	std::cout<<"\n*****************Finished Relation Finding **************\n";
	*/

	return result;
}

Symbol ArabicP1RelationFinder::findPronounRelation(Mention *first, Mention* second, EntityType currType1,
											 EntityType currType2,
											 EntityType& firstType, EntityType& secondType)
{
	int nenttypes = EntityType::getNTypes();
	EntityType origFirstEntType = first->getEntityType();
	EntityType origSecondEntType = second->getEntityType();
	double relScore[15];
	const int MAX_PRON_RELATIONS = 15;
	EntityType relFirstEntType[MAX_PRON_RELATIONS];
	EntityType relSecondEntType[MAX_PRON_RELATIONS];
	Symbol relationType[MAX_PRON_RELATIONS];
	int nrelations = 0;
	if((first->getMentionType() == Mention::PRON) && (second->getMentionType() == Mention::PRON)){
		if(_allowPronTypeChange != Symbol(L"all")){
			firstType = currType1;
			secondType = currType2;
			return _tagSet->getNoneTag();	//assume these will be bogus relations

		}
//		std::cout<<"Look for a relation between 2 pronouns"<<std::endl;
		if((currType1 == EntityType::getUndetType()) &&(currType2 == EntityType::getUndetType())){
			for(int i=0; i<nenttypes; i++){
				if(EntityType::getType(i) == EntityType::getOtherType())
					continue;
				for(int j=0; j<nenttypes; j++){
					if(EntityType::getType(j) == EntityType::getOtherType())
						continue;
					first->setEntityType(EntityType::getType(i));
					second->setEntityType(EntityType::getType(j));
					double score;
					Symbol answer = _decodePronounRelation(first, second, score);
					if((answer != _tagSet->getNoneTag() && nrelations < MAX_PRON_RELATIONS)){
						/*
						std::cout<<"Found a relation: "<<score<<" "<<
							first->getEntityType().getName().to_debug_string()<<" "
							<<second->getEntityType().getName().to_debug_string()<<" "
							<<answer.to_debug_string()<<std::endl;
						std::cout<<"Mentions from observation: "<<std::endl;

						_observation->getMention1()->dump(std::cout);
						std::cout<<std::endl;
						_observation->getMention2()->dump(std::cout);
						std::cout<<std::endl;
						*/

						relScore[nrelations] =score;
						relFirstEntType[nrelations] = first->getEntityType();
						relSecondEntType[nrelations] = second->getEntityType();
						relationType[nrelations] = answer;
						nrelations++;
					}
				}
			}
			first->setEntityType(origFirstEntType);
			second->setEntityType(origSecondEntType);
			if(nrelations ==0){
				firstType = origFirstEntType;
				secondType = origSecondEntType;
				return _tagSet->getNoneTag();
			}
			int bestindex = 0;
			double bestscore = relScore[0];
			for(int j=1; j<nrelations; j++){
				if(relScore[j] > bestscore){
					bestscore = relScore[j];
					bestindex = j;
				}
			}
			firstType = relFirstEntType[bestindex];
			secondType = relSecondEntType[bestindex];
			return relationType[bestindex];
		}
		else if(currType1 == EntityType::getUndetType()){
			/*
			std::cout<<"Look for a relation between pronoun and ";
			second->dump(std::cout);
			std::cout<<std::endl;
			*/
			if(second->getEntityType() == EntityType::getOtherType())
				return _tagSet->getNoneTag();
			if(!second->isOfRecognizedType())
				return _tagSet->getNoneTag();
			for(int i=0; i<nenttypes; i++){
				if(EntityType::getType(i) == EntityType::getOtherType())
					continue;
				first->setEntityType(EntityType::getType(i));
				double score = 0;
				Symbol answer = _decodePronounRelation(first, second, score);
				if(answer != _tagSet->getNoneTag() && (nrelations > MAX_PRON_RELATIONS)){
					/*
					std::cout<<"Found a relation: "<<score<<" "<<
						first->getEntityType().getName().to_debug_string()<<" "
						<<second->getEntityType().getName().to_debug_string()<<" "
						<<answer.to_debug_string()<<std::endl;
					std::cout<<"Mentions from observation: "<<std::endl;
					_observation->getMention1()->dump(std::cout);
					std::cout<<std::endl;
					_observation->getMention2()->dump(std::cout);
					std::cout<<std::endl;
					*/

					relScore[nrelations] =score;
					relFirstEntType[nrelations] = first->getEntityType();
					relationType[nrelations] = answer;
					nrelations++;
				}
			}

			first->setEntityType(origFirstEntType);
			second->setEntityType(origSecondEntType);
			secondType = origSecondEntType;
			if(nrelations ==0){
				firstType = origFirstEntType;
				return _tagSet->getNoneTag();
			}
			int bestindex =0;
			double bestscore = relScore[0];
			for(int j=1; j<nrelations; j++){
				if(relScore[j] > bestscore){
					bestscore = relScore[j];
					bestindex = j;
				}
			}
			firstType = relFirstEntType[bestindex];
			return relationType[bestindex];
		}
		else if(currType2 == EntityType::getUndetType()){
			/*
			std::cout<<"Look for a relation between ";
			first->dump(std::cout);
			std::cout<<" and a pronoun"<<std::endl;
			*/
			if(first->getEntityType() == EntityType::getOtherType())
				return _tagSet->getNoneTag();
			if(!first->isOfRecognizedType())
				return _tagSet->getNoneTag();

			for(int i=0; i<nenttypes; i++){
				if(EntityType::getType(i) == EntityType::getOtherType())
					continue;
				if(first->getEntityType() == EntityType::getOtherType())
					return _tagSet->getNoneTag();

				second->setEntityType(EntityType::getType(i));
				double score = 0;
				Symbol answer = _decodePronounRelation(first,second, score);
				if((answer != _tagSet->getNoneTag()) && (nrelations < MAX_PRON_RELATIONS)){
					/*
					std::cout<<"Found a relation: "<<score<<" "<<
						first->getEntityType().getName().to_debug_string()<<" "
						<<second->getEntityType().getName().to_debug_string()<<" "
						<<answer.to_debug_string()<<std::endl;
					std::cout<<"Mentions from observation: "<<std::endl;
					_observation->getMention1()->dump(std::cout);
					std::cout<<std::endl;
					_observation->getMention2()->dump(std::cout);
					std::cout<<std::endl;
					*/
					relScore[nrelations] =score;
					relSecondEntType[nrelations] = second->getEntityType();
					relationType[nrelations] = answer;
					nrelations++;

				}
			}

			first->setEntityType(origFirstEntType);
			second->setEntityType(origSecondEntType);
			firstType = origFirstEntType;
			if(nrelations ==0){
				secondType = origSecondEntType;
				return _tagSet->getNoneTag();
			}
			int bestindex =0;
			double bestscore = relScore[0];
			for(int j=1; j<nrelations; j++){
				if(relScore[j] > bestscore){
					bestscore = relScore[j];
					bestindex = j;
				}
			}
			secondType = relSecondEntType[bestindex];
			return relationType[bestindex];
		}
		else{
			double score;
			Symbol answer = _decodePronounRelation(first, second, score);
			firstType = first->getEntityType();
			secondType = second->getEntityType();
			return answer;
		}
	}
	else if(first->getMentionType() == Mention::PRON){
		if(currType1 == EntityType::getUndetType()){
			/*
			std::cout<<"Look for a relation between pronoun and ";
			second->dump(std::cout);
			std::cout<<std::endl;
			*/
			if(second->getEntityType() == EntityType::getOtherType())
				return _tagSet->getNoneTag();
			if(!second->isOfRecognizedType())
				return _tagSet->getNoneTag();
			for(int i=0; i<nenttypes; i++){
				if(EntityType::getType(i) == EntityType::getOtherType())
					continue;
				first->setEntityType(EntityType::getType(i));
				double score;
				Symbol answer = _decodePronounRelation(first, second, score);
				if( (answer != _tagSet->getNoneTag()) && (nrelations < MAX_PRON_RELATIONS)){
					/*
					std::cout<<"Found a relation: "<<score<<" "<<
						first->getEntityType().getName().to_debug_string()<<" "
						<<second->getEntityType().getName().to_debug_string()<<" "
						<<answer.to_debug_string()<<std::endl;
					std::cout<<"Mentions from observation: "<<std::endl;
					_observation->getMention1()->dump(std::cout);
					std::cout<<std::endl;
					_observation->getMention2()->dump(std::cout);
					std::cout<<std::endl;
					*/

					relScore[nrelations] =score;
					relFirstEntType[nrelations] = first->getEntityType();
					relationType[nrelations] = answer;
					nrelations++;
				}
			}

			first->setEntityType(origFirstEntType);
			second->setEntityType(origSecondEntType);
			secondType = origSecondEntType;
			if(nrelations ==0){
				firstType = origFirstEntType;
				return _tagSet->getNoneTag();
			}
			int bestindex =0;
			double bestscore = relScore[0];
			for(int j=1; j<nrelations; j++){
				if(relScore[j] > bestscore){
					bestscore = relScore[j];
					bestindex = j;
				}
			}
			firstType = relFirstEntType[bestindex];
			return relationType[bestindex];



		}
		else{
			double score;
			Symbol answer = _decodePronounRelation(first, second, score);
			firstType = first->getEntityType();
			secondType = second->getEntityType();
			return answer;
		}

	}
	else if(second->getMentionType() == Mention::PRON){
		if(currType2 == EntityType::getUndetType()){
			/*
			std::cout<<"Look for a relation between ";
			first->dump(std::cout);
			std::cout<<" and a pronoun"<<std::endl;
			*/
			if(first->getEntityType() == EntityType::getOtherType())
				return _tagSet->getNoneTag();
			if(!first->isOfRecognizedType())
				return _tagSet->getNoneTag();

			for(int i=0; i<nenttypes; i++){
				if(EntityType::getType(i) == EntityType::getOtherType())
					continue;
				second->setEntityType(EntityType::getType(i));
				double score;
				Symbol answer = _decodePronounRelation(first, second, score);
				if((answer != _tagSet->getNoneTag()) && (nrelations < MAX_PRON_RELATIONS)){
					/*

					std::cout<<"Found a relation: "<<score<<" "<<
						first->getEntityType().getName().to_debug_string()<<" "
						<<second->getEntityType().getName().to_debug_string()<<" "
						<<answer.to_debug_string()<<std::endl;
					std::cout<<"Mentions from observation: "<<std::endl;
					_observation->getMention1()->dump(std::cout);
					std::cout<<std::endl;
					_observation->getMention2()->dump(std::cout);
					std::cout<<std::endl;
					*/
					relScore[nrelations] =score;
					relSecondEntType[nrelations] = second->getEntityType();
					relationType[nrelations] = answer;
					nrelations++;
				}
			}

			first->setEntityType(origFirstEntType);
			second->setEntityType(origSecondEntType);
			firstType = origFirstEntType;
			if(nrelations ==0){
				secondType = origSecondEntType;
				return _tagSet->getNoneTag();
			}
			int bestindex =0;
			double bestscore = relScore[0];
			for(int j=1; j<nrelations; j++){
				if(relScore[j] > bestscore){
					bestscore = relScore[j];
					bestindex = j;
				}
			}
			secondType = relSecondEntType[bestindex];
			return relationType[bestindex];
		}
		else{
			double score;
			Symbol answer = _decodePronounRelation(first, second, score);
			firstType = first->getEntityType();
			secondType = second->getEntityType();
			return answer;
		}
	}

	return _tagSet->getNoneTag();
}
/*
//2004 Relation types - 
Symbol EMP_ORG_EXEC(L"EMP-ORG.Employ-Executive");
Symbol EMP_ORG_STAFF(L"EMP-ORG.Employ-Staff");
Symbol GPE_AFF(L"GPE-AFF");
void ArabicP1RelationFinder::addRelation(const Mention *first, const Mention *second, Symbol type) {
	if (RelationConstants::getBaseTypeSymbol(type) == GPE_AFF) {
		if (_execTable != 0) {
			if  (_execTable->lookup(first->getNode()->getHeadWord()))
				type = EMP_ORG_EXEC;
			else if  (_execTable->lookup(second->getNode()->getHeadWord())) {
				type = EMP_ORG_EXEC;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
		if (_staffTable != 0) {
			if (_staffTable->lookup(first->getNode()->getHeadWord()))
				type = EMP_ORG_STAFF;
			else if (_staffTable->lookup(second->getNode()->getHeadWord())) {
				type = EMP_ORG_STAFF;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
	}

	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			type, _currentSentenceIndex, _n_relations, 0);
		_n_relations++;
	}

}
*/
//2005 relation types
//ARG1 = PER, ARG2 = Employer (this includes American President, Russian soldier, etc.
Symbol ORG_AFF_EMP(L"ORG-AFF.Employment");
Symbol GEN_AFF(L"GEN-AFF");
void ArabicP1RelationFinder::addRelation(const Mention *first, const Mention *second, Symbol type) {
	/*
	//we seem to not miss these in 2005
	if (RelationConstants::getBaseTypeSymbol(type) == GEN_AFF) {
		if (_execTable != 0) {
			if  (_execTable->lookup(first->getNode()->getHeadWord()))
				type = ORG_AFF_EMP;
			else if  (_execTable->lookup(second->getNode()->getHeadWord())) {
				type = ORG_AFF_EMP;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
		if (_staffTable != 0) {
			if (_staffTable->lookup(first->getNode()->getHeadWord()))
				type = ORG_AFF_EMP;
			else if (_staffTable->lookup(second->getNode()->getHeadWord())) {
				type = ORG_AFF_EMP;
				const Mention *temp = first;
                first = second;
                second = temp;
			}
		}
	}
	*/
	//the LDC data sometimes marks a relation between AFP and the city it was written in.
	//ignore these, since Valorem didn't mark them
	bool skip_relation = false;
	
	if(first->getSentenceNumber() == 0){
		if((first->getMentionType() == Mention::NAME) && 
			(second->getMentionType() == Mention::NAME))
		{
			const Mention* org = 0;
			if(first->getEntityType().matchesGPE() && second->getEntityType().matchesORG()){
				org = second;
			}
			else if(first->getEntityType().matchesORG() && second->getEntityType().matchesGPE()){
				org = first;
			}
			if(org != 0){
				Symbol name_words[5];
				int nwords = org->getNode()->getHeadPreterm()->getParent()->getTerminalSymbols(name_words, 5);
				if(nwords == 1){
					if(name_words[0] == ArabicSymbol(L"Af")){
							skip_relation = true;
					}
				}
				if(nwords == 2){
					if((name_words[0] == ArabicSymbol(L"Af")) 
						&& (name_words[1] == ArabicSymbol(L"b"))){
							skip_relation = true;
					}
				}
			}
		}
	}
	if(skip_relation){
		return;
	}
	

	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			type, _currentSentenceIndex, _n_relations, 0);
		_n_relations++;
	}

}

/*void ArabicP1RelationFinder::linkPronounToEntity(Mention* ment, Entity* thisEnt, Entity *othEnt,
										   EntitySet* entitySet, MentionSet* mentSet){
	if((thisEnt != 0 )&&(ment->getEntityType() == thisEnt->getType())){
		return;
	}


	//std::cout<<"\tLink pronoun mention: ";
	//ment->dump(std::cout);
	//std::cout<<"\tMention Set: \n\t";
	//mentSet->dump(std::cout, 8);
	//std::cout<<std::endl;

	//std::cout<<"\tEntity Set: \n\t";
	//entitySet->dump(std::cout, 8);
	//std::cout<<std::endl;
	//std::cout<<"\n---------------------------\n";

	GrowableArray <Entity *> possibleEnts = entitySet->getEntitiesByType(ment->getEntityType());
	int sentPenalty = 500;
	Entity* closestMatch = 0;
	int matchDist = 1000000;
	Entity* closest = 0;
	int closestDist = 1000000;
	Entity* currEnt;
	Mention* currMent;
	//const SynNode* currNode;
	int thisMentSentNum = ment->getSentenceNumber();
	const SynNode* thisMentNode = ment->getNode();
	for(int i=0; i< possibleEnts.length(); i++){
		currEnt = possibleEnts[i];
		if(currEnt == othEnt)
			continue;
		if(currEnt == thisEnt)
			continue;

		for(int j = 0; j<currEnt->getNMentions(); j++){
			currMent = entitySet->getMention(currEnt->getMention(j));
			if(currMent->getSentenceNumber() > thisMentSentNum){
				continue;
			}
			int dist = (thisMentSentNum - currMent->getSentenceNumber()) * sentPenalty;
			if(dist == 0){
				if(currMent->getNode()->getStartToken() < thisMentNode->getStartToken()){
					dist = thisMentNode->getStartToken() - currMent->getNode()->getStartToken();
				}
				else dist = 1000000;
			}
			if(dist < closestDist){
				closest = currEnt;
				closestDist = dist;
			}
			if(_pronounAndNounAgree(ment, ment->getNode(), currMent->getNode())){
				if(dist < matchDist){
					closestMatch = currEnt;
					matchDist = dist;
				}
			}
		}
	}
	if(closestMatch != 0){
		if(thisEnt){
			if(thisEnt->getNMentions() > 1){
				thisEnt->removeMention(ment->getUID());
			}
			else{
				//somewhat hackish, don't remove the mention, change its intended type
				//to be thisEnt's type, change thisEnts type to UNDET, so it doesn't print
				//does this mess up getEntityByType()
				thisEnt->type = EntityType::getUndetType();
				ment->setIntendedType(EntityType::getUndetType());
			}
		}
		closestMatch->addMention(ment->getUID());
	}
	else if(closest != 0){
		if(thisEnt){
			if(thisEnt->getNMentions() > 1){
				thisEnt->removeMention(ment->getUID());
			}
			else{
				//somewhat hackish, don't remove the mention, change its intended type
				//to be thisEnt's type, change thisEnts type to UNDET, so it doesn't print
				//does this mess up getEntityByType()
				thisEnt->type = EntityType::getUndetType();
				ment->setIntendedType(EntityType::getUndetType());
			}
		}



		closest->addMention(ment->getUID());
	}
	else{
		if(thisEnt && (thisEnt->getNMentions() == 1)){
			//note, we probably won't see this entity when we get entities by type, but its a pron...
			thisEnt->type = ment->getEntityType();
		}
		else{
			if(thisEnt){
				//somewhat hackish, don't remove the mention, change its intended type
				//to be thisEnt's type, change thisEnts type to UNDET, so it doesn't print
				//does this mess up getEntityByType()
				thisEnt->type = EntityType::getUndetType();
				ment->setIntendedType(EntityType::getUndetType());
			}
			entitySet->addNew(ment->getUID(), ment->getEntityType());
		}
	}
	//since the entity set has a copy of the mention set, not a pointer to
	//the mention set, update the the entity sets mention set as well
	const MentionSet* thisSentEntMentSet = entitySet->getMentionSet(thisMentSentNum);
	thisSentEntMentSet->getMention(ment->getUID())->setEntityType(ment->getEntityType();
	thisSentEntMentSet->getMention(ment->getUID())->setEntitySubtype(EntitySubtype::getUndetType();



}
*/
bool ArabicP1RelationFinder::isValidRelationEntityTypeCombo(Symbol relationType, EntityType ment1Type,
													  EntityType ment2Type)
{
	Symbol type = RelationConstants::getBaseTypeSymbol(relationType);
	Symbol subtype = RelationConstants::getSubtypeSymbol(relationType);
	/*
		I made these up from the guidelines, trying to filter out truly stupid relations
	*/
	if(type == Symbol(L"EMP-ORG")){
		if(subtype == Symbol(L"Subsidiary")){
			return
				((ment1Type == EntityType::getORGType()) && (ment2Type == EntityType::getORGType()));
		}
		if(subtype == Symbol(L"Member-of-Group")){
			if(ment1Type == EntityType::getORGType()){
				if( (ment2Type ==EntityType::getGPEType()) || (ment2Type == EntityType::getPERType())){
					return true;
				}
			}
			if(ment2Type == EntityType::getORGType()){
				if( (ment1Type ==EntityType::getGPEType()) || (ment1Type == EntityType::getPERType())){
					return true;
				}
			}
			return false;
		}
		if(subtype == Symbol(L"Employ-Staff")){
			if(ment1Type == EntityType::getPERType()){
				if( (ment2Type ==EntityType::getGPEType()) || (ment2Type == EntityType::getORGType())){
					return true;
				}
			}
			if(ment2Type == EntityType::getPERType()){
				if( (ment1Type ==EntityType::getGPEType()) || (ment1Type == EntityType::getORGType())){
					return true;
				}
			}
			return false;
		}

	}
	if(type == Symbol(L"GPE-AFF")){
		if(ment1Type == EntityType::getGPEType()){
			if( (ment2Type ==EntityType::getORGType()) || (ment2Type == EntityType::getPERType())){
				return true;
			}
		}
		if(ment2Type == EntityType::getGPEType()){
			if( (ment1Type ==EntityType::getORGType()) || (ment1Type == EntityType::getPERType())){
				return true;
			}
		}
		return false;
	}

	if(type == Symbol(L"PER-SOC")){
		return ((ment1Type == EntityType::getPERType()) && (ment2Type  == EntityType::getPERType()));
	}

	if(type == Symbol(L"DISC")){
		return (ment1Type == ment2Type);
	}
	if(type == Symbol(L"PHYS")){
		return ((ment1Type == EntityType::getLOCType()) || (ment2Type == EntityType::getGPEType()));
	}
	if(type == Symbol(L"ART")){
		return
			((ment1Type == EntityType::getGPEType()) ||
			(ment1Type ==EntityType::getORGType()) ||
			(ment1Type == EntityType::getPERType()) ||
			(ment2Type == EntityType::getGPEType()) ||
			(ment2Type ==EntityType::getORGType()) ||
			(ment2Type == EntityType::getPERType()));
	}



	return false;
}



Symbol ArabicP1RelationFinder::_decodePronounRelation(Mention* first, Mention* second, double& score)
{
	_observation->populate(first->getIndex(), second->getIndex());
	if (_decoder->DEBUG) {
		_decoder->_debugStream << L" * " << first->getUID() << " " << first->getNode()->toTextString() << L"\n";
		_decoder->_debugStream << first->getNode()->toPrettyParse(3) << L"\n";
		_decoder->_debugStream << L" * Pronoun" << second->getUID() << " " << second->getNode()->toTextString() << L"\n";
		_decoder->_debugStream << second->getNode()->toPrettyParse(3) << L"\n";
		_decoder->_debugStream.flush();
	}
	double s = 0;
	Symbol answer = _decoder->decodeToSymbol(_observation, s);
	if(!isValidRelationEntityTypeCombo(answer, first->getEntityType(), second->getEntityType()))
	{
			//std::cout<<"Supressing invalid relation"
			//	<<first->getEntityType().getName().to_debug_string()<<" "
			//	<<second->getEntityType().getName().to_debug_string()<<" "
			//	<<answer.to_debug_string()<<std::endl;
			answer = _tagSet->getNoneTag();
	}
	score = s;
	return answer;
}
/*
This is copied from the highly imperfect Pronoun linker,
It ignores problems like the gender of names,
*/
/*
bool ArabicP1RelationFinder::_pronounAndNounAgree(Mention* ment, const SynNode* pron, const SynNode* np){

	Symbol gender = WordConstants::guessGender(np->getHeadWord());
	Symbol number  = WordConstants::guessNumber(np->getHeadWord());

	if((ment->getMentionType() == Mention::NAME) && (ment->getEntityType().matchesPER())){
		if(_mascSing->lookup(SymbolUtilities::getFullNameSymbol(np)) &&
			_femSing->lookup(SymbolUtilities::getFullNameSymbol(np))){
				return true;
			}
		if(_mascSing->lookup(np->getHeadWord()) && _femSing->lookup(np->getHeadWord())){
			return true;
		}
		if(_mascSing->lookup(SymbolUtilities::getFullNameSymbol(np))){
			gender = ArabicWordConstants::MASC;
			number = ArabicWordConstants::SINGULAR;
		}
		else if(_femSing->lookup(SymbolUtilities::getFullNameSymbol(np))){
			gender = ArabicWordConstants::FEM;
			number = ArabicWordConstants::SINGULAR;
		}

		else if(_mascSing->lookup(np->getHeadWord())){
			gender = ArabicWordConstants::MASC;
			number = ArabicWordConstants::SINGULAR;
		}
		else if(_femSing->lookup(np->getHeadWord())){
			gender = ArabicWordConstants::FEM;
			number = ArabicWordConstants::SINGULAR;
		}
		else{
			;	//probably a nationality
		}
	}
	else if(!_doGenderMatch){
		return true;
	}

	const SynNode* preterm = pron;
	while(!preterm->isPreterminal()){
		preterm = preterm->getHead();
	}

	Symbol prontag = preterm->getTag();
	if(ArabicSTags::isFemalePronPOS(prontag)){
		if(gender != ArabicWordConstants::FEM) return false;
		if(number == ArabicWordConstants::PLURAL){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return false;
			}
			else if(ArabicSTags::isPluralPronPOS(prontag)){
				return true;
			}
		}
		else if((number == ArabicWordConstants::SINGULAR)){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return true;
			}
			else if(ArabicSTags::isPluralPronPOS(prontag)){
				return false;
			}
		}
	}
	if(ArabicSTags::isMalePronPOS(prontag)){
		if(gender != ArabicWordConstants::MASC) return false;
		if(number == ArabicWordConstants::SINGULAR){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return true;
			}
			else return false;
		}
		if(number == ArabicWordConstants::PLURAL){
			if(ArabicSTags::isPluralPronPOS(prontag)){
				return true;
			}
			else return false;
		}
	}

	return true;

}
*/


bool ArabicP1RelationFinder::_reverseRelation(Symbol relationtype, RelationObservation* obs){
	if(_reverseRelations == Symbol(L"none")){
		return false;
	}
	if( (_reverseRelations == Symbol(L"both")) || (_reverseRelations == Symbol(L"rule"))){
		if(!RelationUtilities::get()->is2005ValidRelationEntityTypeCombo(obs->getMention1(), 
			obs->getMention2(), relationtype)){
				if(RelationUtilities::get()->is2005ValidRelationEntityTypeCombo(
					obs->getMention2(), obs->getMention1(), relationtype)){
						return true;
					}
			}
	}
	/*
	//2004 Rules
	Symbol EMP_ORG(L"EMP-ORG");
	Symbol GPE_AFF(L"GPE-AFF");
	Symbol ART(L"ART");
	const Mention* first = obs->getMention1();
	const Mention* second = obs->getMention2();
	if( (_reverseRelations == Symbol(L"both")) || (_reverseRelations == Symbol(L"rule"))){
		if (RelationConstants::getBaseTypeSymbol(relationtype) == EMP_ORG ||
			RelationConstants::getBaseTypeSymbol(relationtype) == ART)
		{
			if(first->getEntityType().matchesPER()){
				return false;
			}
			else if (second->getEntityType().matchesPER()) {
				return true;
			}
		} else if (RelationConstants::getBaseTypeSymbol(relationtype) == GPE_AFF) {
			if(second->getEntityType().matchesGPE()){
				return false;
			}
			else if (first->getEntityType().matchesGPE()) {
				return true;
			}
		}
	}
	*/
//Liz's code to check the order of arguments
	bool reverse = false;
	if( (_reverseRelations == Symbol(L"both")) || (_reverseRelations == Symbol(L"model"))){

		int int_answer = RelationTypeSet::getTypeFromSymbol(relationtype);
		_inst->setStandardInstance(_observation);
		if (!RelationTypeSet::isSymmetric(int_answer)) {
			_inst->setRelationType(relationtype);
			float non_reversed_score = _vectorModel->lookupB2P(_inst);

			int rev_int_answer = RelationTypeSet::reverse(int_answer);
			_inst->setRelationType(RelationTypeSet::getRelationSymbol(rev_int_answer));
			float reversed_score = _vectorModel->lookupB2P(_inst);

			if (_decoder->DEBUG) {
				_decoder->_debugStream << L"Reversed: " << reversed_score << "\n";
				_decoder->_debugStream << L"Not reversed: " << non_reversed_score << "\n";
			}

			if (non_reversed_score < reversed_score)
				reverse = true;
			if (_decoder->DEBUG && reverse) {
				_decoder->_debugStream << L"SO... reverse\n";
			}

		}
	}
	return  reverse;
}
void ArabicP1RelationFinder::loadSymbolHash(SymbolHash *hash, const char* file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	if (stream.fail()) {
		return;
	}
	//std::cout<<"Loading: "<<file<<std::endl;

	std::wstring line;
	while (!stream.eof()) {
		stream.getLine(line);
		if (line.size() == 0 || line.at(0) == L'#')
			continue;
		std::transform(line.begin(), line.end(), line.begin(), towlower);
		Symbol lineSym(line.c_str());
		hash->add(lineSym);
	}

	stream.close();
}

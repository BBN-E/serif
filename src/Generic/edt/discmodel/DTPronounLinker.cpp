// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/P1Decoder.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/edt/discmodel/DTPronounLinker.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/HobbsDistance.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/discmodel/CorefUtils.h"
#include "Generic/edt/PronounLinker.h"
#include "Generic/common/version.h"

#include <boost/algorithm/string.hpp>

DebugStream &DTPronounLinker::_debugStream = PronounLinker::getDebugStream();

DTPronounLinker::DTPronounLinker() : _featureTypes(0), _tagSet(0), _tagScores(0),
	_p1Decoder(0), _maxEntDecoder(0), _weights(0), _discard_new_pronoun_entities(false),
	_p1_overgen_threshold(0.0), _rank_overgen_threshold(0.0),
	_model_outside_and_within_sentence_links_separately(false),
	_sentenceP1Decoder(0), _outsideP1Decoder(0), _sentenceWeights(0), _outsideWeights(0),
	_noneFeatureTypes(0), _featureTypesArr(0)
{
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");

	DTCorefFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	_observation = _new DTCorefObservation();

	// if true we do not create new entities from pronouns
	_discard_new_pronoun_entities = ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_pron_discard_new_pronoun_entities", false);

	// COREF MODEL TYPE -- MaxEnt or P1
	std::string buffer = ParamReader::getRequiredParam("dt_pron_model_type");
	if (boost::iequals(buffer, "maxent"))
		MODEL_TYPE = MAX_ENT;
	else if (boost::iequals(buffer, "p1"))
		MODEL_TYPE = P1;
	else if (boost::iequals(buffer, "p1_ranking"))
		MODEL_TYPE = P1_RANKING;
	else 
		throw UnrecoverableException("DTPronounLinker()::DTPronounLinker()",
		"Parameter 'dt_pron_model_type' must be set to 'maxent', 'p1' or 'p1_ranking'");

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("dt_pron_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("dt_pron_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), DTCorefFeatureType::modeltype);

	// MODEL FILE NAME
	std::string model_file = ParamReader::getRequiredParam("dt_pron_model_file");

	// MODEL OUTSIDE AND WITHIN SENTENCE LINKS SEPARATELY
	_model_outside_and_within_sentence_links_separately = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("dt_pron_model_outside_and_within_sentence_links_separately", false);
	if (_model_outside_and_within_sentence_links_separately && MODEL_TYPE != P1_RANKING) {
		throw UnexpectedInputException("DTPronounLinker::DTPronounLinker()",
				"dt_pron_model_outside_and_within_sentence_separately should only be true when dt_pron_model_type is P1_RANKING");
	}

	_weights = _new DTFeature::FeatureWeightMap(500009);

	if (MODEL_TYPE == P1) {
		std::string file = model_file + "-p1";
		DTFeature::readWeights(*_weights, file.c_str(), DTCorefFeatureType::modeltype);

		double overgen_percentage = ParamReader::getOptionalFloatParamWithDefaultValue("dt_pron_overgen_percentage", 0);
		if (overgen_percentage < 0.0 || overgen_percentage > 100.0)
			throw UnrecoverableException("DTNameLinker::DTNameLinker()",
			"Parameter 'dt_name_coref_overgen_percentage' must range from 0 to 100");

		_p1_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_pron_overgen_threshold", 0);

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _weights, overgen_percentage);
	} 
	else if (MODEL_TYPE == P1_RANKING) {
		
		if (!_model_outside_and_within_sentence_links_separately) {
			std::string file = model_file + "-rank";
			DTFeature::readWeights(*_weights, file.c_str(), DTCorefFeatureType::modeltype);
		} else {
			_sentenceWeights = _new DTFeature::FeatureWeightMap(500009);
			_outsideWeights = _new DTFeature::FeatureWeightMap(500009);

			model_file = ParamReader::getRequiredParam("dt_pron_sentence_model_file");
			std::string file = model_file + "-rank";
			DTFeature::readWeights(*_sentenceWeights, file.c_str(), DTCorefFeatureType::modeltype);

			model_file = ParamReader::getRequiredParam("dt_pron_outside_model_file");
			file = model_file + "-rank";
			DTFeature::readWeights(*_outsideWeights, file.c_str(), DTCorefFeatureType::modeltype);
		}

		_noneFeatureTypes = DTCorefFeatureTypes::makeNoneFeatureTypeSet(Mention::PRON);
		_featureTypesArr = _new DTFeatureTypeSet*[_tagSet->getNTags()];
		_featureTypesArr[_tagSet->getNoneTagIndex()] = _noneFeatureTypes;
		_featureTypesArr[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _featureTypes;

		_rank_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_pron_rank_overgen_threshold", 0);

		if (!_model_outside_and_within_sentence_links_separately) {
			_p1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _weights);
		}
		else {
			_sentenceP1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _sentenceWeights);
			_outsideP1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _outsideWeights);
		}
	}
	else if (MODEL_TYPE == MAX_ENT) {
		std::string file = model_file + "-maxent";
		DTFeature::readWeights(*_weights, file.c_str(), DTCorefFeatureType::modeltype);

		_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _weights);

		_maxent_link_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_pron_maxent_link_threshold", 0.5);
	}

	_use_non_ace_entities_as_no_links = ParamReader::isParamTrue("dt_coref_use_non_ACE_ents_as_no_links");
	if(_use_non_ace_entities_as_no_links) {
		_max_non_ace_candidates = ParamReader::getRequiredIntParam("dt_pron_use_non_ACE_ents_as_no_links_max_candidates");
		std::cerr<<"number of non-ace max candidates: "<<_max_non_ace_candidates<<std::endl;
		if (_max_non_ace_candidates == 0)
			throw UnrecoverableException("DTPronounLinker()::DTPronounLinker()",
			"Parameter 'dt_coref_use_non_ACE_ents_as_no_links_max_candidates' not recognized");
	}
}

DTPronounLinker::~DTPronounLinker() {
	delete _weights;
	delete _p1Decoder;
	delete _maxEntDecoder;
	delete _tagSet;
	delete _tagScores;
	delete _featureTypes;
	delete _noneFeatureTypes;
	delete [] _featureTypesArr;

	delete _sentenceWeights;
	delete _outsideWeights;
	delete _sentenceP1Decoder;
	delete _outsideP1Decoder;
}


void DTPronounLinker::addPreviousParse(const Parse *parse) {
	_previousParses.push_back(parse);
}

void DTPronounLinker::resetPreviousParses() {
	_previousParses.clear();
}

int DTPronounLinker::linkMention (LexEntitySet * currSolution,
								  MentionUID currMentionUID,
								  EntityType linkType,
							      LexEntitySet *results[],
								  int max_results)
{
	//run getLinkGuesses() to get a scored list
	//of link possibilities, create new LexEntitySet's
	Mention *currMention = currSolution->getMention(currMentionUID);
	const SynNode *pronounNode = currMention->node;
	LinkGuess linkGuesses[64];

	max_results = (max_results > 64) ? 64 : max_results;

	if (_use_correct_answers) {
		// if we are using correct mentions, we only want to link pronouns with either
		// a recognized type (totally unfair) or, 
		// as in the EDR conditional eval, with type UNDET.
		if (currMention->getEntityType() == EntityType::getOtherType()) {
			results[0] = currSolution->fork();
			return 1;
		}
	}

//	int nGuesses = getLinkGuesses(currSolution, currMention, linkGuesses, max_results);
	int nGuesses = getLinkGuesses(currSolution, currMention, linkGuesses, 64);
	if (nGuesses > max_results) nGuesses = max_results;

	int i, nResults = 0;
	bool alreadyProcessedNullDecision = false;
	//these will be used to determine entity linking for this pronoun
	const SynNode *guessedNode;
	Mention *guessedMention;
	Entity *guessedEntity;

	if (_debugStream.isActive()) {
		_debugStream << L"-------------------------------\n";
		_debugStream << "RESULTS:\n";
	}

	for(i = 0; i < nGuesses; i++) {
		guessedNode = linkGuesses[i].guess;
		//determine mention from node
		if (guessedNode != NULL) {
			guessedMention = currSolution->getMentionSet(linkGuesses[i].sentence_num)
				->getMentionByNode(guessedNode);
			if (guessedMention == NULL) {
				throw InternalInconsistencyException("DTPronounLinker::linkMention()",
				                                     "No corresponding Mention found for guessed SynNode.");
			}
			if (guessedMention->getMentionType() == Mention::NONE) {
				const SynNode *parent = guessedMention->getNode()->getParent();
				Mention *parentMent = currSolution->getMentionSet(linkGuesses[i].sentence_num)
					->getMentionByNode(parent);
				while (parentMent && parentMent->getMentionType() == Mention::NONE) {
					parent = parent->getParent();
					parentMent = currSolution->getMentionSet(linkGuesses[i].sentence_num)
									->getMentionByNode(parent);
				}
				if (parentMent) {
					guessedMention = parentMent;
					_debugStream << "NONE mention - replaced with parent\n";
				}
			}
		}
		else guessedMention = NULL;

		//determine Entity from Mention
		if (guessedMention != NULL)
			guessedEntity = currSolution->getEntityByMention(guessedMention->getUID());
		else guessedEntity = NULL;

		//many different pronoun resolution decisions can lead to a "no-link" result.
		//therefore, only fork the LexEntitySet for the top scoring no-link guess.
		if(guessedEntity != NULL || !alreadyProcessedNullDecision)  {
			LexEntitySet *newSet = currSolution->fork();
			Mention *newMention = newSet->getMention(currMention->getUID());
			newMention->setLinkConfidence(linkGuesses[i].linkConfidence);

			if (guessedMention != NULL) {
//				newSet->getMention(currMention->getUID())->setEntityType(guessedMention->getEntityType());
				newMention->setEntityType(guessedMention->getEntityType());
				if (_debugStream.isActive()) {
					_debugStream << "CHOSE mention " << guessedMention->getUID();
					_debugStream << " entity type " << guessedMention->getEntityType().getName().to_string();
					_debugStream << " mention type " << guessedMention->getMentionType() << "\n";
				}
			} else {
				if (_debugStream.isActive()) _debugStream << "NULL mention\n";
			}

			if (guessedEntity == NULL || (_use_non_ace_entities_as_no_links && !guessedEntity->getType().isRecognized())) {
				alreadyProcessedNullDecision = true;
				// for pronouns we gave types to in relation finding, say
				if (currMention->getEntityType().isRecognized()) {
					newSet->getMention(currMention->getUID())->setEntityType(currMention->getEntityType());				
					if(!_discard_new_pronoun_entities){
						newSet->addNew(currMention->getUID(), currMention->getEntityType());
						_debugStream << "Forcing pronoun to be an entity (perhaps because it's in a relation OR because we've already classified pronouns): ";
						if (currMention->getNode()->getParent() != 0 && 
							currMention->getNode()->getParent()->getParent() != 0)
						{
							_debugStream << currMention->getNode()->getParent()->getParent()->toTextString();
						}
						_debugStream << "\n";
					} else {
						_debugStream << "Discarding pronoun mention in order not to create a new entity\n";
					}
				}else if (_use_non_ace_entities_as_no_links && guessedEntity == NULL) {
					// using non-ace candidates the no-link model is mapped into a decision to create a new entity
					//#ifdef ENGLISH_LANGUAGE // until we add isPERTypePronoun(hw) to other languages
					if (SerifVersion::isEnglish()) {
						Symbol hw = currMention->getHead()->getHeadWord();
						EntityType predictedType;
						if(WordConstants::isPERTypePronoun(hw)) {
							predictedType = EntityType::getPERType(); // ***** This needs to be modified
							_debugStream << "Forcing pronoun to be an entity of type PER (non-ACE op)(for now) \n";
						} else if(WordConstants::isLOCTypePronoun(hw)) {
							//					if(hw == WordConstants::WHERE || hw == WordConstants::HERE || 
							//						hw == WordConstants::THERE) {
							predictedType = EntityType::getLOCType();
							_debugStream << "Forcing pronoun to be an entity of type LOC (non-ACE op)(for now) \n";
						}
						newSet->getMention(currMention->getUID())->setEntityType(predictedType);	
						newSet->addNew(currMention->getUID(), predictedType);
					}
					//#endif
				} else { // not adding the pronoun
					_debugStream << "NULL entity - not added ";
					if(guessedEntity != NULL) {
						if (_debugStream.isActive()) {
							_debugStream << " because of linking to a non-ACE entity # ";
							_debugStream << guessedEntity->getID() << "   "<< guessedEntity->getType().getName().to_string();
							_debugStream << "  score: " << linkGuesses[i].score << "  ";
							_debugStream << L": [";
						}
						for (int m = 0; m < guessedEntity->getNMentions(); m++){
							_debugStream << currSolution->getMention(guessedEntity->getMention(m))->getNode()->toTextString();
							if(m != guessedEntity->getNMentions()-1) _debugStream << L" ,";
						}
						_debugStream << L"]";
					}
					_debugStream << "\n";
				}
			}
			else {
				newSet->add(currMention->getUID(), guessedEntity->getID());
				if(_debugStream.isActive()) {
					_debugStream << "LINKED to entity # " << guessedEntity->getID() << " score: " << linkGuesses[i].score << ": [";
					for (int m = 0; m < guessedEntity->getNMentions(); m++){
						_debugStream << currSolution->getMention(guessedEntity->getMention(m))->getNode()->toTextString();
						if(m != guessedEntity->getNMentions()-1) _debugStream << L" ,";
					}
					_debugStream << L"]";
					_debugStream << L"\n";
				}
			}
			results[nResults++] = newSet;

		}
		//otherwise we ignore this pronoun

	}
	return nResults;
}


int DTPronounLinker::getLinkGuesses(EntitySet *currSolution,
								    Mention *currMention,
								    LinkGuess results[],
									int max_results)
{
	LinkGuess withinSentence[64];
	LinkGuess outsideSentence[64];

	int nWithin = 0;
	int nOutside = 0;

	const int MAX_CANDIDATES = 50;
	HobbsDistance::SearchResult hobbsCandidates[MAX_CANDIDATES];
	int nHobbsCandidates;
	int nResults = 0;
	LinkGuess newGuess;
	int link_index = _tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol());
	int no_link_index = _tagSet->getNoneTagIndex();
	
	const SynNode *pronNode = currMention->getNode();
	while (pronNode->getParent() != NULL && !pronNode->getParent()->getSingleWord().is_null())
		pronNode = pronNode->getParent();
	

	_debugStream << L"\n BEGIN_GUESSES\n################\n";
	// get a feature array for the proposed linking of the mention with each appropriate
	// entity. The feature array itself is language-specific. Use the array in the probability
	// model to get a score. use the score to order the results properly.
	const GrowableArray <Entity *> &allEntities = currSolution->getEntities();
	GrowableArray <Entity *> filteredEnts;
	int n_non_ace_added = 0;
	if(_use_non_ace_entities_as_no_links){
		// keep only ace entities
		for (int i=0; i<allEntities.length(); i++) {
			Entity *ent = allEntities[i];
			if(ent->getType().isRecognized())
				filteredEnts.add(ent);
		}
		// add the closest non-ace entities
		n_non_ace_added = CorefUtils::addNearestEntities(filteredEnts, currSolution, currMention, _max_non_ace_candidates);
	}
	const GrowableArray <Entity *> &candidates = 
		_use_non_ace_entities_as_no_links ? filteredEnts : allEntities;

	int sent_num = currMention->getSentenceNumber();
	const MentionSet* mentionSet = currSolution->getMentionSet(sent_num);
	_observation->resetForNewDocument(currSolution);
	_observation->resetForNewSentence(mentionSet);

	// TO DO: do we need to search for the pronoun node, instead of using mention node?
	nHobbsCandidates = HobbsDistance::getCandidates(pronNode, _previousParses,
													hobbsCandidates, MAX_CANDIDATES);
	
	if (_debugStream.isActive()) {
		_debugStream << "\nPRONOUN: " << pronNode->getHeadWord().to_string() 
			<< "  " << pronNode->toFlatString() << ". ";
		_debugStream << "Mention " << currMention->getUID() << ". ";
		_debugStream << candidates.length() << " candidates found." << "\n";
		if(_use_non_ace_entities_as_no_links)
			_debugStream << "Out of which " << n_non_ace_added << " are of type 'OTHER'\n";
	}

	double thisscore;
	Symbol linkval;
	if (MODEL_TYPE == P1_RANKING){
		// compute the no_link score
		// add the new entity case for P1_RANKING
		static_cast<DTNoneCorefObservation*>(_observation)->populate(currMention->getUID());

		if (!_model_outside_and_within_sentence_links_separately) {
			thisscore = _p1Decoder->getScore(_observation, no_link_index) - _rank_overgen_threshold;

			newGuess.guess = NULL;
			newGuess.sentence_num = LinkGuess::NO_SENTENCE;
			newGuess.score = thisscore;
			results[nResults++] = newGuess;
			if (_debugStream.isActive()) {
				_debugStream << L"-------------------------------\n";
				_debugStream << L"CONSIDERING NO-LINK OPTION\n";
				_debugStream << L"NO-LINK SCORE: "<< newGuess.score <<" := ";
				_debugStream << _tagSet->getNoneTag().to_debug_string();
				_debugStream << L"\n";
			}
		} else {
			thisscore = _sentenceP1Decoder->getScore(_observation, no_link_index) - _rank_overgen_threshold;

			if (nWithin < max_results) {
				newGuess.guess = NULL;
				newGuess.sentence_num = LinkGuess::NO_SENTENCE;
				newGuess.score = thisscore;
				withinSentence[nWithin++] = newGuess;
				if (_debugStream.isActive()) {
					_debugStream << L"-------------------------------\n";
					_debugStream << L"CONSIDERING WITHIN SENTENCE NO-LINK OPTION\n";
					_sentenceP1Decoder->printDebugInfo(_observation, no_link_index, _debugStream);
					_debugStream << L"NO-LINK SCORE: "<< newGuess.score <<" := ";
					_debugStream << _tagSet->getNoneTag().to_debug_string();
					_debugStream << L"\n";
				}
			}

			thisscore = _outsideP1Decoder->getScore(_observation, no_link_index) - _rank_overgen_threshold;

			if (nOutside < max_results) {
				newGuess.guess = NULL;
				newGuess.sentence_num = LinkGuess::NO_SENTENCE;
				newGuess.score = thisscore;
				outsideSentence[nOutside++] = newGuess;
				if (_debugStream.isActive()) {
					_debugStream << L"-------------------------------\n";
					_debugStream << L"CONSIDERING OUTSIDE SENTENCE NO-LINK OPTION\n";
					_outsideP1Decoder->printDebugInfo(_observation, no_link_index, _debugStream);
					_debugStream << L"NO-LINK SCORE: "<< newGuess.score <<" := ";
					_debugStream << _tagSet->getNoneTag().to_debug_string();
					_debugStream << L"\n";
				}
			}
		}
	}

	for (int i = 0; i < candidates.length(); i++) {

		Entity* prevEntity = candidates[i];

		// Don't link to "simple coref" entities; too risky
		// Relative pronouns (e.g. "the drug which...") are taken care of in pre-linking for English
		if (prevEntity->getType().useSimpleCoref()) {
			continue;
		}		

		// TB: why isn't linked_badly being used?
		bool linked_badly = false;
		if (prevEntity != NULL &&
			_isLinkedBadly(currSolution, currMention, prevEntity))
			linked_badly = true;

		if (_isSpeakerEntity(prevEntity, currSolution))
			continue;

		int hobbs_distance = getHobbsDistance(currSolution, prevEntity,
											  hobbsCandidates, nHobbsCandidates);

		_observation->populate(currMention->getUID(), prevEntity->getID(), hobbs_distance);
		Mention *latestMent = _observation->getLastEntityMention();
		bool sameSentence = (latestMent->getSentenceNumber() == currMention->getSentenceNumber());

		if (_debugStream.isActive()) {
			_debugStream << L"-------------------------------\n";
			if (!_model_outside_and_within_sentence_links_separately) {
				_debugStream << L"CONSIDERING LINK TO Entity #" << prevEntity->getID() << L": [";
			} else {
				if (sameSentence)
					_debugStream << L"CONSIDERING WITHIN SENTENCE LINK TO Entity #" << prevEntity->getID() << L": [";
				else
					_debugStream << L"CONSIDERING OUTSIDE SENTENCE LINK TO Entity #" << prevEntity->getID() << L": [";
			}
			for (int m = 0; m < prevEntity->getNMentions(); m++){
				_debugStream << currSolution->getMention(prevEntity->getMention(m))->getNode()->toTextString();
				if(m != prevEntity->getNMentions()-1) _debugStream << L" ,";
			}
			_debugStream << L"]\n";
		}
		
		if (MODEL_TYPE == MAX_ENT) {
			_maxEntDecoder->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags());
			if (_tagScores[link_index] > _maxent_link_threshold) {
				linkval = _tagSet->getTagSymbol(link_index);
				thisscore = _tagScores[link_index];
			}
			else {
				linkval = _tagSet->getNoneTag();
				thisscore = _tagScores[_tagSet->getNoneTagIndex()];
			}
		}
		else if (MODEL_TYPE == P1) {
			linkval = _p1Decoder->decodeToSymbol(_observation, thisscore);
			if (linkval == _tagSet->getNoneTag() && thisscore < _p1_overgen_threshold) {
				linkval = _tagSet->getTagSymbol(link_index);
				thisscore = 1/thisscore;
			}
		}else if (MODEL_TYPE == P1_RANKING){
			if (!_model_outside_and_within_sentence_links_separately) {
				thisscore = _p1Decoder->getScore(_observation, link_index);
			} else {
				if (sameSentence) 
					thisscore = _sentenceP1Decoder->getScore(_observation, link_index);
				else 
					thisscore = _outsideP1Decoder->getScore(_observation, link_index);
			}
			linkval = _tagSet->getTagSymbol(link_index); 
		}

		if (_debugStream.isActive()) {
			if (!_model_outside_and_within_sentence_links_separately) {
				_p1Decoder->printDebugInfo(_observation, link_index, _debugStream);
			} else {
				if (sameSentence)
					_sentenceP1Decoder->printDebugInfo(_observation, link_index, _debugStream);
				else
					_outsideP1Decoder->printDebugInfo(_observation, link_index, _debugStream);
			}
			_debugStream << L"LINK SCORE " << thisscore << L" := ";
			_debugStream << linkval.to_debug_string();
			_debugStream << L"\n";
		}

		if (linkval != _tagSet->getNoneTag()) {
			Mention *latestMent = _observation->getLastEntityMention();
			newGuess.guess = latestMent->getNode();
			newGuess.sentence_num = latestMent->getSentenceNumber();
			newGuess.score = thisscore;

			if (MODEL_TYPE != P1_RANKING || !_model_outside_and_within_sentence_links_separately) {	
				// add results in sorted order
				bool insertedSolution = false;
				for (int p = 0; p < nResults; p++) {
					if(results[p].score < newGuess.score) {
						if (nResults == max_results)
							nResults--;
						for (int k = nResults; k > p; k--)
							results[k] = results[k-1];
						results[p] = newGuess;
						nResults++;
						insertedSolution = true;
						break;
					}
				}
				// solution doesn't make the cut. if there's room, insert it at the end
				// otherwise, ditch it
				if (!insertedSolution) {
					if (nResults < max_results)
						results[nResults++] = newGuess;
				}
			} else {  //MODEL_TYPE == P1_RANKING && _model_outside_and_within_sentence_links_separately
				if (sameSentence) {
					bool insertedSolution = false;
					for (int p = 0; p < nWithin; p++) {
						if(withinSentence[p].score < newGuess.score) {
							if (nWithin == max_results)
								nWithin--;
							for (int k = nWithin; k > p; k--)
								withinSentence[k] = withinSentence[k-1];
							withinSentence[p] = newGuess;
							nWithin++;
							insertedSolution = true;
							break;
						}
					}
					// solution doesn't make the cut. if there's room, insert it at the end
					// otherwise, ditch it
					if (!insertedSolution) {
						if (nWithin < max_results)
							withinSentence[nWithin++] = newGuess;
					}
				}
				else {
					bool insertedSolution = false;
					for (int p = 0; p < nOutside; p++) {
						if(outsideSentence[p].score < newGuess.score) {
							if (nOutside == max_results)
								nOutside--;
							for (int k = nOutside; k > p; k--)
								outsideSentence[k] = outsideSentence[k-1];
							outsideSentence[p] = newGuess;
							nOutside++;
							insertedSolution = true;
							break;
						}
					}
					// solution doesn't make the cut. if there's room, insert it at the end
					// otherwise, ditch it
					if (!insertedSolution) {
						if (nOutside < max_results)
							outsideSentence[nOutside++] = newGuess;
					}

				}
			}
		}
	}

	// add the new entity case (for ranked model new entity option is always added
	// outside this function)
	if (nResults < max_results && MODEL_TYPE != P1_RANKING) {
		newGuess.guess = NULL;
		newGuess.sentence_num = LinkGuess::NO_SENTENCE;
		// not sure what the proper score here is
		// we'll simply make it slightly less than the next best score
		if (nResults > 0)
			newGuess.score = (results[nResults-1].score - 0.01);
		else
			newGuess.score = 100;
		results[nResults++] = newGuess;

	}

	if (MODEL_TYPE == P1_RANKING && _model_outside_and_within_sentence_links_separately) {
		if (nWithin > 0 && withinSentence[0].guess == NULL) {
			for (int i = 0; i < nOutside; i++) {
				if (nResults < max_results)
					results[nResults++] = outsideSentence[i];
			}
		}
		else {
			for (int i = 0; i < nWithin; i++) {
				if (nResults < max_results)
					results[nResults++] = withinSentence[i];
			}
		}
	}

	// compute confidences for the linking options
	CorefUtils::computeConfidence(results, nResults);

	return nResults;
}

bool DTPronounLinker::_isLinkedBadly(EntitySet *currSolution, Mention* pronMention,
								     Entity* linkedEntity)
{
	const SynNode *pronNode = pronMention->getNode();

	// people should not be linked with non-person pronouns (such as "it")
	if (WordConstants::isNonPersonPronoun(pronNode->getHeadWord()) &&
		linkedEntity->getType() == EntityType::getPERType()) {	
		return true;
	}

	// if a pronoun is linked to an entity it modifies, then it is badly linked
	const SynNode *parentNode = pronNode->getParent();
	if (!parentNode) return false;

	for (int j = 0; j < parentNode->getNChildren(); j++) {
		if (j == parentNode->getHeadIndex()) return false;
		if (parentNode->getChild(j) == pronNode) break;
	}

	const MentionSet *ms = currSolution->getMentionSet(pronMention->getSentenceNumber());
	const Mention *parentMention = ms->getMentionByNode(parentNode);

	if (!parentMention) return false;
	Entity *parentEntity = currSolution->getEntityByMention(parentMention->getUID());
	return parentEntity == linkedEntity;

}

int DTPronounLinker::getHobbsDistance(EntitySet *entitySet, Entity *entity,
					 HobbsDistance::SearchResult *hobbsCandidates, int nHobbsCandidates)
{
	// Make a list of the entity mentions in this entity.
	std::set<MentionUID> entityMentionIds;
	for(int ent_no = 0; ent_no < entity->getNMentions(); ent_no++){
		entityMentionIds.insert(entity->getMention(ent_no));
	}

	for (int i = 0; i < nHobbsCandidates; i++) {
		const MentionSet *mset = entitySet->getMentionSet(hobbsCandidates[i].sentence_number);
		const Mention *ment = mset->getMentionByNode(hobbsCandidates[i].node);	

		if(ment != NULL){
			//if (entitySet->getEntityByMention(ment->getUID()) == entity)
			if (entityMentionIds.find(ment->getUID()) != entityMentionIds.end())
				return i;
		}
		
	}
	return -1;
}

bool DTPronounLinker::_isSpeakerMention(Mention *ment) { 
		return (_docTheory->isSpeakerSentence(ment->getSentenceNumber()));
}
	
bool DTPronounLinker::_isSpeakerEntity(Entity *ent, EntitySet *ents) {
	if (ent == 0)
		return false;
	for (int i=0; i < ent->mentions.length(); i++) {
		Mention* ment = ents->getMention(ent->mentions[i]);
		if (_isSpeakerMention(ment))
			return true;
	}
	return false;
}

void DTPronounLinker::resetForNewDocument(DocTheory *docTheory) { 
	_docTheory = docTheory;
	_debugStream << L"*** NEW DOCUMENT - ";
	_debugStream << docTheory->getDocument()->getName().to_string();
	_debugStream << L" ***\n";
}

void DTPronounLinker::resetForNewSentence() {
	_debugStream << L"*** NEW SENTENCE ***\n";
}


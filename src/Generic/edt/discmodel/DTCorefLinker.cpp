// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/GrowableArray.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/edt/discmodel/DTCorefLinker.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/LexEntity.h"
#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/discmodel/CorefUtils.h"
#include "Generic/theories/EntityType.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/common/version.h"

#include <boost/algorithm/string.hpp>

#include <math.h>
#include <stdio.h>

#include "Generic/CASerif/correctanswers/CorrectMention.h"

DebugStream DTCorefLinker::_debugStream;

DTCorefLinker::DTCorefLinker() : _featureTypes(0), _tagSet(0), _tagScores(0), // JJO 09 Aug 2011 - paramter bool strict (do word inclusion or not)
	_p1Decoder(0), _maxEntDecoder(0), _p1Weights(0), _maxentWeights(0),
	_filter_by_entity_type(true), _filter_by_entity_subtype(false), _filter_by_name_gpe(false)
	,_use_non_ace_entities_as_no_links(false), _max_non_ace_entities(0)
	,_p1_overgen_threshold(0.0), _rank_overgen_threshold(0.0)
	,_noneFeatureTypes(0), _featureTypesArr(0)
{
	/*
	// +++ JJO 09 Aug 2011 +++
	// Word inclusion
	_strict = strict;
	// --- JJO 09 Aug 2011 ---
	*/

	_debugStream.init(Symbol(L"desclink_debug"));

	DTCorefFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	_observation = _new DTCorefObservation();

	std::string buffer;

	// FILTERING BY ENTITY TYPE AND SUBTYPE
	buffer = ParamReader::getParam("do_coref_entity_type_filtering");
	if (buffer == "false")
		_filter_by_entity_type = false;
	_filter_by_entity_subtype = ParamReader::isParamTrue("do_coref_entity_subtype_filtering");

	// FILTERING BY NAME-GPE AFFILIATIONS
	if (ParamReader::isParamTrue("do_coref_name_gpe_filtering")) {
		_filter_by_name_gpe = true;
		DescLinkFeatureFunctions::loadNationalities();
		DescLinkFeatureFunctions::loadNameGPEAffiliations();
	}

	_block_headword_clashes = ParamReader::getOptionalTrueFalseParamWithDefaultVal("block_headword_clashes", false);

	// COREF MODEL TYPE -- MaxEnt or P1
	buffer = ParamReader::getRequiredParam("dt_coref_model_type");
	if (boost::iequals(buffer, "maxent"))
		MODEL_TYPE = MAX_ENT;
	else if (boost::iequals(buffer, "p1"))
		MODEL_TYPE = P1;
	else if (boost::iequals(buffer, "both"))
		MODEL_TYPE = BOTH;
	else if (boost::iequals(buffer, "p1_ranking"))
		MODEL_TYPE = P1_RANKING;
	else 
		throw UnrecoverableException("DTCorefLinker()::DTCorefLinker()",
		"Parameter 'dt_coref_model_type' must be set to 'maxent', 'p1', 'both' or 'p1_ranking'");

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("dt_coref_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("dt_coref_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), DTCorefFeatureType::modeltype);

	// MODEL FILE NAME
	std::string model_file = ParamReader::getRequiredParam("dt_coref_model_file");

	_use_non_ace_entities_as_no_links = ParamReader::isParamTrue("dt_coref_use_non_ACE_ents_as_no_links");

	if (MODEL_TYPE == P1_RANKING) {
		std::string file = model_file + "-rank";
		_p1Weights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_p1Weights, file.c_str(), DTCorefFeatureType::modeltype);

		_noneFeatureTypes = DTCorefFeatureTypes::makeNoneFeatureTypeSet(Mention::DESC);
		DTFeatureTypeSet **_featureTypesArr = _new DTFeatureTypeSet*[_tagSet->getNTags()];
		_featureTypesArr[_tagSet->getNoneTagIndex()] = _noneFeatureTypes;
		_featureTypesArr[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _featureTypes;

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _p1Weights);

		_rank_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_coref_rank_overgen_threshold", 0);		
	}
	else if (MODEL_TYPE == P1 || MODEL_TYPE == BOTH) {
		std::string file = model_file + "-p1";
		_p1Weights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_p1Weights, file.c_str(), DTCorefFeatureType::modeltype);

		double overgen_percentage = ParamReader::getOptionalFloatParamWithDefaultValue("dt_coref_overgen_percentage", 0);	
		if (overgen_percentage < 0.0 || overgen_percentage > 100.0)
			throw UnrecoverableException("DTCorefLinker::DTCorefLinker()",
				"Parameter 'dt_coref_overgen_percentage' must range from 0 to 100");

		_p1_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_coref_overgen_threshold", 0);

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, overgen_percentage);

	} 
	if (MODEL_TYPE == MAX_ENT || MODEL_TYPE == BOTH) {
		std::string file = model_file + "-maxent";
		_maxentWeights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_maxentWeights, file.c_str(), DTCorefFeatureType::modeltype);

		_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);

		_maxent_link_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_coref_maxent_link_threshold", 0.5);
	}
}

DTCorefLinker::~DTCorefLinker() {
	delete _maxentWeights;
	delete _p1Weights;
	delete _p1Decoder;
	delete _maxEntDecoder;
	delete _tagSet;
	delete[] _tagScores;
	delete _featureTypes;
	delete _noneFeatureTypes;
	delete [] _featureTypesArr;
}

void DTCorefLinker::useNonAceEntitiesAsNoLinks() {
//	_use_non_ace_entities_as_no_links = true;
	std::cerr<< "temporarly not initializing corefLinker with using non-ACE entities";
//	_max_non_ace_entities = max_non_ace_entities;
}

void DTCorefLinker::resetForNewSentence() {
	_debugStream << L"*** NEW SENTENCE ***\n";
}

void DTCorefLinker::resetForNewDocument(Symbol docName) {
	_debugStream << L"*** NEW DOCUMENT " << docName.to_string()  <<  " ***\n";
}

// set up to link. This method doesn't do any of the actual processing - it just provides a way
// for multiple paths to get through
int DTCorefLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID,
								 EntityType linkType, LexEntitySet *results[], int max_results)
{
	LexEntitySet *newSet = NULL;
	EntityGuess guesses[64];
	Mention *currMention = currSolution->getMention(currMentionUID);

	if (_debugStream.isActive()) {
		_debugStream << L"\nPROCESSING Mention #" << currMention->getUID() << L": ";
		_debugStream << currMention->getNode()->toTextString() << L"\n";
	}

//	int nGuesses = guessEntity(currSolution, currMention, linkType, guesses, 
//								max_results>64 ? 64: max_results);
	int nGuesses = guessEntity(currSolution, currMention, linkType, guesses, 64);
	if(nGuesses>max_results) nGuesses = max_results;

	for (int i = 0; i < nGuesses; i++) {
		newSet = currSolution->fork();
		if (guesses[i].id == EntityGuess::NEW_ENTITY ||
			!guesses[i].type.isRecognized()) 
		{ // when used with non-ACE candidates for no-link
			if (!guesses[i].type.isRecognized()) {
				if (_debugStream.isActive()) 
					_debugStream << L"OUTCOME: NOT CREATING A NEW ENTITY BECAUSE OF LINKING TO NON-ACE ENTITY #"<<guesses[i].id<<"\n";
			}
			else {
				newSet->addNew(currMention->getUID(), linkType);

				/*
				// +++ JJO 08 Aug 2011 +++
				// Word inclusion
				if (_strict) {
					const SynNode *currNode = currMention->getNode();
					int currNWords = currMention->getNWords();
					Symbol *currWords = _new Symbol[512];
					currNode->getTerminalSymbols(currWords, currNWords);
					Entity *entity = newSet->getEntityByMention(currMention->getUID());
					entity->addWords(currWords, currNWords);
					delete [] currWords;
				}
				// --- JJO 08 Aug 2011 ---
				*/

				if (_debugStream.isActive())
					_debugStream << L"OUTCOME: CREATED NEW ENTITY #" << newSet->getNEntities() - 1 << "\n";
			}
			Mention *newMention = newSet->getMention(currMention->getUID());
			newMention->setLinkConfidence(guesses[i].linkConfidence);
		}
		else {
			newSet->add(currMention->getUID(), guesses[i].id);
			
			/*
			// +++ JJO 08 Aug 2011 +++
			// Word inclusion (same as above)
			if (_strict) {
				const SynNode *currNode = currMention->getNode();
				int currNWords = currMention->getNWords();
				Symbol *currWords = _new Symbol[512];
				currNode->getTerminalSymbols(currWords, currNWords);
				Entity *entity = newSet->getEntityByMention(currMention->getUID());
				entity->addWords(currWords, currNWords);
				delete [] currWords;
			}
			// --- JJO 08 Aug 2011 ---
			*/
			
			Mention *newMention = newSet->getMention(currMention->getUID());
			if(!_filter_by_entity_type && newMention->getEntityType()!=guesses[i].type) {
				newMention->setEntityType(guesses[i].type);
				newMention->setEntitySubtype(EntitySubtype::getDefaultSubtype(guesses[i].type));
			}
			newMention->setLinkConfidence(guesses[i].linkConfidence);
			if (_debugStream.isActive())
				_debugStream << L"OUTCOME: LINKED TO ENTITY #"  << guesses[i].id << " score: "<< guesses[i].score <<"\n";
		}
		newSet->setScore(newSet->getScore() + static_cast<float>(guesses[i].score));
		results[i] = newSet;
		newSet->customDebugPrint(_debugStream);
	}
	if (_debugStream.isActive())
		_debugStream << L"\nDONE PROCESSING MENTION #" << currMention->getUID() << "\n\n";

	return nGuesses;
}

// get probability for linking to each entity. If no entity has > 50% chance, put link to
// new entity at top of heap.
int DTCorefLinker::guessEntity(LexEntitySet * currSolution, Mention * currMention,
								EntityType linkType, EntityGuess results[], int max_results)
{
	int link_index = _tagSet->getLinkTagIndex();
	int no_link_index = _tagSet->getNoneTagIndex();
	int nResults = 0;

	if (_debugStream.isActive())
		_debugStream << L"BEGIN_GUESSES... mention type: "<<linkType.getName().to_string()<<L"\n";
	// this is a bit of a hack to get both of these, but because there is no
	// assignment operation on GrowableArrays, we'll just do it this way
	const GrowableArray <Entity *> &entitiesByType = currSolution->getEntitiesByType(linkType);
	const GrowableArray <Entity *> &allEntities = currSolution->getEntities();
	const GrowableArray <Entity *>  &ents = 
		(_filter_by_entity_type || _filter_by_entity_subtype) ? entitiesByType : allEntities;

	GrowableArray <Entity *> filteredEnts;
	for (int c = 0; c < ents.length(); c++) {
		if (!DescLinkFeatureFunctions::globalGPEAffiliationClash(_docTheory, currSolution, ents[c], currMention)) { // Use Name-GPE affiliation lists to check whether there's a clash (AB)
			if (_filter_by_entity_subtype) {
				if (CorefUtils::subtypeMatch(currSolution, ents[c], currMention))
					filteredEnts.add(ents[c]);
			}
			else if (_filter_by_entity_type)
				filteredEnts.add(ents[c]);
			else 
				if(!_use_non_ace_entities_as_no_links || ents[c]->getType().isRecognized()) // only ace entities are added
					filteredEnts.add(ents[c]);
		}
	}

	// add some non-ACE entities
	if(_use_non_ace_entities_as_no_links)
		CorefUtils::addNearestEntities(filteredEnts, currSolution , currMention, _max_non_ace_entities);

	//when approximating document level coreference, you cant use getLastMentionSet()
	//_observation->resetForNewSentence(currSolution->getLastMentionSet(), currSolution);
	int sent_num = currMention->getSentenceNumber();
	const MentionSet* mentionSet = currSolution->getMentionSet(sent_num);
	//TODO: add mentionMapping information + move to Document level
	_observation->resetForNewDocument(currSolution);
	_observation->resetForNewSentence(mentionSet);

	double thisscore;


	// compute the no_link score
	// add the new entity case for P1_RANKING
	if (MODEL_TYPE == P1_RANKING){

		EntityGuess newGuess;
		newGuess.id = EntityGuess::NEW_ENTITY;
		newGuess.type = linkType;
		static_cast<DTNoneCorefObservation*>(_observation)->populate(currMention->getUID());
		//having a positive overgen parameter increases the chances for linking
		newGuess.score = _p1Decoder->getScore(_observation, no_link_index) - _rank_overgen_threshold;

		if (_debugStream.isActive()) {
			_debugStream << L"CONSIDERING NO-LINK option\n";
			_p1Decoder->printDebugInfo(_observation, no_link_index, _debugStream);
			_debugStream << L"NO-LINK SCORE " << newGuess.score << L"\n\n";
		}
		results[nResults++] = newGuess;
	}

	for (int i = 0; i < filteredEnts.length(); i++) {
		//document level coreference leaves empty entities behind these cause crashes...
		if (filteredEnts[i]->getNMentions() == 0) {
			continue;
		}

		// Don't do descriptor linking for simple coref instances; too risky
		// Note that appositives and copulas and whatnot are linked elsewhere; they will
		//   still get through
		if (currMention->getEntityType().useSimpleCoref()) {
			continue;
		}

		/*
		// +++ JJO 08 Aug 2011 +++		
		// Word inclusion
		if (_strict) {
			const SynNode *currNode = currMention->getNode();
			int currNWords = currMention->getNWords();
			Symbol *currWords = _new Symbol[512];
			currNode->getTerminalSymbols(currWords, currNWords);

			bool reject = false;
			Entity *entity = filteredEnts[i];
			for (int j = 0; j < currNWords && j < 512; j++) {
				if (!entity->hasWord(currWords[j]))
					reject = true;
			}
			delete [] currWords;
			if (reject) {
				_debugStream << L"Rejected (word inclusion failed)";
				continue;
			}
		}
		// --- JJO 08 Aug 2011 ---
		*/

		const EntityType &entType = filteredEnts[i]->getType();
		Symbol linkval = _tagSet->getNoneTag();

		_observation->populate(currMention->getUID(), filteredEnts[i]->getID());

		if (_debugStream.isActive()) {
			_debugStream << L"-------------------------------\n";
//			_debugStream << L"Mention: " << currMention->getNode()->toTextString() << L"\n";
			_debugStream << L"CONSIDERING LINK TO ";
			if(_use_non_ace_entities_as_no_links && !entType.isRecognized())
				_debugStream << L"Non-ACE ";
			_debugStream << L"Entity #" << filteredEnts[i]->getID() << L": [";
			for (int m = 0; m < filteredEnts[i]->getNMentions(); m++)
				_debugStream << currSolution->getMention(filteredEnts[i]->getMention(m))->getNode()->toTextString() << L" ,";
			_debugStream << L"]\n";
		}

		// This is evil and we should block it, not rely on our model to get rid of this
		// This kills things like "Chilean leader" ~ "Venezuelan president" and "Iraqi forces" ~ "British forces"
		if (DescLinkFeatureFunctions::localGPEAffiliationClash(_observation->getMention(), _observation->getEntity(), _observation->getEntitySet(), _docTheory)) {
			if (_debugStream.isActive())
				_debugStream << L"Evil mismatch named modifier; skipping as candidate\n";
			continue;
		}		
		if (_block_headword_clashes &&
			DescLinkFeatureFunctions::hasHeadwordClash(_observation->getMention(), _observation->getEntity(), _observation->getEntitySet(), _docTheory)) 
		{
			if (_debugStream.isActive())
				_debugStream << L"Headword clash; skipping as candidate\n";
			continue;
		}

		if (MODEL_TYPE == MAX_ENT || MODEL_TYPE == BOTH) {
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
		
		if (linkval == _tagSet->getNoneTag() && (MODEL_TYPE == P1 || MODEL_TYPE == BOTH)) {
			// Compute the whether to link or not and get the score
			linkval = _p1Decoder->decodeToSymbol(_observation, thisscore);
			// If no-link, we can still decide to link if the score is less then overgen_threshold
			if (linkval == _tagSet->getNoneTag() && thisscore < _p1_overgen_threshold) {
				linkval = _tagSet->getTagSymbol(link_index);
				// I'm not sure what the appropriate behavior is here when thisscore == 0
				// Do we even do anything with these scores?
				if (thisscore != 0)
					thisscore = 1/thisscore;
				else thisscore = 0;
			}
		}

		if (MODEL_TYPE == P1_RANKING) {
			linkval = _tagSet->getTagSymbol(link_index);
			thisscore = _p1Decoder->getScore(_observation, link_index);
		}

		if (_debugStream.isActive()) {
			if (_p1Decoder != 0)
				_p1Decoder->printDebugInfo(_observation, link_index, _debugStream);
			if (_maxEntDecoder != 0)
				_maxEntDecoder->printDebugInfo(_observation, link_index, _debugStream);
			_debugStream << L"LINK SCORE " << thisscore << L" := ";
			_debugStream << linkval.to_debug_string();
			_debugStream << L"\n\n";
		}

		if (linkval != _tagSet->getNoneTag()) {
			EntityGuess newGuess;;
			newGuess.id = filteredEnts[i]->getID();
			newGuess.type = entType;
			newGuess.score = thisscore;
			// add results in sorted order
			bool insertedSolution = false;
			for (int p = 0; p < nResults; p++) {
				if(results[p].score < newGuess.score) {
					if (nResults < max_results)
						nResults++;
					for (int k = nResults-1; k > p; k--)
						results[k] = results[k-1];
					results[p] = newGuess;
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
		}

	}
	// add the new entity case
	if (nResults < max_results && MODEL_TYPE != P1_RANKING) {
		EntityGuess newGuess;
		newGuess.id = EntityGuess::NEW_ENTITY;
		newGuess.type = linkType;
		// not sure what the proper score here is
		// we'll simply make it slightly less than the next best score
		if (nResults > 0)
			newGuess.score = (results[nResults-1].score - 0.01);
		else
			newGuess.score = 100;
		results[nResults++] = newGuess;

	}

	// compute confidences for the linking options
	CorefUtils::computeConfidence(results, nResults);

	return nResults;
}


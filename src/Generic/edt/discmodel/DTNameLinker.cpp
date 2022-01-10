// Copyright 2008 by BBN Technologies Corp.
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
#include "Generic/edt/discmodel/DTNameLinker.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/LexEntity.h"
#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/discmodel/CorefUtils.h"
#include "Generic/theories/EntityType.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/edt/SimpleRuleNameLinker.h"

#include <boost/algorithm/string.hpp>

#include <math.h>
#include <stdio.h>

#include "Generic/CASerif/correctanswers/CorrectMention.h"

DebugStream DTNameLinker::_debugStream;

DTNameLinker::DTNameLinker(DocumentMentionInformationMapper *infoMap) : 
	_featureTypes(0), _tagSet(0), _tagScores(0)
	, _p1Decoder(0), _maxEntDecoder(0), _p1Weights(0), _maxentWeights(0)
	, _filter_by_entity_type(true), _filter_by_entity_subtype(false)
	, _p1_overgen_threshold(0.0), _rank_overgen_threshold(0.0)
	, _limitToNames(false), _noneFeatureTypes(0), _featureTypesArr(0)
{
	_debugStream.init(Symbol(L"namelink_debug"));

	DTCorefFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	_observation = _new DTCorefObservation(infoMap);

	// FILTERING BY ENTITY TYPE AND SUBTYPE
	std::string buffer = ParamReader::getParam("do_name_coref_entity_type_filtering");
	if (buffer == "false")
		_filter_by_entity_type = false;

	_filter_by_entity_subtype = ParamReader::isParamTrue("do_name_coref_entity_subtype_filtering");

	// COREF MODEL TYPE -- MaxEnt or P1
	buffer = ParamReader::getRequiredParam("dt_name_coref_model_type");
	if (boost::iequals(buffer, "maxent"))
		MODEL_TYPE = MAX_ENT;
	else if (boost::iequals(buffer, "p1"))
		MODEL_TYPE = P1;
	else if (boost::iequals(buffer, "both"))
		MODEL_TYPE = BOTH;
	else if (boost::iequals(buffer, "p1_ranking"))
		MODEL_TYPE = P1_RANKING;
	else 
		throw UnrecoverableException("DTNameLinker()::DTNameLinker()",
		"Parameter 'dt_name_coref_model_type' must be set to 'maxent', 'p1', 'both' or 'p1_ranking'");

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("dt_name_coref_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("dt_name_coref_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), DTCorefFeatureType::modeltype);

	// MODEL FILE NAME
	std::string model_file = ParamReader::getRequiredParam("dt_name_coref_model_file");

	// LIMIT LINKING JUST TO NAMES
	_limitToNames = ParamReader::isParamTrue("dt_name_coref_link_only_to_names");

	if (MODEL_TYPE == P1_RANKING) {
		std::string file = model_file + "-rank";
		_p1Weights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_p1Weights, file.c_str(), DTCorefFeatureType::modeltype);

		_noneFeatureTypes = DTCorefFeatureTypes::makeNoneFeatureTypeSet(Mention::NAME);
		_featureTypesArr = _new DTFeatureTypeSet*[_tagSet->getNTags()];
		_featureTypesArr[_tagSet->getNoneTagIndex()] = _noneFeatureTypes;
		_featureTypesArr[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _featureTypes;

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _p1Weights);

		_rank_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_name_coref_rank_overgen_threshold", 0);
		
	}
	else if (MODEL_TYPE == P1 || MODEL_TYPE == BOTH) {
		std::string file = model_file + "-p1";
		_p1Weights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_p1Weights, file.c_str(), DTCorefFeatureType::modeltype);

		double overgen_percentage = ParamReader::getOptionalFloatParamWithDefaultValue("dt_name_coref_overgen_percentage", 0);
		if (overgen_percentage < 0.0 || overgen_percentage > 100.0)
			throw UnrecoverableException("DTNameLinker::DTNameLinker()",
			"Parameter 'dt_name_coref_overgen_percentage' must range from 0 to 100");

		_p1_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_name_coref_overgen_threshold", 0);

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, overgen_percentage);

	} 
	if (MODEL_TYPE == MAX_ENT || MODEL_TYPE == BOTH) {
		std::string file = model_file + "-maxent";
		_maxentWeights = _new DTFeature::FeatureWeightMap(500009);
		DTFeature::readWeights(*_maxentWeights, file.c_str(), DTCorefFeatureType::modeltype);

		_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);
		_maxent_link_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_name_coref_maxent_link_threshold", 0.5);
	}
	_use_rules = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_simple_rule_namelink", false);
	if (_use_rules){
		_simpleRuleNameLinker = _new SimpleRuleNameLinker();
		_distillation_mode = ParamReader::getOptionalTrueFalseParamWithDefaultVal("simple_rule_name_link_distillation", false);
		if(!_distillation_mode)
			throw UnexpectedInputException("DTNameLinker::DTNameLinker()", 
			"If DTNameLinker used and 'use_simple_rule_namelink: true', 'simple_rule_name_link_distillation' must also be true");
	}
}

DTNameLinker::~DTNameLinker() {
	delete _maxentWeights;
	delete _p1Weights;
	delete _p1Decoder;
	delete _maxEntDecoder;
	delete _tagSet;
	delete[] _tagScores;
	delete _featureTypes;
	delete _noneFeatureTypes;
	delete [] _featureTypesArr;
	if (_use_rules)
		delete _simpleRuleNameLinker;
}


void DTNameLinker::resetForNewSentence() {
	_debugStream << L"*** NEW SENTENCE ***\n";
}

void DTNameLinker::resetForNewDocument(Symbol docName) {
	_debugStream << L"*** NEW DOCUMENT " << docName.to_string()  <<  " ***\n";
}

// set up to link. This method doesn't do any of the actual processing - it just provides a way
// for multiple paths to get through
int DTNameLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID,
								 EntityType linkType, LexEntitySet *results[], 
								 int max_results)
{
	LexEntitySet *newSet = NULL;
	EntityGuess guesses[64];
	Mention *currMention = currSolution->getMention(currMentionUID);
	if (_use_rules)
		_simpleRuleNameLinker->setCurrSolution(currSolution);

	_debugStream << L"\nPROCESSING Mention #" << currMention->getUID() << L": ";
	_debugStream << currMention->getNode()->toTextString() << L"\n";

	int nGuesses = guessEntity(currSolution, currMention, linkType, guesses, 
								max_results>64 ? 64: max_results);

	for (int i = 0; i < nGuesses; i++) {
		newSet = currSolution->fork();
		if (guesses[i].id == EntityGuess::NEW_ENTITY ||
			!guesses[i].type.isRecognized()) { // when used with non-ACE candidates for no-link
			if (!guesses[i].type.isRecognized())
				_debugStream << L"OUTCOME: NOT CREATING A NEW ENTITY BECAUSE OF LINKING TO NON-ACE ENTITY #"<<guesses[i].id<<"\n";
			else {
				newSet->addNew(currMention->getUID(), linkType);
				_debugStream << L"OUTCOME: CREATED NEW ENTITY #" << newSet->getNEntities() - 1 << "\n";
			}
		}
		else {
			newSet->add(currMention->getUID(), guesses[i].id);
			Mention *newMention = newSet->getMention(currMention->getUID());
			if(!_filter_by_entity_type && newMention->getEntityType()!=guesses[i].type) {
				newMention->setEntityType(guesses[i].type);
				newMention->setEntitySubtype(EntitySubtype::getDefaultSubtype(guesses[i].type));
			}
			_debugStream << L"OUTCOME: LINKED TO ENTITY #"  << guesses[i].id << " score: "<< guesses[i].score <<"\n";
		}
		newSet->setScore(newSet->getScore() + static_cast<float>(guesses[i].score));
		results[i] = newSet;
		newSet->customDebugPrint(_debugStream);
	}
	_debugStream << L"\nDONE PROCESSING MENTION #" << currMention->getUID() << "\n\n";

	return nGuesses;
}

// get probability for linking to each entity. If no entity has > 50% chance, put link to
// new entity at top of heap.
int DTNameLinker::guessEntity(LexEntitySet * currSolution, Mention * currMention,
								EntityType linkType, EntityGuess results[], int max_results)
{
	int link_index = _tagSet->getLinkTagIndex();
	int no_link_index = _tagSet->getNoneTagIndex();
	int nResults = 0;

	_debugStream << L"BEGIN_GUESSES... mention type: "<<linkType.getName().to_string()<<L"\n";
	// this is a bit of a hack to get both of these, but because there is no
	// assignment operation on GrowableArrays, we'll just do it this way
	const GrowableArray <Entity *> &entitiesByType = currSolution->getEntitiesByType(linkType);
	const GrowableArray <Entity *> &allEntities = currSolution->getEntities();
	const GrowableArray <Entity *>  &ents = 
		(_filter_by_entity_type || _filter_by_entity_subtype) ? entitiesByType : allEntities;

	GrowableArray <Entity *> filteredEnts;
	for (int c = 0; c < ents.length(); c++) {
		Entity *entity = ents[c];
		if (_limitToNames && (CorefUtils::getEntityMentionLevel(currSolution,entity)!=Mention::NAME)) 
			continue;

		if (_filter_by_entity_subtype) {
			if (CorefUtils::subtypeMatch(currSolution, entity, currMention))
				filteredEnts.add(entity);
		}
		else if (_filter_by_entity_type)
			filteredEnts.add(entity);
		else 
			if(ents[c]->getType().isRecognized()) // only ace entities are added
				filteredEnts.add(entity);
	}

	//when approximating document level coreference, you cant use getLastMentionSet()
	//_observation->resetForNewSentence(currSolution->getLastMentionSet(), currSolution);
	int sent_num = currMention->getSentenceNumber();
	const MentionSet* mentionSet = currSolution->getMentionSet(sent_num);
	//TODO: add mentionMapping information + move to Document level
	// this is problematic to do because of the beam.
	_observation->resetForNewDocument(currSolution);
	_observation->resetForNewSentence(mentionSet);

	
	// compute the no_link score
	// add the new entity case for P1_RANKING
	if (MODEL_TYPE == P1_RANKING && !currMention->getEntityType().useSimpleCoref()){

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
		const EntityType &entType = filteredEnts[i]->getType();
		Symbol linkval = _tagSet->getNoneTag();
		double thisscore = 0.0f;

		if (currMention->getEntityType().useSimpleCoref()) {
			// Only match within a given type, regardless of settings above
			if (filteredEnts[i]->getType() != currMention->getEntityType())
				continue;
			std::wstring name_string = currMention->getAtomicHead()->toTextString();
			for (int m = 0; m < filteredEnts[i]->getNMentions(); m++) {
				const Mention *ment = currSolution->getMention(filteredEnts[i]->getMention(m));
				if (ment->getMentionType() == Mention::NAME) {
					std::wstring candidate_name_string = ment->getAtomicHead()->toTextString();
					if (name_string == candidate_name_string) {
						linkval = _tagSet->getTagSymbol(link_index);
						thisscore = 1.0f;
						break;
					}
				}
			}
		} else {

			if (_use_rules) {
				bool okToLink = false;
				bool metonymyCase = false;
				EntityGuess* guessForRules = _new EntityGuess();
				guessForRules->id = filteredEnts[i]->getID();
				guessForRules->type = filteredEnts[i]->getType();
				if (_simpleRuleNameLinker->isOKToLink(currMention, linkType, guessForRules)) {
					okToLink = true;
				}
				if(_distillation_mode ){
					if( _simpleRuleNameLinker->isMetonymyLinkCase(currMention, linkType, guessForRules)){
						metonymyCase = true;
					}
					//DT Name Linker might link capitals to countries even when isMetonymyLinkCase() is false.
					//Since, we really want to avoid this behavior, block all country --> city links
					else if( _simpleRuleNameLinker->isCityAndCountry(currMention, linkType, guessForRules)){
						metonymyCase = true;
					}
				}
				delete guessForRules;
				if(!okToLink || metonymyCase){
					if (_debugStream.isActive()) {
						_debugStream << L"-------------------------------\n";
						//			_debugStream << L"Mention: " << currMention->getNode()->toTextString() << L"\n";
						_debugStream << L"SKIP LINK (from rules) TO ";
						_debugStream << L"Entity #" << filteredEnts[i]->getID() << L": [";
						for (int m = 0; m < filteredEnts[i]->getNMentions(); m++)
							_debugStream << currSolution->getMention(filteredEnts[i]->getMention(m))->getNode()->toTextString() << L" ,";
						_debugStream << L"]\n";
						_debugStream << L"okToLink: "<<okToLink<<", metonymyCase: "<<metonymyCase<<L"\n";
					}
					continue;
				}
			}
			_observation->populate(currMention->getUID(), filteredEnts[i]->getID());
			if (_debugStream.isActive()) {
				_debugStream << L"-------------------------------\n";
				//			_debugStream << L"Mention: " << currMention->getNode()->toTextString() << L"\n";
				_debugStream << L"CONSIDERING LINK TO ";
				_debugStream << L"Entity #" << filteredEnts[i]->getID() << L": [";
				for (int m = 0; m < filteredEnts[i]->getNMentions(); m++)
					_debugStream << currSolution->getMention(filteredEnts[i]->getMention(m))->getNode()->toTextString() << L" ,";
				_debugStream << L"]\n";
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
				linkval = _p1Decoder->decodeToSymbol(_observation, thisscore);
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
				if (_p1Decoder != 0) _p1Decoder->printDebugInfo(_observation, link_index, _debugStream);
				_debugStream << L"LINK SCORE " << thisscore << L" := ";
				_debugStream << linkval.to_debug_string();
				_debugStream << L"\n\n";
			}
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
	if (nResults < max_results && (MODEL_TYPE != P1_RANKING || currMention->getEntityType().useSimpleCoref())) {
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
	return nResults;
}

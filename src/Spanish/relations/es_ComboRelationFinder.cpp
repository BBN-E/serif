// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/relations/es_ComboRelationFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Spanish/relations/es_SpecialRelationCases.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"

#include "Generic/common/ParamReader.h"
#include "Spanish/common/es_WordConstants.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/wordnet/xx_WordNet.h"

#include "Generic/discTagger/P1Decoder.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Spanish/relations/discmodel/es_P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/VectorModel.h"
#include "Spanish/relations/es_TreeModel.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

UTF8OutputStream SpanishComboRelationFinder::_debugStream;
bool SpanishComboRelationFinder::DEBUG = false;

Symbol SpanishComboRelationFinder::NO_RELATION_SYM = Symbol(L"NO_RELATION");

SpanishComboRelationFinder::SpanishComboRelationFinder() : _allow_mention_set_changes(false), _skip_special_case_answer(false) {

	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");

	SpanishP1RelationFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	std::string debug_buffer = ParamReader::getParam("relation_debug");
	if (!debug_buffer.empty()) {
		_debugStream.open(debug_buffer.c_str());
		DEBUG = true;
	}

	std::string tag_set_file = ParamReader::getRequiredParam("relation_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);

	/*std::string maxent_tag_set_file = ParamReader::getParam("maxent_relation_tag_set_file");
	if (!maxent_tag_set_file.empty())
		_maxentTagSet = _new DTTagSet(maxent_tag_set_file.c_str(), false, false);
	else
		_maxentTagSet = _tagSet;
	_maxentTagScores = _new double[_maxentTagSet->getNTags()];*/

	std::string features_file = ParamReader::getRequiredParam("relation_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1RelationFeatureType::modeltype);

	std::string p1_model_file = ParamReader::getParam("p1_relation_model_file");
	if (!p1_model_file.empty()) {
        // OVERGENERATION PERCENTAGE
		double overgen_percentage = ParamReader::getRequiredFloatParam("p1_relation_overgen_percentage");

		_p1Weights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_p1Weights, p1_model_file.c_str(), P1RelationFeatureType::modeltype);

		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, overgen_percentage);
		if (overgen_percentage == 0 && ParamReader::hasParam("p1_relation_underges_percentage")) {
			double under = ParamReader::getOptionalFloatParamWithDefaultValue("p1_relation_underges_percentage", 0);
			_p1Decoder->setUndergenPercentage(under);
		}
	} else {
		_p1Decoder = 0;
		_p1Weights = 0;
	}

	std::string maxent_model_file = ParamReader::getParam("maxent_relation_model_file");	
	if (!maxent_model_file.empty()) {
		_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_maxentWeights, maxent_model_file.c_str(), P1RelationFeatureType::modeltype);

		_maxentDecoder = _new MaxEntModel(_maxentTagSet, _featureTypes, _maxentWeights);
	} else {
		_maxentDecoder = 0;
		_maxentWeights = 0;
	}

	

	std::string secondary_model_file = ParamReader::getParam("secondary_p1_relation_model_file");		
	if (!secondary_model_file.empty()) {
		_p1SecondaryWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_p1SecondaryWeights, secondary_model_file.c_str(), P1RelationFeatureType::modeltype);

		std::string secondary_features_files = ParamReader::getRequiredParam("secondary_p1_relation_features_file");
		_p1SecondaryFeatureTypes = _new DTFeatureTypeSet(secondary_features_files.c_str(), P1RelationFeatureType::modeltype);

		_p1SecondaryDecoder = _new P1Decoder(_tagSet, _p1SecondaryFeatureTypes, _p1SecondaryWeights, 0, true);
	} else {
		_p1SecondaryDecoder = 0;
		_p1SecondaryFeatureTypes = 0;
		_p1SecondaryWeights = 0;
	}

	//mrf - The relation validation string is used to determine which 
	//relation-type/argument types the decoder allows.  RelationObservation calls 
	//the language specific RelationUtilities::get()->isValidRelationEntityTypeCombo(). 
	std::string validation_str = ParamReader::getRequiredParam("relation_validation_str");
	_observation = _new RelationObservation(validation_str.c_str(), _featureTypes);

	_inst = _new PotentialRelationInstance();

	/*std::string vector_model_file = ParamReader::getRequiredParam("vector_tree_relation_model_file");
	RelationTypeSet::initialize();
	_vectorModel = _new VectorModel(vector_model_file.c_str());
	_treeModel = _new TreeModel(vector_model_file.c_str());

	_facOrgWords = _new SymbolHash(100);
	std::string parameter = ParamReader::getParam("ambiguous_fac_org_words");
	if (!parameter.empty()) {
		loadSymbolHash(_facOrgWords, parameter.c_str());
	}*/

	_skip_special_case_answer = ParamReader::getOptionalTrueFalseParamWithDefaultVal("relation_skip_special_case_answer", false);

}

SpanishComboRelationFinder::~SpanishComboRelationFinder() {
	delete _observation;
	delete _p1Decoder;
	delete _p1SecondaryDecoder;
	delete _maxentDecoder;
	//delete _facOrgWords;
	//delete _vectorModel;
	//delete _treeModel;
	delete _p1Weights;
	delete _p1SecondaryWeights;
	delete _maxentWeights;
	delete _featureTypes;
	delete _p1SecondaryFeatureTypes;
	//if (_maxentTagSet != _tagSet)
	//	delete _maxentTagSet;
	delete _tagSet;
	delete _inst;
	//delete[] _maxentTagScores;
}

void SpanishComboRelationFinder::cleanup() {
	delete _observation;
	_observation = 0;
}

void SpanishComboRelationFinder::resetForNewSentence() {
	_n_relations = 0;
}

// File-local helper function:
namespace {
	/** Generate a description of a mention, for debugging purposes. */
	void debugDescribeMention(std::wstring label, const Mention *mention, SentenceTheory *sentTheory, UTF8OutputStream &out) {
		out << label << "\n";
		if (sentTheory) {
			out << "  Character Span: ["
				<< sentTheory->getTokenSequence()->getToken(mention->getNode()->getStartToken())->getStartEDTOffset() << L":"
				<< sentTheory->getTokenSequence()->getToken(mention->getNode()->getEndToken())->getEndEDTOffset()  << L"]\n";
			out << "  DocID: " << sentTheory->getDocID() << L"\n";
		}
		out << L"  Text: " << mention->getNode()->toTextString() << L"\n";
		out << L"  Mention Type: " << mention->getTypeString(mention->getMentionType()) << L"\n";
		out << L"  Entity Type: " << mention->getEntityType().getName().to_string() << L"\n";
		out << L"  Entity Subtype: " << mention->getEntitySubtype().getName().to_string() << L"\n";
	}
}

RelMentionSet *SpanishComboRelationFinder::getRelationTheory(EntitySet *entitySet,
													  SentenceTheory *sentTheory,
											   const Parse *parse,
											   MentionSet *mentionSet,
											   ValueMentionSet *valueMentionSet,
											   PropositionSet *propSet,
											   const PropTreeLinks* ptLinks)
{

	if (mentionSet == 0) {
		throw InternalInconsistencyException("SpanishComboRelationFinder::getRelationTheory()",
											"MentionSet is null.");
	}

	_n_relations = 0;
	_currentSentenceIndex = mentionSet->getSentenceNumber();

	propSet->fillDefinitionsArray();

	if (_observation == 0) {
		std::string validation_str = ParamReader::getRequiredParam("relation_validation_str");
		_observation = _new RelationObservation(validation_str.c_str());
	}
	_observation->resetForNewSentence(entitySet, parse, mentionSet, valueMentionSet, propSet, 0, 0, ptLinks);

	int nmentions = mentionSet->getNMentions();

	for (int i = 0; i < nmentions; i++) {
		const Mention* mention1 = mentionSet->getMention(i);
		if (mention1->getMentionType() == Mention::NONE ||
			mention1->getMentionType() == Mention::APPO ||
			mention1->getMentionType() == Mention::LIST)
			continue;
		for (int j = i + 1; j < nmentions; j++) {
			const Mention* mention2 = mentionSet->getMention(j);
			if (mention2->getMentionType() == Mention::NONE ||
				mention2->getMentionType() == Mention::APPO ||
				mention2->getMentionType() == Mention::LIST)
				continue;
			_observation->populate(i, j);

			// this means we're doing sentence-level relation finding
			// currently the only implemented use of this is to find mention set changes
			if (_allow_mention_set_changes) {
				findMentionSetChanges(mentionSet);
				continue;
			}			
			if (!mention1->isOfRecognizedType() ||
				!mention2->isOfRecognizedType())
			{
				continue;
			}

			if (DEBUG) {
				debugDescribeMention(L"LHS", mention1, sentTheory, _debugStream);
				debugDescribeMention(L"RHS", mention2, sentTheory, _debugStream);
				_debugStream.flush();
			}

			Symbol p1Answer = _tagSet->getNoneTag();
			Symbol maxentAnswer = _tagSet->getNoneTag();//_maxentTagSet->getNoneTag();
			Symbol confidentVectorAnswer = _tagSet->getNoneTag();
			Symbol vectorAnswer = _tagSet->getNoneTag();
			Symbol treeAnswer = _tagSet->getNoneTag();
			Symbol secondaryAnswer = _tagSet->getNoneTag();

			_inst->setStandardInstance(_observation);
			Symbol specialCaseAnswer = findSpecialCaseRelation(mentionSet);
			if (_tagSet->getTagIndex(specialCaseAnswer) == -1)
				specialCaseAnswer = _tagSet->getNoneTag();

			if (specialCaseAnswer == NO_RELATION_SYM)
				continue;


			RelationPropLink *link = _observation->getPropLink();
			if (!link->isEmpty() && !link->isNegative()) {
				//confidentVectorAnswer
				//	= RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst, .3));
				if (_maxentDecoder != 0) {  // if _maxentDecoder is not defined, p1 is always used, so there is no need for vectorAnswer and treeAnswer.
					vectorAnswer
						= RelationTypeSet::getRelationSymbol(_vectorModel->findBestRelationType(_inst));
					treeAnswer
						= RelationTypeSet::getRelationSymbol(_treeModel->findBestRelationType(_inst));
				}
			}

			if (_p1Decoder)
				p1Answer = _p1Decoder->decodeToSymbol(_observation);

			double maxent_score = 0;
			if (_maxentDecoder) {
				maxentAnswer = _maxentDecoder->decodeToSymbol(_observation, maxent_score, true);
			}

			if (_p1SecondaryDecoder) {
				secondaryAnswer = _p1SecondaryDecoder->decodeToSymbol(_observation);
				if (secondaryAnswer != _tagSet->getNoneTag()) {
					double tagscore = _p1SecondaryDecoder->getScore(_observation, _tagSet->getTagIndex(secondaryAnswer));
					double nonescore = _p1SecondaryDecoder->getScore(_observation, _tagSet->getNoneTagIndex());
					double diff = tagscore - nonescore;
					if (diff < 1500)
						secondaryAnswer = _tagSet->getNoneTag();
				}
			}


			if (DEBUG) {
				_debugStream << L"VECTOR: " << vectorAnswer.to_debug_string() << "\n";
				_debugStream << L"CONFIDENT VECTOR: " << confidentVectorAnswer.to_debug_string() << "\n";
				_debugStream << L"TREE: " << treeAnswer.to_debug_string() << "\n";
				if (_p1Decoder) {
					_debugStream << L"P1: " << p1Answer.to_debug_string() << "\n";
					if (p1Answer != _tagSet->getNoneTag())
						_p1Decoder->printDebugInfo(_observation, _tagSet->getNoneTagIndex(), _debugStream);
					_p1Decoder->printDebugInfo(_observation, _tagSet->getTagIndex(p1Answer), _debugStream);
				}
				if (_maxentDecoder) {
					_debugStream << L"MAXENT: " << maxentAnswer.to_debug_string() << "\n";
					//if (maxentAnswer != _maxentTagSet->getNoneTag())
					//	_maxentDecoder->printDebugInfo(_observation, _maxentTagSet->getNoneTagIndex(), _debugStream);
					//_maxentDecoder->printDebugInfo(_observation, _maxentTagSet->getTagIndex(maxentAnswer), _debugStream);
					_maxentDecoder->printDebugTable(_observation, _debugStream);
					_debugStream << "MAXENT SCORE: " << maxent_score << "\n";
				}
				if (_p1SecondaryDecoder) {
					_debugStream << L"SECONDARY P1: " << secondaryAnswer.to_debug_string() << "\n";							
				}
			}

			// combine these in some clever way
			Symbol answer;
			bool heuristic_fired = false;
			if (_skip_special_case_answer)
				answer = _tagSet->getNoneTag();
			else {
				answer = specialCaseAnswer;
				heuristic_fired = true;
			}

			if (_use_correct_answers) {
				if (answer == _tagSet->getNoneTag())
					answer = p1Answer;			
				if (answer == _tagSet->getNoneTag() && _maxentTagSet == _tagSet)
					answer = maxentAnswer;			
				if (answer == _tagSet->getNoneTag())
					answer = confidentVectorAnswer;
				if (answer == _tagSet->getNoneTag())
					answer = vectorAnswer;
			} else {
				if (_maxentDecoder == 0 || (answer == _tagSet->getNoneTag() && maxentAnswer != _maxentTagSet->getNoneTag())) {
					answer = p1Answer;
					if (DEBUG) {
						_debugStream << L"USE P1 ANSWER\n";
					}
				}
	
				// this definitely helps
				if (answer == _tagSet->getNoneTag()) {
					answer = confidentVectorAnswer;
					if (DEBUG) {
						_debugStream << L"FALL BACK TO CONFIDENT VECTOR ANSWER\n";
					}
				}
	
				// this is kind of a wash, but I include it on the assumption that the eval
				//  data will have better recall due to the consistency checks
				if (_maxentDecoder != 0 &&
					answer == _tagSet->getNoneTag() && 
					vectorAnswer == treeAnswer && 
					vectorAnswer != _tagSet->getNoneTag()) {
					answer = p1Answer;
					if (DEBUG) {
						_debugStream << L"VECTOR=TREE, SO USE P1 ANSWER\n";
					}
				}
			}
			if (answer == _tagSet->getNoneTag()) {
				answer = secondaryAnswer;
				if (secondaryAnswer != _tagSet->getNoneTag() && DEBUG) {
					_debugStream << L"SECONDARY SELECTED: " << secondaryAnswer.to_debug_string() << "\n";
					_p1SecondaryDecoder->printDebugInfo(_observation, _tagSet->getNoneTagIndex(), _debugStream);
					_p1SecondaryDecoder->printDebugInfo(_observation, _tagSet->getTagIndex(secondaryAnswer), _debugStream);
				}
			}

			if (answer != _tagSet->getNoneTag()) {
				Symbol ethnicType = fixEthnicityEtcRelation(mention1,
										mention2);
				if (ethnicType.is_null())
					ethnicType = fixEthnicityEtcRelation(mention2,
										mention1);
				if (!ethnicType.is_null()) {
					heuristic_fired = true;
					answer = ethnicType;
					if (DEBUG) {
						_debugStream << L"ETHNIC TYPE HEURISTIC: " << ethnicType.to_debug_string() << L"\n";
					}
				}
			}

			float confidence = 0;
			if (answer != _tagSet->getNoneTag()) {
				float reversed_score = shouldBeReversedScore(answer);
				if (DEBUG) {
					_debugStream << L"Reverse Score: " << reversed_score << "\n";
					if (reversed_score > 0)
						_debugStream << L"SO... reverse\n";
				}
					
//#ifdef Spanish_LANGUAGE
				confidence = (float)calculateConfidenceScore(mention1, mention2, reversed_score, answer, _observation);
//#else
//				confidence = (float) maxent_score;
//#endif

				if (heuristic_fired) 
					confidence = 1.0;

				if (reversed_score > 0) {
					addRelation(_observation->getMention2(), _observation->getMention1(), answer, confidence);
				} else {
					addRelation(_observation->getMention1(), _observation->getMention2(), answer, confidence);
				}
			} else tryForcingRelations(mentionSet);

			if (DEBUG) {
				_debugStream << L"FINAL ANSWER: " << answer.to_debug_string() << L"\n";
				_debugStream << L"  Confidence: " << confidence << L"\n";
				_debugStream << L"\n"; // Newline to separate the debug output for different relations.
				_debugStream.flush();
			}
		}
	}


	int killed_rel_mentions = 0;
	// What can we do about situations like "US military officials"
	// The relation finder will likely find (US, officials), (US, military), and (military, officials)
	// This is not really a great idea -- this solution is overkill, but oh well.

	for (int p = 0; p < propSet->getNPropositions(); p++) {
		const Proposition *prop = propSet->getProposition(p);
		if (prop->getNArgs() < 3 || prop->getPredType() != Proposition::NOUN_PRED)
			continue;
		const Mention *refMent = 0;
		const Mention *firstPremodMent = 0;
		const Mention *secondPremodMent = 0;
		for (int a = 0; a < prop->getNArgs(); a++) {
			if(prop->getArg(a)->getType() != Argument::MENTION_ARG){
				//std::cout<<"es_combo_relation_finder skipping non mention arg"<<std::endl;
				continue;
			}
			if (prop->getArg(a)->getRoleSym() == Argument::REF_ROLE)
				refMent = prop->getArg(a)->getMention(mentionSet);
			if (prop->getArg(a)->getRoleSym() == Argument::UNKNOWN_ROLE && firstPremodMent == 0)
				firstPremodMent = prop->getArg(a)->getMention(mentionSet);
			else if (prop->getArg(a)->getRoleSym() == Argument::UNKNOWN_ROLE &&	secondPremodMent == 0)
				secondPremodMent = prop->getArg(a)->getMention(mentionSet);
		}
		if (secondPremodMent == 0 || firstPremodMent == 0 || refMent == 0)
			continue;
		int ref_first = -1;
		int ref_second = -1;
		int first_second = -1;
		for (int j = 0; j < _n_relations; j++) {
			if (_relations[j] == 0)
				continue;
			if (_relations[j]->getLeftMention() == refMent) {
				if (_relations[j]->getRightMention() == firstPremodMent)
					ref_first = j;
				if (_relations[j]->getRightMention() == secondPremodMent)
					ref_second = j;
			} else if (_relations[j]->getLeftMention() == firstPremodMent) {
				if (_relations[j]->getRightMention() == refMent)
					ref_first = j;
				if (_relations[j]->getRightMention() == secondPremodMent)
					first_second = j;
			} else if (_relations[j]->getLeftMention() == secondPremodMent) {
				if (_relations[j]->getRightMention() == firstPremodMent)
					first_second = j;
				if (_relations[j]->getRightMention() == refMent)
					ref_second = j;
			} 
		}
		if (ref_first == -1 || ref_second == -1 || first_second == -1)
			continue;
		if (firstPremodMent->getIndex() < secondPremodMent->getIndex()) {
			killed_rel_mentions++;
			delete _relations[ref_first];
			_relations[ref_first] = 0;
		} else {
			killed_rel_mentions++;
			delete _relations[ref_second];
			_relations[ref_second] = 0;
		}
	}


	RelMentionSet *result = _new RelMentionSet();
	for (int j = 0; j < _n_relations; j++) {
		if (_relations[j] != 0)
			result->takeRelMention(_relations[j]);
		_relations[j] = 0;
	}


	return result;
}

/** This confidence metric was hand-crafted based on Python scripts that
 *  analyzed the effect of various features and conditions on the system's
 *  precision.  See text/Projects/SERIF/experiments/relation-confidence
 *  in svn for the code that generated it.  In the long run, it would 
 *  probably be better to do this with a trained machine learning model; 
 *  or if we had a less ad-hoc relation model, it might be possible to use
 *  a simple analysis of features to come up with a confidence score.  But
 *  this will do for now.  It's tuned for Spanish ACE, and may not work 
 *  well in other domains.  Here are the conditions it checks, and their
 *  (positive) impact on the confidence score:
 *
 *    0.152 overlap=='e=e'
 *    0.131 alt-model(LDC2004 PHYS.Part-Whole) (w=1.75)
 *    0.130 entity_types=='PER+GPE'
 *    0.126 rhs_entity_type=='GPE'
 *    0.124 rhs_entity_type!='PER'
 *    0.112 rhs_mention_type=='name'
 *    0.110 rhs_entity_subtype!='UNDET'
 *    0.103 reversed>9999.40587
 *    0.101 rhs_entity_subtype=='Nation'
 *    0.101 entity_types=='PER+ORG'
 *    0.099 alt-model(LDC2004 EMP-ORG.Employ-Staff) (w=2.12)
 *    0.097 reversed>9999.631035
 *    0.094 alt-model-enttype(LDC2004 PHYS.Part-Whole GPE GPE) (w=1.71)
 *
 *  The following graphs give an indication of the performance of the
 *  confidence metric (on heldout data):
 *
 *  1.000| Prec vs Score (Running avg)    100.0|      Precision vs Rank    :   
 *  0.857|                                 85.7|                           :   
 *  0.714|                      .:         71.4|                        .  :   
 *  0.571|                  .:  :: ...     57.1|                 .  .:. :..:   
 *  0.429|          ::... ::::.:::::::     42.9|           : :.  ::::::.::::   
 *  0.286|      : . ::::: ::::::::::::     28.6|       . .::::: .:::::::::::   
 *  0.143|   :: :.::::::::::::::::::::     14.3|.....:.:::::::::::::::::::::   
 *  0.000|::::::::::::::::::::::::::::      0.0|::::::::::::::::::::::::::::   
 *      0.00        0.44            1.0       0.00        0.44            1.0  
 * 
 *  1.000|:          Density              1.000|        Score vs Rank    .::   
 *  0.857|:                               0.857|                        .:::   
 *  0.714|:                               0.714|                   ...::::::   
 *  0.571|:               . .             0.571|             ..:::::::::::::   
 *  0.429|:              :: :        .    0.429|          .:::::::::::::::::   
 *  0.286|:  : ..  .  :..:: : :      :    0.286|       ..:::::::::::::::::::   
 *  0.143|: :: ::.::. ::::::: ::  :  :    0.143|    ..::::::::::::::::::::::   
 *  0.000|:_::::::::::::::::::::::::_:    0.000|____::::::::::::::::::::::::   
 *      0.00        0.44            1.0       0.00        0.44            1.0  
 * 
 *      Prec(top 10%): 56%       Gradient: 0.538
 *      Prec(top 25%): 54%      R-Squared: 0.118
 *      Prec(top 50%): 45%  Rank Gradient: 0.561
 *      Prec(bot 50%): 20%    x-intercept:-0.148
 *      Prec(bot 25%):  9%    y-intercept: 0.080
 */
double SpanishComboRelationFinder::calculateConfidenceScore(const Mention* mention1, const Mention* mention2, 
													 float reversed_score, Symbol answer, RelationObservation *obs) {
	double score = 0;
	if (DEBUG) 
		_debugStream << "Confidence Metric:\n";

	if (mention1->getNode()->getEndToken() == mention2->getNode()->getEndToken()) {
		score += 0.152;
		if (DEBUG) _debugStream << "  0.152 overlap=='e=e'\n";
	}
	for (int i = 0; i < obs->getNAltDecoderPredictions(); i++) {
		if ((obs->getNthAltDecoderName(i) == Symbol(L"LDC2004")) &&
			(obs->getNthAltDecoderPrediction(i) == Symbol(L"PHYS.Part-Whole"))) {
			score += 0.131;
			if (DEBUG) _debugStream << "  0.131 alt-model(LDC2004 PHYS.Part-Whole)\n";
		}
	}
	if ((mention1->getEntityType().matchesPER()) && 
		(mention2->getEntityType().matchesGPE())) {
		score += 0.130;
		if (DEBUG) _debugStream << "  0.130 entity_types=='PER+GPE'\n";
	}
	if (mention2->getEntityType().matchesGPE()) {
		score += 0.126;
		if (DEBUG) _debugStream << "  0.126 rhs_entity_type=='GPE'\n";
	}
	if (!mention2->getEntityType().matchesPER()) {
		score += 0.124;
		if (DEBUG) _debugStream << "  0.124 rhs_entity_type!='PER'\n";
	}
	if (mention2->getMentionType() == Mention::NAME) {
		score += 0.112;
		if (DEBUG) _debugStream << "  0.112 rhs_mention_type=='name'\n";
	}
	if (mention2->getEntitySubtype().isDetermined()) {
		score += 0.110;
		if (DEBUG) _debugStream << "  0.110 rhs_entity_subtype!='UNDET'\n";
	}
	if (reversed_score > 9999.40587) {
		score += 0.103;
		if (DEBUG) _debugStream << "  0.103 reversed>9999.40587\n";
	}
	if (mention2->getEntitySubtype().getName() == Symbol(L"Nation")) {
		score += 0.101;
		if (DEBUG) _debugStream << "  0.101 rhs_entity_subtype=='Nation'\n";
	}
	if ((mention1->getEntityType().matchesPER()) && 
		(mention2->getEntityType().matchesORG())) {
		score += 0.101;
		if (DEBUG) _debugStream << "  0.101 entity_types=='PER+ORG'\n";
	}
	for (int i = 0; i < obs->getNAltDecoderPredictions(); i++) {
		if ((obs->getNthAltDecoderName(i) == Symbol(L"LDC2004")) &&
			(obs->getNthAltDecoderPrediction(i) == Symbol(L"EMP-ORG.Employ-Staff"))) {
			score += 0.099;
			if (DEBUG) _debugStream << "  0.099 alt-model(LDC2004 EMP-ORG.Employ-Staff)\n";
		}
	}
	if (reversed_score > 9999.631035) {
		score += 0.097;
		if (DEBUG) _debugStream << "  0.097 reversed>9999.631035\n";
	}
	for (int i = 0; i < obs->getNAltDecoderPredictions(); i++) {
		if ((obs->getNthAltDecoderName(i) == Symbol(L"LDC2004")) &&
			(obs->getNthAltDecoderPrediction(i) == Symbol(L"PHYS.Part-Whole")) &&
			(mention1->getEntityType().matchesGPE()) && 
			(mention2->getEntityType().matchesGPE())) {
			score += 0.094;
			if (DEBUG) _debugStream << "  0.094 alt-model-enttype(LDC2004 PHYS.Part-Whole GPE GPE)\n";
		}
	}

	return std::min(score, 1.0);
}

Symbol SpanishComboRelationFinder::findSpecialCaseRelation(MentionSet *mentionSet) {

	RelationPropLink *propLink = _observation->getPropLink();

	// here in XXX
	/* DK todo
	if (_observation->getMention1()->getNode()->getHeadWord() == SpanishWordConstants::HERE &&
		_observation->getWordsBetween() == L"in" &&
		_observation->getMention1()->getEntityType().matchesLOC() &&
		(_observation->getMention2()->getEntityType().matchesLOC() ||
		 _observation->getMention2()->getEntityType().matchesGPE()))
	{
		return NO_RELATION_SYM;
	}
	*/
	if (!propLink->isEmpty()) {
		const Mention *refMent = 0;
		const Mention *unknownMent = 0;
		if (propLink->getArg1Role() == Argument::REF_ROLE &&
			propLink->getArg2Role() == Argument::UNKNOWN_ROLE)
		{
			refMent = propLink->getArg1Ment(mentionSet);
			unknownMent = propLink->getArg2Ment(mentionSet);
		} else if (propLink->getArg2Role() == Argument::REF_ROLE &&
			propLink->getArg1Role() == Argument::UNKNOWN_ROLE)
		{
			refMent = propLink->getArg2Ment(mentionSet);
			unknownMent = propLink->getArg1Ment(mentionSet);
		}
		if (refMent != 0 && unknownMent != 0 &&
			refMent->getEntityType().matchesPER())
		{
			Symbol type = fixEthnicityEtcRelation(refMent, unknownMent);
			if (!type.is_null())
				return type;
		}
	}

	// sign-offs
	if (!propLink->isEmpty() && propLink->getTopProposition()->getPredType() == Proposition::SET_PRED &&		
		propLink->getArg1Role() == Argument::MEMBER_ROLE &&
		propLink->getArg2Role() == Argument::MEMBER_ROLE)
	{
		Proposition *prop = propLink->getTopProposition();
		if (prop->getNArgs() >= 4 &&
			prop->getArg(0)->getType() == Argument::MENTION_ARG &&
			prop->getArg(1)->getType() == Argument::MENTION_ARG &&
			prop->getArg(2)->getType() == Argument::MENTION_ARG &&
			prop->getArg(3)->getType() == Argument::MENTION_ARG) 
		{
			const Mention *person = prop->getArg(1)->getMention(mentionSet);
			const Mention *organization = prop->getArg(2)->getMention(mentionSet);
			const Mention *location = prop->getArg(3)->getMention(mentionSet);
			if (person->getEntityType().matchesPER() &&
				organization->getEntityType().matchesORG() &&
				(location->getEntityType().matchesGPE() ||
				location->getEntityType().matchesFAC() ||
				location->getEntityType().matchesLOC()))
			{
				if ((_observation->getMention1() == person && 
					_observation->getMention2() == organization) ||
					(_observation->getMention2() == person &&
					_observation->getMention1() == organization))
				{
					return SpecialRelationCases::getEmployeesType();
				} else if ((_observation->getMention1() == person && 
					_observation->getMention2() == location) ||
					(_observation->getMention2() == person &&
					_observation->getMention1() == location))
				{
					return SpecialRelationCases::getLocatedType();
				} else return NO_RELATION_SYM;
			}
		}
	}

	return _tagSet->getNoneTag();

}


void SpanishComboRelationFinder::addRelation(const Mention *first, const Mention *second, Symbol type, float score) {
	if (_n_relations < MAX_SENTENCE_RELATIONS) {
		_relations[_n_relations] = _new RelMention(first, second,
			type, _currentSentenceIndex, _n_relations, score);
		_n_relations++;
	}
}

// THIS SHOULD GET CHANGED INTO A LIST. I AM TIRED AND NOT GOING TO DO IT RIGHT NOW.

static Symbol jewishSym = Symbol(L"jewish");
static Symbol westernSym = Symbol(L"western");
static Symbol republicanSym = Symbol(L"republican");
static Symbol christianSym = Symbol(L"christian");
static Symbol muslimSym = Symbol(L"muslim");
static Symbol democraticSym = Symbol(L"democratic");
static Symbol democratSym = Symbol(L"democrat");
static Symbol islamicSym = Symbol(L"islamic");
static Symbol shiiteSym = Symbol(L"shiite");
static Symbol shiite2Sym = Symbol(L"shi'ite");
static Symbol sunniSym = Symbol(L"sunni");
static Symbol communistSym = Symbol(L"communist");
static Symbol sikhSym = Symbol(L"sikh");
static Symbol catholicSym = Symbol(L"catholic");
static Symbol hinduSym = Symbol(L"hindu");
static Symbol methodistSym = Symbol(L"methodist");
static Symbol neonaziSym = Symbol(L"neo-nazi");
static Symbol zionistSym = Symbol(L"zionist");
static Symbol arabSym = Symbol(L"arab");
static Symbol kurdishSym = Symbol(L"kurdish");
static Symbol hispanicSym = Symbol(L"hispanic");
static Symbol africanAmericanSym = Symbol(L"african-american");
static Symbol asianAmericanSym = Symbol(L"asian-american");
static Symbol persianSym = Symbol(L"persian");
static Symbol latinoSym = Symbol(L"latino");
static Symbol latinaSym = Symbol(L"latina");
static Symbol creoleSym = Symbol(L"creole");
static Symbol angloSym = Symbol(L"anglo");
static Symbol chicanoSym = Symbol(L"chicano");
static Symbol chicanaSym = Symbol(L"chicana");

Symbol SpanishComboRelationFinder::fixEthnicityEtcRelation(const Mention *first, const Mention *second) {

	Symbol forcedType = Symbol();

	if (second->getMentionType() == Mention::NAME) {
		
		Symbol headword = second->getNode()->getHeadWord();
		if (headword == jewishSym ||
			headword == westernSym)
		{			
			// ACE 2004
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"OTHER-AFF.Other")) 
				!= RelationTypeSet::INVALID_TYPE)
				forcedType = Symbol(L"OTHER-AFF.Other");

			// ACE 2005
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity")) 
				!= RelationTypeSet::INVALID_TYPE)
				forcedType = Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity");

		} else if (headword == christianSym ||
			headword == muslimSym ||
			headword == islamicSym ||
			headword == shiiteSym ||
			headword == shiite2Sym ||
			headword == sunniSym ||
			headword == sikhSym ||
			headword == catholicSym ||
			headword == hinduSym ||
			headword == methodistSym ||
			headword == neonaziSym ||
			headword == zionistSym)
		{
			// ACE 2004
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"OTHER-AFF.Ideology")) ==
				RelationTypeSet::INVALID_TYPE)
				forcedType = Symbol(L"OTHER-AFF.Ideology");

			// ACE 2005
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity")) 
				!= RelationTypeSet::INVALID_TYPE)
				forcedType = Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity");


		} else if (headword == arabSym ||
			headword == hispanicSym ||
			headword == africanAmericanSym ||
			headword == asianAmericanSym ||
			headword == persianSym ||
			headword == kurdishSym ||
			headword == latinoSym ||
			headword == latinaSym||
			headword == creoleSym ||
			headword == chicanoSym ||
			headword == chicanaSym ||
			headword == angloSym)
		{
			// ACE 2004
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"OTHER-AFF.Ethnic")) 
				!= RelationTypeSet::INVALID_TYPE)
				forcedType = Symbol(L"OTHER-AFF.Ethnic");

			// ACE 2005
			if (RelationTypeSet::getTypeFromSymbol(Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity")) 
				!= RelationTypeSet::INVALID_TYPE)
				forcedType = Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity");
		}
	}

	return forcedType;
}

void SpanishComboRelationFinder::findMentionSetChanges(MentionSet *mentionSet) {
	const Mention *m1 = _observation->getMention1();
	const Mention *m2 = _observation->getMention2();

	const Mention *knownMention = 0;
	const Mention *unknownMention = 0;
	if (m1->getEntityType().isRecognized() && !m2->getEntityType().isRecognized()) {
		knownMention = m1;
		unknownMention = m2;
	} else if (m2->getEntityType().isRecognized() && !m1->getEntityType().isRecognized()) {
		knownMention = m2;
		unknownMention = m1;
	}

	// if there's no proplink, we don't know nearly enough to be doing this kind of thing
	if (_observation->getPropLink()->isEmpty() || _observation->getPropLink()->isNegative())
		return;

	// look at potential people pronouns
	if (unknownMention != 0) {
		Symbol headword = unknownMention->getNode()->getHeadWord();
		/* DK todo
		if (unknownMention->getMentionType() == Mention::PRON &&
			(headword == SpanishWordConstants::THEIR ||
			headword == SpanishWordConstants::THEY ||
			headword == SpanishWordConstants::THEM ||
			headword == SpanishWordConstants::WHO ||
			headword == SpanishWordConstants::WHOM ||
			headword == SpanishWordConstants::WHOSE))
		{
			mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getPERType());
			_inst->setStandardInstance(_observation);
			Symbol answer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			if (answer == Symbol(L"PER-SOC.Family") ||
				answer == Symbol(L"PER-SOC.Business") ||
				answer == Symbol(L"PER-SOC.Lasting-Personal") ||
				answer == Symbol(L"ORG-AFF.Employment"))
			{			
				//std::cerr << "Forcing pronoun (" << headword << ") to PER due to relation:\n";
				//std::cerr << _observation->getPropLink()->getTopProposition()->toDebugString() << "\n";
			} else mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getOtherType());
		} else*/ if (unknownMention->getMentionType() == Mention::DESC) {
			if (WordNet::getInstance()->isHyponymOf(headword, "person")) {
				mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getPERType());
				_inst->setStandardInstance(_observation);		
				//Symbol answer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
				//if (answer != _tagSet->getNoneTag()) {			
				//	//std::cerr << "Forcing desc (" << headword << ") to PER due to relation:\n";
				//	//std::cerr << _observation->getPropLink()->getTopProposition()->toDebugString() << "\n";
				//} else mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getOtherType());
			} else if (WordNet::getInstance()->isHyponymOf(headword, "facility")) {
				mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getFACType());
				_inst->setStandardInstance(_observation);		
				//Symbol answer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
				//if (answer != _tagSet->getNoneTag()) {			
					//std::cerr << "Forcing desc (" << headword << ") to FAC due to relation:\n";
					//std::cerr << _observation->getPropLink()->getTopProposition()->toDebugString() << "\n";
				//} else mentionSet->changeEntityType(unknownMention->getUID(), EntityType::getOtherType());
			} 
		}
	} 

	const Mention *pronMention = 0;
	const Mention *otherMention = 0;
	if (m1->getMentionType() == Mention::PRON && m2->getMentionType() != Mention::PRON) {
		pronMention = m1;
		otherMention = m2;
	} else if (m2->getMentionType() == Mention::PRON && m1->getMentionType() != Mention::PRON) {
		pronMention = m2;
		otherMention = m1;
	} 
		
	if (pronMention != 0 && pronMention->getEntityType().matchesPER()) {
		Symbol headword = pronMention->getNode()->getHeadWord();
		/* DK todo
		if (headword == SpanishWordConstants::WE ||
			headword == SpanishWordConstants::OUR)
		{
			//std::cerr << "Found 1p plural pronoun\n";
			if (pronMention->getNode()->getParent() != 0 &&
				pronMention->getNode()->getParent()->getParent() != 0)
			{
				//std::cerr << pronMention->getNode()->getParent()->getParent()->toDebugTextString() << "\n";
			}
			// should start as PER type
			_inst->setStandardInstance(_observation);
			float per_score = -10000;
			float org_score = -10000;
			float gpe_score = -10000;
			Symbol perAnswer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			//std::cerr << "PER -- " << perAnswer << "\n";
			if (perAnswer != _tagSet->getNoneTag()) {
				_inst->setRelationType(perAnswer);
				per_score = _vectorModel->lookup(_inst);
			}
			mentionSet->changeEntityType(pronMention->getUID(), EntityType::getORGType());
			_inst->setStandardInstance(_observation);
			Symbol orgAnswer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			if (orgAnswer != _tagSet->getNoneTag()) {
				_inst->setRelationType(orgAnswer);
				org_score = _vectorModel->lookup(_inst);
				//std::cerr << "ORG -- " << orgAnswer << " " << org_score << "\n";				
			}
			mentionSet->changeEntityType(pronMention->getUID(), EntityType::getGPEType());
			_inst->setStandardInstance(_observation);
			Symbol gpeAnswer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
			//std::cerr << "GPE -- " << gpeAnswer << "\n";
			if (gpeAnswer != _tagSet->getNoneTag()) {
				_inst->setRelationType(gpeAnswer);
				gpe_score = _vectorModel->lookup(_inst);
			}

			EntityType etype = EntityType::getPERType();
			if (org_score != -10000 && org_score >= per_score && org_score >= gpe_score) {
				etype = EntityType::getORGType();
			} else if (gpe_score != -10000 && gpe_score >= per_score && gpe_score >= org_score) {
				etype = EntityType::getGPEType();
			} 
			if (!etype.matchesPER()) {
				//std::cerr << "Forcing pronoun (" << headword << ") to " << etype.getName() << " due to relation:\n";
				//std::cerr << _observation->getPropLink()->getTopProposition()->toDebugString() << "\n";
				mentionSet->changeEntityType(pronMention->getUID(), etype);
			} else mentionSet->changeEntityType(pronMention->getUID(), EntityType::getPERType());
		} 
		*/
	}


}



void SpanishComboRelationFinder::tryForcingRelations(MentionSet *mentionSet) {

	const Mention *m1 = _observation->getMention1();
	const Mention *m2 = _observation->getMention2();

	// at least one has to be of a known type!
	if (!m1->getEntityType().isRecognized() && !m2->getEntityType().isRecognized())
		return;

	// ought to know what we're doing at least a little to try this
	if (_observation->getPropLink()->isEmpty())
		return;

	Symbol acceptableTypes[4];
	acceptableTypes[0] = Symbol(L"PHYS.Located");
	acceptableTypes[1] = Symbol(L"PHYS.Near");
	acceptableTypes[2] = Symbol(L"PART-WHOLE.Geographical");
	acceptableTypes[3] = Symbol();

	/*const Mention *orgMention = 0;
	if (m1->getEntityType().matchesORG() && m2->getEntityType().matchesPER()) {
		if (_facOrgWords->lookup(m1->getNode()->getHeadWord()))
			orgMention = m1;
	} else if (m2->getEntityType().matchesORG() && m1->getEntityType().matchesPER()) {
		if (_facOrgWords->lookup(m2->getNode()->getHeadWord())) {
			orgMention = m2;
		}
	}

	if (orgMention != 0) {
		if (tryFakeTypeRelation(mentionSet, orgMention,
			EntityType::getLOCType(), acceptableTypes))
			return;
		if (tryFakeTypeRelation(mentionSet, orgMention,
			EntityType::getFACType(), acceptableTypes))
			return;
		if (tryFakeTypeRelation(mentionSet, orgMention,
			EntityType::getGPEType(), acceptableTypes))
			return;
	}

	const Mention *facMention = 0;
	if (m1->getEntityType().matchesFAC()) {
		facMention = m1;
	} else if (m2->getEntityType().matchesFAC()) {
		facMention = m2;
	}

	if (facMention != 0) {
		if (tryFakeTypeRelation(mentionSet, facMention,
			EntityType::getLOCType(), acceptableTypes))
			return;		
		if (tryFakeTypeRelation(mentionSet, facMention,
			EntityType::getGPEType(), acceptableTypes))
			return;
	}

	const Mention *locMention = 0;
	if (m1->getEntityType().matchesLOC()) {
		locMention = m1;
	} else if (m2->getEntityType().matchesLOC()) {
		locMention = m2;
	}

	if (locMention != 0) {
		if (tryFakeTypeRelation(mentionSet, locMention,
			EntityType::getFACType(), acceptableTypes))
			return;		
		if (tryFakeTypeRelation(mentionSet, locMention,
			EntityType::getGPEType(), acceptableTypes))
			return;
	}
	
	const Mention *gpeMention = 0;
	if (m1->getEntityType().matchesGPE()) {
		gpeMention = m1;
	} else if (m2->getEntityType().matchesGPE()) {
		gpeMention = m2;
	}

	if (gpeMention != 0) {
		if (tryFakeTypeRelation(mentionSet, gpeMention,
			EntityType::getLOCType(), acceptableTypes))
			return;		
		if (tryFakeTypeRelation(mentionSet, gpeMention,
			EntityType::getFACType(), acceptableTypes))
			return;
	}*/

	const Mention *othMention = 0;
	if (!m1->getEntityType().isRecognized()) {
		othMention = m1;
	} else if (!m2->getEntityType().isRecognized()) {
		othMention = m2;
	}

	if (othMention != 0) {
		Symbol headword = othMention->getNode()->getHeadWord();
		if (WordNet::getInstance()->isHyponymOf(headword, "person")) {
			if (tryFakeTypeRelation(mentionSet, othMention,
				EntityType::getPERType(), acceptableTypes))
				return;
		} else if (WordNet::getInstance()->isHyponymOf(headword, "facility")) {
			if (tryFakeTypeRelation(mentionSet, othMention,
				EntityType::getFACType(), acceptableTypes))
				return;
		} /*else if (WordNet::getInstance()->isHyponymOf(headword, "location")) {
			if (tryFakeTypeRelation(mentionSet, othMention,
				EntityType::getLOCType(), acceptableTypes))
				return;
		} else if (WordNet::getInstance()->isHyponymOf(headword, "organization")) {
			if (tryFakeTypeRelation(mentionSet, othMention,
				EntityType::getORGType(), acceptableTypes))
				return;
		} */
	}


}
bool SpanishComboRelationFinder::tryFakeTypeRelation(MentionSet *mentionSet,
												 const Mention *possMention,
												 EntityType possibleType,
												 Symbol *acceptableTypes)
{
	EntityType origType = possMention->getEntityType();

	if (!_allow_mention_set_changes && !origType.isRecognized())
		return false;

	mentionSet->changeEntityType(possMention->getUID(), possibleType);
	Symbol answer = _tagSet->getNoneTag();
	
	// just use confident vectors for these faked types
	RelationPropLink *link = _observation->getPropLink();
	_inst->setStandardInstance(_observation);
	if (!link->isEmpty() && !link->isNegative()) {
		answer = RelationTypeSet::getRelationSymbol(_vectorModel->findConfidentRelationType(_inst));
	}	

	if (answer != _tagSet->getNoneTag() &&
		(answer == acceptableTypes[0] || answer == acceptableTypes[1] ||
		answer == acceptableTypes[2] || answer == acceptableTypes[3]))
	{
		//std::cerr << "\nFORCED (from ";
		//std::cerr << origType.getName().to_debug_string() << " to ";
		//std::cerr << possibleType.getName().to_debug_string() << "):\n";
		//std::cerr << _observation->getMention1()->getNode()->toDebugTextString() << "\n";
		//std::cerr << _observation->getMention2()->getNode()->toDebugTextString() << "\n";
		//std::cerr << answer.to_debug_string() << "\n";

		if (origType.isRecognized())
			mentionSet->changeEntityType(possMention->getUID(), origType);		
		
		// LB: just always use 0.5 as the score for these relations.... I don't know what's best here...
		if (shouldBeReversed(answer)) {
			addRelation(_observation->getMention2(), _observation->getMention1(), answer, 0.5);
		} else {
			addRelation(_observation->getMention1(), _observation->getMention2(), answer, 0.5);
		}

		return true;
	} else {
		// if not, just change it back
		mentionSet->changeEntityType(possMention->getUID(), origType);
	}
	return false;
}

/*bool SpanishComboRelationFinder::tryCoercedTypeRelation(MentionSet *mentionSet, EntitySet *entitySet,
												 const Mention *possMention,
												 EntityType possibleType)
{
	EntityType origType = possMention->getEntityType();

	if (!possibleType.isRecognized())
		return false;

	std::cerr << "Trying: " << possMention->getNode()->toDebugTextString() << "\n";

	// let's see if it would be a relation
	mentionSet->changeEntityType(possMention->getUID(), possibleType);
	Symbol answer = _decoder->decodeToSymbol(_observation);

	if (answer == _tagSet->getNoneTag()) {
		mentionSet->changeEntityType(possMention->getUID(), origType);
		return false;
	}

	float answer_score = _decoder->lookupScore(_observation, answer);
	float none_score = _decoder->lookupScore(_observation, answer);
	float diff = answer_score - none_score;
	if (diff / answer_score < .5) {
		std::cerr << "Relation not sure\n";
		mentionSet->changeEntityType(possMention->getUID(), origType);
		return false;
	}

	std::cerr << "FORCED (from ";
	std::cerr << origType.getName().to_debug_string() << " to ";
	std::cerr << possibleType.getName().to_debug_string() << "):\n";
	std::cerr << _observation->getMention1()->getNode()->toDebugTextString() << "\n";
	std::cerr << _observation->getMention2()->getNode()->toDebugTextString() << "\n";
	std::cerr << answer.to_debug_string() << "\n";

	entitySet->getNonConstLastMentionSet()->changeEntityType(possMention->getUID(),
		possibleType);
	entitySet->addNew(possMention->getUID(), possibleType);

	if (shouldBeReversed(answer))
		addRelation(_observation->getMention2(), _observation->getMention1(), answer);
	else addRelation(_observation->getMention1(), _observation->getMention2(), answer);

	return true;
}
*/

bool SpanishComboRelationFinder::shouldBeReversed(Symbol answer) {
	return shouldBeReversedScore(answer) > 0;
}

float SpanishComboRelationFinder::shouldBeReversedScore(Symbol answer) {
	/*int int_answer = RelationTypeSet::getTypeFromSymbol(answer);
	if (!RelationTypeSet::isSymmetric(int_answer)) {
		_inst->setRelationType(answer);
		float non_reversed_score = _vectorModel->lookupB2P(_inst);

		int rev_int_answer = RelationTypeSet::reverse(int_answer);
		_inst->setRelationType(RelationTypeSet::getRelationSymbol(rev_int_answer));
		float reversed_score = _vectorModel->lookupB2P(_inst);

		return (reversed_score - non_reversed_score);
	}*/
	return 0;
}

void SpanishComboRelationFinder::loadSymbolHash(SymbolHash *hash, const char* file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	if (stream.fail()) {
		string err = "problem opening ";
		err.append(file);
		throw UnexpectedInputException("SpanishComboRelationFinder::loadSymbolHash()",
			(char *)err.c_str());
	}

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


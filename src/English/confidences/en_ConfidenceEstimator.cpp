// Copyright 2014 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/confidences/en_ConfidenceEstimator.h"
#include "English/names/en_IdFWordFeatures.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/MentionConfidence.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"

#include <cfloat>

#include "boost/foreach.hpp"


/** These confidence metrics were hand-crafted based on Python scripts that
 *  analyzed the effect of various features and conditions on the system's
 *  precision.  See text/Projects/CVE/python/confidence
 *  in svn for the code that generated them.  In the long run, it would 
 *  probably be better to do this with trained machine learning models; 
 *  but this will do for now. It's tuned for English ACE, and may not work 
 *  well in other domains.
 */
EnglishConfidenceEstimator::EnglishConfidenceEstimator() : _wordFeatures(0), _estimate_confidences(true) { 
	_estimate_confidences = ParamReader::getOptionalTrueFalseParamWithDefaultVal("estimate_confidences", false);
	_wordFeatures = IdFWordFeatures::build();

}

EnglishConfidenceEstimator::~EnglishConfidenceEstimator() { delete _wordFeatures; }

void EnglishConfidenceEstimator::process(DocTheory *docTheory) {
	if (_estimate_confidences) {
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(i);
			RelMentionSet *relMentionSet = sentTheory->getRelMentionSet();
			for (int j = 0; j < relMentionSet->getNRelMentions(); j++) {
				RelMention *relMention = relMentionSet->getRelMention(j);
				double confidence = calculateConfidenceScore(relMention);
				relMention->setConfidence((float)confidence);
			}	
		}
		EntitySet *entitySet = docTheory->getEntitySet();
		for (int k = 0; k < entitySet->getNEntities(); k++) {
			Entity *entity = entitySet->getEntity(k);
			addMentionConfidenceScores(entity, docTheory);
		}
	}
}


/**
 * Note: There are some extra steps in this method to handle the fact that the Mentions 
 * contained by the EntitySet are copies of the Mentions in the sentence-level
 * MentionSets (to allow for multiple theories). Here we explicitly set the confidence 
 * values for both copies of the Mention. This could cause unpredictable behavior if we
 * were to use a beam with multiple MentionSets, but we currently never do that.  
 */
void EnglishConfidenceEstimator::addMentionConfidenceScores(const Entity *entity, const DocTheory *docTheory) const {
	const Mention* canonical = getBestMention(entity, docTheory);
	
	EntitySet *entitySet = docTheory->getEntitySet();
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* mention = entitySet->getMention(entity->getMention(i));
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(mention->getSentenceNumber());
		Mention *sentenceLevelMention = sentTheory->getMentionSet()->getMention(mention->getUID());
		
		// Mention confidence
		double confidence = calculateConfidenceScore(mention, docTheory);
		mention->setConfidence((float)confidence);
		sentenceLevelMention->setConfidence((float)confidence);
		
		// Link confidence - only produced for entities with a canonical name
		if (canonical->getMentionType() == Mention::NAME) {
			double link_confidence = calculateLinkConfidenceScore(mention, canonical, docTheory);
			mention->setLinkConfidence((float)link_confidence);
			sentenceLevelMention->setLinkConfidence((float)link_confidence);
		}
			
	}
}

double EnglishConfidenceEstimator::calculateConfidenceScore(const Mention *mention, const DocTheory *docTheory) const {
	double score = 0;

	SessionLogger::dbg("mention_conf") << "Mention Confidence Metric:\n";

	Mention::Type type = mention->getMentionType();
	switch(type) {
		case Mention::NAME:
			score = calculateNameConfidenceScore(mention, docTheory);
			break;
		case Mention::DESC:
			score = calculateDescConfidenceScore(mention, docTheory);
			break;
		default:
			SessionLogger::dbg("mention_conf") << Mention::getTypeString(type) << ": " << mention->toAtomicCasedTextString() << "\n";
	}

	SessionLogger::dbg("mention_conf") << "Mention Confidence: " << score << "\n"; 
	return score;
}



/** 
 *  Name Confidence
 *
 *  [0.254] entity_subtype==u'Individual'
 *  [0.204] verb_prop==u'<sub>:said'
 *  [0.178] entity_subtype==u'Population-Center'
 *  [0.176] idf_word_features==':allCaps'
 *  [0.170] in_appo==True
 *  [0.165] entity_type==u'PER'
 *  [0.141] entity_subtype==u'Nation'
 *  [0.119] idf_word_features!=':initCap'
 *  [0.112] entity_subtype!=u'UNDET'
 *  [0.088] idf_word_features!=':containsAlphaAndPunct'
 *
 * On heldout data:
 *
 *        Prec vs Score (Running avg)                Precision vs Rank         
 *  1.000|         . ......... ..::: :    100.0|                ...    ....:   
 *  0.857|      :  ::::::::::: ::::: :     85.7|         ::::: :::::::::::::   
 *  0.714|     :: :::::::::::: ::::: :     71.4|       :::::::::::::::::::::   
 *  0.571|::::.::::::::::::::: ::::: :     57.1|:::::.::::::::::::::::::::::   
 *  0.429|:::::::::::::::::::: ::::: :     42.9|::::::::::::::::::::::::::::   
 *  0.286|:::::::::::::::::::: ::::: :     28.6|::::::::::::::::::::::::::::   
 *  0.143|:::::::::::::::::::: ::::: :     14.3|::::::::::::::::::::::::::::   
 *  0.000|::::::::::::::::::::_:::::_:      0.0|::::::::::::::::::::::::::::   
 *      0.00        0.44            1.0       0.00        0.44            1.0  
 *  
 *  1.000|      :    Density              1.000|        Score vs Rank      :   
 *  0.857|  .   :  .                      0.857|                           :   
 *  0.714|  :   :  :                      0.714|                          .:   
 *  0.571|  :   :  :                      0.571|                        .:::   
 *  0.429|  :  .:  :.                     0.429|                    ...:::::   
 *  0.286|  :  ::  ::     .               0.286|        .......:::::::::::::   
 *  0.143|. :..::  ::...  :               0.143| .....::::::::::::::::::::::   
 *  0.000|:_:::::::::::::::::_:::_:__:    0.000|::::::::::::::::::::::::::::   
 *      0.00        0.44            1.0       0.00        0.44            1.0  
 *
 *      Prec(top 10%): 97%       Gradient: 0.715       Bin Diff: 1.16 (high=good)  
 *      Prec(top 25%): 95%      R-Squared: 0.092     Bin Diff 2: 3.34 (low=good)   
 *      Prec(top 50%): 91%  Rank Gradient: 0.418        bin[-1]: 0.97 (high=good)  
 *      Prec(bot 50%): 73%    x-intercept:-0.856      max score: 65.53             
 *      Prec(bot 25%): 60%    y-intercept: 0.612  unique scores: 46                
 */
double EnglishConfidenceEstimator::calculateNameConfidenceScore(const Mention *mention, const DocTheory *docTheory) const {
	double score = 0;

	// Must be a NAME matching one of the 5 primary entity types
	// and not a speaker mention.
	if (mention->getMentionType() != Mention::NAME ||
		isSpeakerMention(mention, docTheory) ||
		(!mention->getEntityType().matchesFAC() &&
		!mention->getEntityType().matchesGPE() &&
		!mention->getEntityType().matchesLOC() &&
		!mention->getEntityType().matchesORG() &&
		!mention->getEntityType().matchesPER()))
	{	
		return score;
	}

	SessionLogger::dbg("mention_conf") << "Name: " << mention->toAtomicCasedTextString() << "\n";

	if (mention->getEntitySubtype().getName() == Symbol(L"Individual")) {
		score += 0.254;
		SessionLogger::dbg("mention_conf") << "  0.254 entity_subtype=='Individual'\n";
	}
	if (mention->getEntitySubtype().getName() == Symbol(L"Population-Center")) {
		score += 0.178;
		SessionLogger::dbg("mention_conf") << "  0.178 entity_subtype=='Population-Center'\n";
	}
	if (mention->isPartOfAppositive()) {
		score += 0.176;
		SessionLogger::dbg("mention_conf") << "  0.176 in_appo==True\n";
	}
	if (mention->getEntityType().matchesPER()) {
		score += 0.165;
		SessionLogger::dbg("mention_conf") << "  0.165 entity_type==PER\n";
	}
	if (mention->getEntitySubtype().getName() == Symbol(L"Nation")) {
		score += 0.141;
		SessionLogger::dbg("mention_conf") << "  0.141 entity_subtype=='Nation'\n";
	}
	if (mention->getEntitySubtype().isDetermined()) {
		score += 0.112;
		SessionLogger::dbg("mention_conf") << "  0.112 entity_subtype!='UNDET'\n";
	}

	Symbol features[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	// TODO: we really shouldn't be making Symbols out of multi-word strings,
	// the interface to IdFWordFeatures needs to be updated.
	Symbol nameSym = Symbol(mention->toAtomicCasedTextString());
	int n_word_features = _wordFeatures->getAllFeatures(nameSym, false, false,
						features, DTFeatureType::MAX_FEATURES_PER_EXTRACTION);
	bool found_init_cap = false;
	bool found_alpha_and_punct = false;
	for (int i = 0; i < n_word_features; i++) {
		if (features[i] == EnglishIdFWordFeatures::allCaps) {
			score += 0.176;
			SessionLogger::dbg("mention_conf") << "  0.176 idf_word_features==':allCaps'\n";
		}
		if (features[i] == EnglishIdFWordFeatures::initCap) {
			found_init_cap = true;
		}
		if (features[i] == EnglishIdFWordFeatures::containsAlphaAndPunct) {
			found_alpha_and_punct = true;
		}
	}
	if (!found_init_cap) {
		score += 0.119;
		SessionLogger::dbg("mention_conf") << "  0.119 idf_word_features!=':initCap'\n";
	}
	if (!found_alpha_and_punct) {
		score += 0.088;
		SessionLogger::dbg("mention_conf") << "  0.088 idf_word_features!=':containsAlphaAndPunct'\n";
	}

	std::vector<Proposition*> props = verbPropositions(mention, docTheory);
	BOOST_FOREACH(Proposition* prop, props) {
		if (prop->getPredSymbol() == Symbol(L"said")) {
			const Mention *m = prop->getMentionOfRole(Argument::SUB_ROLE, mention->getMentionSet());
			if (m != 0) {
				while (m->getChild() != 0) {
					m = m->getChild();
				}
				if (m->getIndex() == mention->getIndex()) {
					score += 0.204;
					SessionLogger::dbg("mention_conf") << "  0.204 verb_prop=='<sub>:said'\n";	
				}
			}
		}
	}

	return std::min(score, 1.0);
}

/**
 *  Descriptor Confidence
 *
 *  [0.071] entity_subtype!=u'Nation'
 *  [0.071] headtag!='DT'
 *  [0.071] headtag!='JJ'
 *  [0.071] verb_prop!=u'<obj>:saying'
 *  [0.071] verb_prop!=u'<obj>:think'
 *  [0.071] entity_type!=u'LOC'
 *  [0.071] verb_prop!=u'<obj>:have'
 *  [0.070] entity_type!=u'GPE'
 *  [0.070] entity_type!=u'FAC'
 *  [0.070] headtag!='NNPS'
 *  [0.069] entity_is_generic==False
 *  [0.069] headtag!='NNP'
 *  [0.066] entity_type==u'PER'
 *  [0.054] headtag=='NN'
 *  [0.036] in_relation==True
 *  
 * On heldout data:
 *   
 *         Prec vs Score (Running avg)                                          
 *   1.000|                  .        .    100.0|      Precision vs Rank ...:   
 *   0.857|   .        .   .:: :::.::::     85.7|      .  .:: .:::...::::::::   
 *   0.714|   :.  ...:::..:::::::::::::     71.4|..::: :: :::::::::::::::::::   
 *   0.571|   :::::::::::::::::::::::::     57.1|::::::::::::::::::::::::::::   
 *   0.429|   :::::::::::::::::::::::::     42.9|::::::::::::::::::::::::::::   
 *   0.286|   :::::::::::::::::::::::::     28.6|::::::::::::::::::::::::::::   
 *   0.143|   :::::::::::::::::::::::::     14.3|::::::::::::::::::::::::::::   
 *   0.000|___:::::::::::::::::::::::::      0.0|::::::::::::::::::::::::::::   
 *       0.00        0.44            1.0       0.00        0.44            1.0  
 *   
 *                   Density                                                    
 *   1.000|               :        .       1.000|        Score vs Rank   ::::   
 *   0.857|               :        :  .    0.857|                 ..:::::::::   
 *   0.714|               :    .   :  :    0.714|           .::::::::::::::::   
 *   0.571|               :   .:  .:  :    0.571|    .....:::::::::::::::::::   
 *   0.429|              ::   ::  ::  :    0.429|  .:::::::::::::::::::::::::   
 *   0.286|         ..   ::.  :: .::  :    0.286|.:::::::::::::::::::::::::::   
 *   0.143|     .   ::.  :::..:: :::  :    0.143|::::::::::::::::::::::::::::   
 *   0.000|:_::::::_::::::::::::::::__:    0.000|::::::::::::::::::::::::::::   
 *       0.00        0.44            1.0       0.00        0.44            1.0  
 *   
 *       Prec(top 10%): 94%       Gradient: 0.359       Bin Diff: 0.69 (high=good)  
 *       Prec(top 25%): 90%      R-Squared: 0.037     Bin Diff 2: 3.43 (low=good)   
 *       Prec(top 50%): 88%  Rank Gradient: 0.266        bin[-1]: 0.94 (high=good)  
 *       Prec(bot 50%): 74%    x-intercept:-1.560      max score: 655.86            
 *       Prec(bot 25%): 71%    y-intercept: 0.561  unique scores: 98                
 */
double EnglishConfidenceEstimator::calculateDescConfidenceScore(const Mention *mention, const DocTheory *docTheory) const {
	double score = 0;

	// Must be a DESC matching one of the 5 primary entity types
	// and not a speaker mention.
	if (mention->getMentionType() != Mention::DESC ||
		isSpeakerMention(mention, docTheory) ||
		(!mention->getEntityType().matchesFAC() &&
		!mention->getEntityType().matchesGPE() &&
		!mention->getEntityType().matchesLOC() &&
		!mention->getEntityType().matchesORG() &&
		!mention->getEntityType().matchesPER()))
	{	
		return score;
	}

	SessionLogger::dbg("mention_conf") << "Desc: " << mention->toAtomicCasedTextString() << "\n";

	if (mention->getEntitySubtype().getName() != Symbol(L"Nation")) {
		score += 0.071;
		SessionLogger::dbg("mention_conf") << "  0.071 entity_subtype!='Nation'\n";
	}
	if (mention->getAtomicHead()->getTag() != Symbol(L"DT")) {
		score += 0.071;
		SessionLogger::dbg("mention_conf") << "  0.071 headtag!='DT'\n";
	}
	if (mention->getAtomicHead()->getTag() != Symbol(L"JJ")) {
		score += 0.071;
		SessionLogger::dbg("mention_conf") << "  0.071 headtag!='JJ'\n";
	}
	if (!mention->getEntityType().matchesLOC()) {
		score += 0.071;
		SessionLogger::dbg("mention_conf") << "  0.071 entity_type!='LOC'\n";
	}
	if (!mention->getEntityType().matchesGPE()) {
		score += 0.070;
		SessionLogger::dbg("mention_conf") << "  0.071 entity_type!='GPE'\n";
	}
	if (!mention->getEntityType().matchesFAC()) {
		score += 0.070;
		SessionLogger::dbg("mention_conf") << "  0.071 entity_type!='FAC'\n";
	}
	if (mention->getAtomicHead()->getTag() != Symbol(L"NNPS")) {
		score += 0.070;
		SessionLogger::dbg("mention_conf") << "  0.070 headtag!='NNPS'\n";
	}
	Entity *entity = mention->getEntity(docTheory);
	if (entity == 0 || !entity->isGeneric()) {
		score += 0.069;
		SessionLogger::dbg("mention_conf") << "  0.069 entity_is_generic==False\n";
	}
	if (mention->getAtomicHead()->getTag() != Symbol(L"NNP")) {
		score += 0.069;
		SessionLogger::dbg("mention_conf") << "  0.069 headtag!='NNP'\n";
	}
	if (mention->getEntityType().matchesPER()) {
		score += 0.066;
		SessionLogger::dbg("mention_conf") << "  0.066 entity_type=='PER'\n";
	}
	if (mention->getAtomicHead()->getTag() == Symbol(L"NN")) {
		score += 0.054;
		SessionLogger::dbg("mention_conf") << "  0.054 headtag=='NN'\n";
	}
	if (relationMentions(mention, docTheory).size() > 0) {
		score += 0.036;
		SessionLogger::dbg("mention_conf") << "  0.036 in_relation==True\n";
	}

	bool found_saying = false;
	bool found_think = false;
	bool found_have = false;
	std::vector<Proposition*> props = verbPropositions(mention, docTheory);
	BOOST_FOREACH(Proposition* prop, props) {
		if (prop->getPredSymbol() == Symbol(L"saying") ||
			prop->getPredSymbol() == Symbol(L"think") ||
			prop->getPredSymbol() == Symbol(L"have"))
		{
			const Mention *m = prop->getMentionOfRole(Argument::OBJ_ROLE, mention->getMentionSet());
			if (m != 0) {
				while (m->getChild() != 0) {
					m = m->getChild();
				}
				if (m->getIndex() == mention->getIndex()) {
					if (prop->getPredSymbol() == Symbol(L"saying"))
						found_saying = true;
					if (prop->getPredSymbol() == Symbol(L"think"))
						found_think = true;
					if (prop->getPredSymbol() == Symbol(L"have"))
						found_have = true;
				}
			}
		}
	
	}
	if (!found_saying) {
		score += 0.071;
		SessionLogger::dbg("mention_conf") << "  0.071 verb_prop!='<obj>:saying'\n";
	}
	if (!found_think) {
		score += 0.071;
		SessionLogger::dbg("mention_conf") << "  0.071 verb_prop!='<obj>:think'\n";
	}
	if (!found_have) {
		score += 0.071;
		SessionLogger::dbg("mention_conf") << "  0.071 verb_prop!='<obj>:have'\n";
	}

	return std::min(score, 1.0);

}

/**
 *  RelMention Confidence
 *  
 *  [0.370] rhs_entity_subtype==u'Media'
 *  [0.325] lhs_entity_subtype==u'Population-Center'
 *  [0.322] entity_types==u'ORG+FAC'
 *  [0.259] entity_types==u'PER+PER'
 *  [0.259] rhs_entity_type==u'PER'
 *  [0.254] lhs_entity_subtype==u'Government'
 *  [0.239] entity_types==u'FAC+GPE'
 *  [0.231] lhs_entity_type==u'FAC'
 *  [0.220] mention_types=='MentionType.name+MentionType.name'
 *  [0.210] entity_types==u'GPE+GPE'
 *  [0.205] rhs_entity_subtype==u'Group'
 *  [0.183] overlap=='adjacent'
 *  [0.172] mention_types=='MentionType.pron+MentionType.desc'
 *  [0.169] entity_types==u'ORG+GPE'
 *  [0.168] rhs_entity_subtype==u'Nation'
 *  
 *  On heldout data:
 *   
 *   1.000| Prec vs Score (Running avg)    100.0|      Precision vs Rank    :   
 *   0.857|                      .:...:     85.7|                         ..:   
 *   0.714|       .:... ..  :::::::::::     71.4|                  :. . :::::   
 *   0.571|...::.::::::::::::::::::::::     57.1|..:..:..:.::::..::::::::::::   
 *   0.429|::::::::::::::::::::::::::::     42.9|::::::::::::::::::::::::::::   
 *   0.286|::::::::::::::::::::::::::::     28.6|::::::::::::::::::::::::::::   
 *   0.143|::::::::::::::::::::::::::::     14.3|::::::::::::::::::::::::::::   
 *   0.000|::::::::::::::::::::::::::::      0.0|::::::::::::::::::::::::::::   
 *       0.00        0.44            1.0       0.00        0.44            1.0  
 *   
 *   1.000|:          Density              1.000|        Score vs Rank     .:   
 *   0.857|:                               0.857|                         .::   
 *   0.714|:                               0.714|                        ::::   
 *   0.571|:                               0.571|                     .::::::   
 *   0.429|:   .                           0.429|                  ..::::::::   
 *   0.286|:   :                           0.286|                ..::::::::::   
 *   0.143|:   :::  .     . .  .     .     0.143|          ::::::::::::::::::   
 *   0.000|:___::::_:::::::::::::::_:::    0.000|__________::::::::::::::::::   
 *       0.00        0.44            1.0       0.00        0.44            1.0  
 *   
 *       Prec(top 10%): 78%       Gradient: 0.256       Bin Diff: 0.65 (high=good)  
 *       Prec(top 25%): 72%      R-Squared: 0.022     Bin Diff 2: 2.28 (low=good)   
 *       Prec(top 50%): 66%  Rank Gradient: 0.227        bin[-1]: 0.78 (high=good)  
 *       Prec(bot 50%): 56%    x-intercept:-2.154      max score: 60.00             
 *       Prec(bot 25%): 56%    y-intercept: 0.550  unique scores: 53    
 */
double EnglishConfidenceEstimator::calculateConfidenceScore(const RelMention *relMention) const {
	double score = 0;

	const Mention *mention1 = relMention->getLeftMention();
	const Mention *mention2 = relMention->getRightMention();

	SessionLogger::dbg("rel_mention_conf") << "RelMention Confidence Metric:\n";

	if (mention2->getEntitySubtype().getName() == Symbol(L"Media")) {
		score += 0.370;
		SessionLogger::dbg("rel_mention_conf") << "  0.370 rhs_entity_subtype=='Media'\n";
	}
	if (mention1->getEntitySubtype().getName() == Symbol(L"Population-Center")) {
		score += 0.325;
		SessionLogger::dbg("rel_mention_conf") << "  0.325 lhs_entity_subtype=='Population-Center'\n";
	}
	if ((mention1->getEntityType().matchesORG()) && 
		(mention2->getEntityType().matchesFAC())) {
		score += 0.322;
		SessionLogger::dbg("rel_mention_conf") << "  0.322 entity_types=='ORG+FAC'\n";
	}
	if ((mention1->getEntityType().matchesPER()) && 
		(mention2->getEntityType().matchesPER())) {
		score += 0.259;
		SessionLogger::dbg("rel_mention_conf") << "  0.259 entity_types=='PER+PER'\n";
	}
	if (mention2->getEntityType().matchesPER()) {
		score += 0.259;
		SessionLogger::dbg("rel_mention_conf") << "  0.259 rhs_entity_type=='PER'\n";
	}
	if (mention1->getEntitySubtype().getName() == Symbol(L"Government")) {
		score += 0.254;
		SessionLogger::dbg("rel_mention_conf") << "  0.254 lhs_entity_subtype=='Government'\n";
	}
	if ((mention1->getEntityType().matchesFAC()) && 
		(mention2->getEntityType().matchesGPE())) {
		score += 0.239;
		SessionLogger::dbg("rel_mention_conf") << "  0.239 entity_types=='FAC+GPE'\n";
	}
	if (mention1->getEntityType().matchesFAC()) {
		score += 0.231;
		SessionLogger::dbg("rel_mention_conf") << "  0.231 lhs_entity_type=='FAC'\n";
	}	
	if ((mention1->getMentionType() == Mention::NAME) &&
		(mention2->getMentionType() == Mention::NAME)) {
		score += 0.220;
		SessionLogger::dbg("rel_mention_conf") << "  0.220 mention_types=='MentionType.name+MentionType.name'\n";
	}
	if ((mention1->getEntityType().matchesGPE()) && 
		(mention2->getEntityType().matchesGPE())) {
		score += 0.210;
		SessionLogger::dbg("rel_mention_conf") << "  0.210 entity_types=='GPE+GPE'\n";
	}
	if (mention2->getEntitySubtype().getName() == Symbol(L"Group")) {
		score += 0.205;
		SessionLogger::dbg("rel_mention_conf") << "  0.205 rhs_entity_subtype=='Group'\n";
	}
	if (relationArgumentDist(relMention) == 1) {
		score += 0.183;
		SessionLogger::dbg("rel_mention_conf") << "  0.183 overlap=='adjacent'\n";
	}
	if ((mention1->getMentionType() == Mention::PRON) &&
		(mention2->getMentionType() == Mention::DESC)) {
		score += 0.172;
		SessionLogger::dbg("rel_mention_conf") << "  0.172 mention_types=='MentionType.pron+MentionType.desc'\n";
	}
	if ((mention1->getEntityType().matchesORG()) && 
		(mention2->getEntityType().matchesGPE())) {
		score += 0.169;
		SessionLogger::dbg("rel_mention_conf") << "  0.169 entity_types=='ORG+GPE'\n";
	}
	if (mention2->getEntitySubtype().getName() == Symbol(L"Nation")) {
		score += 0.168;
		SessionLogger::dbg("rel_mention_conf") << "  0.168 rhs_entity_subtype=='Nation'\n";
	}
	SessionLogger::dbg("rel_mention_conf") << "RelMention confidence: " << std::min(score, 1.0) << "\n"; 

	return std::min(score, 1.0);
}

/**
 *  Coref Link Confidence
 *
 *  [0.084] entity_subtypes!=u'UNDET+Government'
 *  [0.084] substring==True
 *  [0.084] brandy_confidence!='OTHER_PRON'
 *  [0.084] entity_subtypes!=u'Nation+Nation'
 *  [0.084] entity_subtypes!=u'Commercial+Media'
 *  [0.084] entity_subtypes!=u'Non-Governmental+UNDET'
 *  [0.084] char_dist>0
 *  [0.084] entity_subtypes!=u'UNDET+Population-Center'
 *  [0.084] mention_types!='MentionType.pron+MentionType.name'
 *  [0.084] entity_subtypes!=u'Group+UNDET'
 *  [0.084] mention_types!='MentionType.desc+MentionType.name'
 *  [0.080] entity_subtypes==u'Individual+Individual'
 *  
 *  On heldout data:
 *  
 *        Prec vs Score (Running avg)                                          
 *  1.000|                         ..:    100.0|      Precision vs Rank    :   
 *  0.857|                      ..::::     85.7|                          ::   
 *  0.714|                 ...::::::::     71.4|                     .::::::   
 *  0.571|......::::::::::::::::::::::     57.1|.:.:.:.::.::::::::::::::::::   
 *  0.429|::::::::::::::::::::::::::::     42.9|::::::::::::::::::::::::::::   
 *  0.286|::::::::::::::::::::::::::::     28.6|::::::::::::::::::::::::::::   
 *  0.143|::::::::::::::::::::::::::::     14.3|::::::::::::::::::::::::::::   
 *  0.000|::::::::::::::::::::::::::::      0.0|::::::::::::::::::::::::::::   
 *      0.00        0.44            1.0       0.00        0.44            1.0  
 *  
 *                  Density                                                    
 *  1.000|          :                     1.000|        Score vs Rank      :   
 *  0.857|          :                     0.857|                      ....::   
 *  0.714|     :    :                     0.714|                     :::::::   
 *  0.571|     :    :                     0.571|                  ::::::::::   
 *  0.429|     :    :          :          0.429|        ..........::::::::::   
 *  0.286|     :    :     :    :          0.286|       .::::::::::::::::::::   
 *  0.143|     :    :     :    :          0.143|.:::::::::::::::::::::::::::   
 *  0.000|:____:____:_____:____:_____:    0.000|::::::::::::::::::::::::::::   
 *      0.00        0.44            1.0       0.00        0.44            1.0  
 *  
 *      Prec(top 10%): 82%       Gradient: 0.353       Bin Diff: 0.69 (high=good)  
 *      Prec(top 25%): 76%      R-Squared: 0.029     Bin Diff 2: 2.17 (low=good)   
 *      Prec(top 50%): 67%  Rank Gradient: 0.245        bin[-1]: 0.82 (high=good)  
 *      Prec(bot 50%): 57%    x-intercept:-1.318      max score: 523.31            
 *      Prec(bot 25%): 57%    y-intercept: 0.465  unique scores: 6                 
 */

double EnglishConfidenceEstimator::calculateLinkConfidenceScore(const Mention *mention, const Mention *canonical, const DocTheory *docTheory) const {
	double score = 0;

	if (canonical->getMentionType() != Mention::NAME) 
		return score;

	SessionLogger::dbg("link_conf") << "Link Confidence Metric:\n";

	std::wstring mention_text = mention->getEDTHead()->toTextString();
	std::wstring canonical_text = canonical->getEDTHead()->toTextString();

	SessionLogger::dbg("link_conf") << "Linking '" << mention_text << "' to '" << canonical_text << "'\n";

	if ((mention == canonical) || mention_text.compare(canonical_text) == 0) {
		SessionLogger::dbg("link_conf") << "Link confidence: 1.0\n";
		return 1.0;
	}

	MentionConfidenceAttribute brandyConf = 
		MentionConfidence::determineMentionConfidence(docTheory, docTheory->getSentenceTheory(mention->getSentenceNumber()), mention);

	std::vector<const Mention*> canonicalMentions = 
		getEquivalentMentions(canonical, docTheory->getEntitySet());
	
	if (!(mention->getEntitySubtype().getName() == Symbol(L"UNDET") &&
		canonical->getEntitySubtype().getName() == Symbol(L"Government")))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 entity_subtypes!='UNDET+Government'\n";
	}
	if (canonical_text.find(mention_text) != std::wstring::npos) {
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 substring==True\n";
	}
	if (brandyConf != MentionConfidenceStatus::OTHER_PRON) {
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 brandy_confidence!='OTHER_PRON'\n";
	}
	if (!(mention->getEntitySubtype().getName() == Symbol(L"Nation") &&
		canonical->getEntitySubtype().getName() == Symbol(L"Nation")))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 entity_subtypes!='Nation+Nation'\n";
	}
	if (!(mention->getEntitySubtype().getName() == Symbol(L"Commercial") &&
		canonical->getEntitySubtype().getName() == Symbol(L"Media")))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 entity_subtypes!='Commercial+Media'\n";
	}
	if (!(mention->getEntitySubtype().getName() == Symbol(L"Non-Governmental") &&
		canonical->getEntitySubtype().getName() == Symbol(L"UNDET")))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 entity_subtypes!='Non-Governmental+UNDET'\n";
	}
	if (getMinCharacterDistance(mention, canonicalMentions, docTheory) > 0) {
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 char_dist>0\n";
	}
	if (!(mention->getEntitySubtype().getName() == Symbol(L"UNDET") &&
		canonical->getEntitySubtype().getName() == Symbol(L"Population-Center")))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 entity_subtypes!='UNDET+Population-Center'\n";
	}
	if (!(mention->getMentionType() == Mention::PRON &&
		canonical->getMentionType() == Mention::NAME))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 mention_types!='MentionType.pron+MentionType.name'\n";
	}
	if (!(mention->getEntitySubtype().getName() == Symbol(L"Group") &&
		canonical->getEntitySubtype().getName() == Symbol(L"UNDET")))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 entity_subtypes!='Group+UNDET'\n";
	}
	if (!(mention->getMentionType() == Mention::DESC &&
		canonical->getMentionType() == Mention::NAME))
	{
		score += 0.084;
		SessionLogger::dbg("link_conf") << "  0.084 mention_types!='MentionType.desc+MentionType.name'\n";
	}
	if ((mention->getEntitySubtype().getName() == Symbol(L"Individual") &&
		canonical->getEntitySubtype().getName() == Symbol(L"Individual")))
	{
		score += 0.080;
		SessionLogger::dbg("link_conf") << "  0.080 entity_subtypes=='Individual+Individual'\n";
	}

	SessionLogger::dbg("link_conf") << "Link confidence: " << std::min(score, 1.0) << "\n"; 

	return std::min(score, 1.0);
}

bool EnglishConfidenceEstimator::relationHasNestedArgs(const RelMention *relMention) const {
	const Mention *mention1 = relMention->getLeftMention();
	const Mention *mention2 = relMention->getRightMention();

	int lhs_start = mention1->getNode()->getStartToken();
	int rhs_start = mention2->getNode()->getStartToken();
	int lhs_end = mention1->getNode()->getEndToken();
	int rhs_end = mention2->getNode()->getEndToken();

	return (((rhs_start <= lhs_start) && (lhs_start <= rhs_end+1)) || 
			((lhs_start <= rhs_start) && (rhs_start <= lhs_end+1)));
}

int EnglishConfidenceEstimator::relationArgumentDist(const RelMention *relMention) const {
	if (relationHasNestedArgs(relMention))
		return 0;

	const Mention *mention1 = relMention->getLeftMention();
	const Mention *mention2 = relMention->getRightMention();

	int lhs_start = mention1->getNode()->getStartToken();
	int rhs_start = mention2->getNode()->getStartToken();
	int lhs_end = mention1->getNode()->getEndToken();
	int rhs_end = mention2->getNode()->getEndToken();

	return std::min(std::abs((rhs_end+1) - lhs_start),
					std::abs((lhs_end+1) - rhs_start));
}

std::vector<Proposition*> EnglishConfidenceEstimator::verbPropositions(const Mention* mention, const DocTheory *docTheory) const {
	std::vector<Proposition*> results; 
	const Mention *parent = getParentMention(mention);
	PropositionSet *propSet = docTheory->getSentenceTheory(mention->getSentenceNumber())->getPropositionSet();
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);
		if (prop->getPredType() == Proposition::VERB_PRED) {
			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getType() == Argument::MENTION_ARG) {
					int arg_ment_idx = arg->getMentionIndex();
					if (mention->getIndex() == arg_ment_idx ||
						(parent != 0 && parent->getIndex() == arg_ment_idx))
					{
						results.push_back(prop);
						break;
					}
				}
			}
		}
	}
	return results;
}

const Mention* EnglishConfidenceEstimator::getParentMention(const Mention *mention) const {
	const Mention *parent = mention->getParent();
	while (parent != 0 && parent->getParent() != 0) {
		parent = parent->getParent();
	}
	return parent;
}

std::vector<RelMention*> EnglishConfidenceEstimator::relationMentions(const Mention* mention, const DocTheory *docTheory) const {
	std::vector<RelMention*> results;
	RelMentionSet *relMentionSet = docTheory->getSentenceTheory(mention->getSentenceNumber())->getRelMentionSet();
	for (int i = 0; i < relMentionSet->getNRelMentions(); i++) {
		RelMention *relMention = relMentionSet->getRelMention(i);
		if (relMention->getLeftMention()->isIdenticalTo(*mention) ||
			relMention->getRightMention()->isIdenticalTo(*mention))
		{
			results.push_back(relMention);
		}
	}
	return results;
}

bool EnglishConfidenceEstimator::isSpeakerMention(const Mention* mention, const DocTheory *docTheory) const {
	return docTheory->isSpeakerSentence(mention->getSentenceNumber());
}


const Mention* EnglishConfidenceEstimator::getBestMention(const Entity* entity, const DocTheory *docTheory) const {
	const Mention *bestMention = 0;
	double best_score = -DBL_MAX;
	for (int i = 0; i < entity->getNMentions(); i++) {
		const Mention *ment = docTheory->getEntitySet()->getMention(entity->getMention(i));
		double score = mentionScore(ment, docTheory);
		if (score > best_score) {
			best_score = score;
			bestMention = ment;
		}
	}	
	return bestMention;
}

double EnglishConfidenceEstimator::mentionScore(const Mention* mention, const DocTheory *docTheory) const {
	double score = 0;
	
	if (isSpeakerMention(mention, docTheory)) score -= 20;
	if (mention->getMentionType() == Mention::NAME) score += 10;
	if (mention->getMentionType() == Mention::DESC) score += 5;

	std::vector<const SynNode*> terminals;
	mention->getEDTHead()->getAllTerminalNodes(terminals);
	size_t num_words = terminals.size();
	switch(num_words) {
		case 3:
			score += 5;
			break;
		case 2:
			score += 4;
			break;
		case 4:
			score += 3;
			break;
		case 5:
			score += 2;
			break;
		case 6:
			score += 1;
			break;
		default:
			break;
	}
	if (num_words > 8) score -= 1;
	return score;
}

std::vector<const Mention*> EnglishConfidenceEstimator::getEquivalentMentions(const Mention* mention, const EntitySet *entitySet) const {
	std::vector<const Mention*> results;
	std::wstring mention_text = mention->getEDTHead()->toTextString();
	const Entity *entity = entitySet->getEntityByMention(mention->getUID());
	for (int i = 0; i < entity->getNMentions(); i++) {
		const Mention *oth = entitySet->getMention(mention->getUID());
		std::wstring oth_text = oth->getEDTHead()->toTextString();
		if (mention_text.compare(oth_text) == 0) {
			results.push_back(oth);
		}
	}
	return results;
}

int EnglishConfidenceEstimator::getMinCharacterDistance(const Mention *mention, std::vector<const Mention*> &mentions, const DocTheory *docTheory) const {
	int min = INT_MAX;
	BOOST_FOREACH(const Mention *m, mentions) {
		int d = getCharacterDistance(mention, m, docTheory);
		if (d < min)
			min = d;
	}
	return min;
}

int EnglishConfidenceEstimator::getCharacterDistance(const Mention* mention1, const Mention* mention2, const DocTheory *docTheory) const {
	const SynNode *node1 = mention1->getEDTHead();
	const SynNode *node2 = mention2->getEDTHead();

	TokenSequence *tokens1 = docTheory->getSentenceTheory(mention1->getSentenceNumber())->getTokenSequence();
	TokenSequence *tokens2 = docTheory->getSentenceTheory(mention2->getSentenceNumber())->getTokenSequence();

	int start1 = tokens1->getToken(node1->getStartToken())->getStartEDTOffset().value();
	int start2 = tokens2->getToken(node2->getStartToken())->getStartEDTOffset().value();
	int end1 = tokens1->getToken(node1->getEndToken())->getEndEDTOffset().value() + 1;
	int end2 = tokens2->getToken(node2->getEndToken())->getEndEDTOffset().value() + 1;

	return std::min(std::abs(end1 - start2), std::abs(end2 - start1));	
}

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ParamReader.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/relations/ar_RelationFinder.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/wordClustering/WordClusterClass.h"

int ArabicRelationUtilities::getRelationCutoff() {
	static bool init = false;
	static int relation_cutoff;
	if (!init) {
		relation_cutoff = ParamReader::getRequiredIntParam("relation_mention_dist_cutoff");
		init = true;
		if (relation_cutoff < 0) {
			throw UnexpectedInputException("ArabicRelationUtilities::getRelationCutoff()",
				"Parameter 'relation_mention_dist_cutoff' must be greater than or equal to 0");
		}
	}
	return relation_cutoff;
}

int ArabicRelationUtilities::getAllowableRelationDistance() {
	static bool init = false;
	static int allow_relations_within_distance;
	if (!init) {
		allow_relations_within_distance = ParamReader::getRequiredIntParam("allow_relations_within_distance");
		init = true;
		if (allow_relations_within_distance <= 0) {
			throw UnexpectedInputException("ArabicRelationUtilities::getAllowableRelationDistance()",
				"Parameter 'allow_relations_within_distance' must be greater than 0");
		}
	}
	return allow_relations_within_distance;
}

std::vector<bool> ArabicRelationUtilities::identifyFalseOrHypotheticalProps(const PropositionSet *propSet,
														 const MentionSet *mentionSet) 
{
	return std::vector<bool>(propSet->getNPropositions(), false);
}

void ArabicRelationUtilities::fillClusterArray(const Mention* ment, int* clust){
	Symbol hw = ment->getNode()->getHeadWord();
	Mention::Type mtype = ment->getMentionType();
	WordClusterClass wc1(hw);
	for(int k =0; k<4; k++){
		clust[k] = 0;
	}
	if(mtype != Mention::PRON){
		clust[0] = wc1.c8();
		clust[1] = wc1.c12();
		clust[2] = wc1.c16();
		clust[3] = wc1.c20();
	}



}
bool ArabicRelationUtilities::validRelationArgs(Mention *m1, Mention *m2) {

	// This uses the same cutoff parameter as is used to distinguish between FAR/CLOSE in a number
	// of other features. This is probably not what we want, so I am commenting it out and
	// have added a new parameter below. 
	//if(ArabicRelationUtilities::distIsLessThanCutoff(m1, m2)){
	//	return true;
	//}

	// We allow all mention pairs that are within distance X. 
	// (We might allow some others if they meet the following criteria,
	//  but these we definitely allow.)
	if (calcMentionDist(m1, m2) < getAllowableRelationDistance()) {
		return true;
	}	

	const SynNode *chunk1 = findNPChunk(m1->getNode());
	const SynNode *chunk2 = findNPChunk(m2->getNode());

	if (chunk1 == chunk2)
		return true;

	const SynNode *parent = chunk1->getParent();
	int i;
	for (i = 0; i < parent->getNChildren(); i++) {
		if (parent->getChild(i) == chunk1 ||
			parent->getChild(i) == chunk2)
		{
			i++;
			break;
		}
	}
	for (int j = i; j < parent->getNChildren(); j++) {
		if (parent->getChild(j) == chunk1 ||
			parent->getChild(j) == chunk2)
		{
			return true;
		}
		//if (parent->getChild(j)->getTag() != ArabicSTags::PREP &&
		if (parent->getChild(j)->getTag() != ArabicSTags::IN &&
			!LanguageSpecificFunctions::isNPtypeLabel(parent->getChild(j)->getTag())){
				if((parent->getChild(j)->getTag() == ArabicSTags::LRB) ||
					(parent->getChild(j)->getTag() == ArabicSTags::RRB))
				{
					//std::cout<<"!!!!invalid relation: !   "<<std::endl;
					//m1->getNode()->dump(std::cout, 5);
					//std::cout<<std::endl;
					//m2->getNode()->dump(std::cout, 5);
					//std::cout<<std::endl;
				}
				return false;
			}

		/*if (parent->getChild(j)->getTag() == ArabicSTags::VERB ||
			parent->getChild(j)->getTag() == ArabicSTags::VERB_IMPERATIVE ||
			parent->getChild(j)->getTag() == ArabicSTags::VERB_IMPERFECT ||
			parent->getChild(j)->getTag() == ArabicSTags::VERB_PERFECT)
			return false;*/
	}

	return true;
}


const SynNode *ArabicRelationUtilities::findNPChunk(const SynNode *node) {
	if (node->getParent() == 0) {
		if (LanguageSpecificFunctions::isNPtypeLabel(node->getTag()) ||
			ArabicSTags::isPronoun(node->getTag()))
			return node;
		else
			return 0;
	}

	const SynNode *parent = node->getParent();
	if (parent->getParent() == 0) {
	   //(LanguageSpecificFunctions::isNPtypeLabel(node->getTag()) || ArabicSTags::isPronoun(node->getTag()))) {
		return node;
	}
	else {
		return findNPChunk(parent);
	}
}


int ArabicRelationUtilities::getMentionStartToken(const Mention *m1){
	int start_ment1 = -1;
	if(m1->mentionType == Mention::NAME){
		start_ment1 = m1->getNode()->getStartToken();
	}
	else{
		if(m1->getHead() != 0){
			start_ment1 = m1->getHead()->getStartToken();
		}
		else if(m1->getNode() != 0){
			if(m1->getNode()->getHead() != 0){
				start_ment1 = m1->getNode()->getHead()->getStartToken();
			}
			else{
				start_ment1 = m1->getNode()->getStartToken();
			}
		}
	}
	return start_ment1;
}
int ArabicRelationUtilities::getMentionEndToken(const Mention *m1){
	int end_ment1 = -1;
	if(m1->mentionType == Mention::NAME){
		end_ment1 = m1->getNode()->getEndToken();
	}
	else{
		if(m1->getHead() != 0){
			end_ment1 = m1->getHead()->getEndToken();
		}
		else if(m1->getNode() != 0){
			if(m1->getNode()->getHead() != 0){
				end_ment1 = m1->getNode()->getHead()->getEndToken();
			}
			else{
				end_ment1 = m1->getNode()->getEndToken();
			}
		}
	}
	return end_ment1;
}
int ArabicRelationUtilities::calcMentionDist(const Mention *m1, const Mention *m2){
	int start_ment1;
	int end_ment1;
	int start_ment2;
	int end_ment2;
	start_ment1 = getMentionStartToken(m1);
	end_ment1 = getMentionEndToken(m1);
	start_ment2 = getMentionStartToken(m2);
	end_ment2 = getMentionEndToken(m2);	
	if((start_ment1 < 0) || (end_ment1 < 0) || (start_ment2 < 0) || (end_ment2 < 0) ){
		return getRelationCutoff() + 50;
	}

	int dist = end_ment1 - start_ment2;
	if(dist  < 0){
		dist = end_ment2 - start_ment1;
	}
	return dist;
}
bool ArabicRelationUtilities::distIsLessThanCutoff(const Mention *m1, const Mention *m2){
	int dist = calcMentionDist(m1, m2);
	if(dist < getRelationCutoff()){
		return true;
	}
	else{
		return false;
	}
}
bool ArabicRelationUtilities::isValidRelationEntityTypeCombo(Symbol validation_type, 
										   const Mention* m1, const Mention* m2, Symbol relType)
{
	if(wcscmp(validation_type.to_string(), L"NONE") == 0){
		return true;
	}
	if(wcscmp(validation_type.to_string(), L"2005") == 0){
		if(is2005ValidRelationEntityTypeCombo(m1, m2, relType)) return true;
		if(is2005ValidRelationEntityTypeCombo(m2, m1, relType)) return true;
		return false;
	}
	if(wcscmp(validation_type.to_string(), L"2005_ORDERED") == 0){
		if(is2005ValidRelationEntityTypeCombo(m1, m2, relType)) return true;
		return false;
	}
	return true;
}
bool ArabicRelationUtilities::is2005ValidRelationEntityTypeCombo(const Mention* arg1, const Mention* arg2,
														   Symbol relType )
{
	Symbol relCat = RelationConstants::getBaseTypeSymbol(relType);
	Symbol relSubtype = RelationConstants::getSubtypeSymbol(relType);
	Symbol PER = EntityType::getPERType().getName();
	Symbol ORG = EntityType::getORGType().getName();
	Symbol LOC = EntityType::getLOCType().getName();
	Symbol FAC = EntityType::getFACType().getName();
	Symbol GPE = EntityType::getGPEType().getName();
	Symbol WEA = Symbol(L"WEA");
	Symbol VEH = Symbol(L"VEH");

	Symbol arg1_type = arg1->getEntityType().getName();
	Symbol arg1_subtype = EntitySubtype::getUndetType().getName();
	if(arg1->getEntitySubtype().isDetermined()){
		Symbol arg1_subtype = arg1->getEntitySubtype().getName();
	}
	Symbol arg2_type = arg2->getEntityType().getName();
	Symbol arg2_subtype = EntitySubtype::getUndetType().getName();
	if(arg2->getEntitySubtype().isDetermined()){
		Symbol arg2_subtype = arg1->getEntitySubtype().getName();
	}
	if(relCat == Symbol(L"PER-SOC")){
		if((arg1_type ==  PER) && (arg2_type == PER)){
			return true;
		}
		return false;
	}
	if(relType == Symbol(L"PHYS.Located")){
		//Arabic guidelines allow arg 1 to be a wea/veh English guidelines do not??
		//if((arg1_type == PER) || (arg1_type == VEH) || (arg1_type == WEA)){
		if(arg1_type == PER){
			if((arg2_type == GPE) || (arg2_type == LOC) || (arg2_type == FAC)){
					return true;
				}
		}
		return false;
	}
	if(relType == Symbol(L"PHYS.Near")){
		if((arg1_type== PER)|| (arg1_type == GPE) || (arg1_type == LOC) || 
			(arg1_type == FAC))
		{
			if((arg2_type == GPE) || (arg2_type == LOC) || (arg2_type == FAC)){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"PART-WHOLE.Geographical")){
		if((arg1_type == GPE) || (arg1_type == LOC) ||(arg1_type == FAC))
		{
			if((arg2_type == GPE) || (arg2_type == LOC) || (arg2_type == FAC)){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"PART-WHOLE.Subsidiary")){
		if((arg1_type == ORG) )
		{
			if((arg2_type == GPE) || (arg2_type == ORG)){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"PART-WHOLE.Artifact")){
		if(arg1_type != arg2_type){
			return false;
		}
		if((arg1_type == VEH) ||(arg1_type == WEA) )
		{
			if((arg2_type == VEH) ||(arg2_type == WEA) ){
				return true;
			}
		}
		return false;
	}		
	if(relType == Symbol(L"ORG-AFF.Employment")){
		if(arg1_type == PER){
			if( (arg2_type == GPE) || (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"ORG-AFF.Ownership")){
		if(arg1_type == PER){
			if( arg2_type == ORG){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"ORG-AFF.Founder")){
		if((arg1_type == PER) || (arg1_type == ORG)){
			if( (arg2_type == GPE) || (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"ORG-AFF.Student-Alum")){
		if(arg1_type == PER){
			if( (arg2_type == ORG) ){
				if(arg2_subtype == EntitySubtype::getUndetType().getName()){
					//subtype hasn't been determined, so keep the relation
					return true;
				}
				if(arg2_subtype == Symbol(L"Educational")){
					return true;
				}
			}
		}
		return false;
	}
	if(relType == Symbol(L"ORG-AFF.Sports-Affiliation")){
		if(arg1_type == PER){
			if( (arg2_type == ORG) ){
				if(arg2_subtype == EntitySubtype::getUndetType().getName()){
					//subtype hasn't been determined, so keep the relation
					return true;
				}
				if(arg2_subtype == Symbol(L"Sports")){
					return true;
				}
			}
		}
		return false;
	}

	if(relType == Symbol(L"ORG-AFF.Investor-Shareholder")){
		if((arg1_type == PER) || (arg1_type == ORG) || (arg1_type == GPE)){
			if( (arg2_type == GPE) || (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"ORG-AFF.Membership")){
		if((arg1_type == PER) || (arg1_type == ORG) || (arg1_type == GPE)){
			if( (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"ART.User-Owner-Inventor-Manufacturer")){
		if((arg1_type == PER) || (arg1_type == ORG) || (arg1_type == GPE)){
			if( (arg2_type == WEA) || (arg2_type == VEH) || (arg2_type == FAC) ){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity")){
		if((arg1_type == PER) ){
			if( (arg2_type == ORG)|| (arg2_type == GPE) || (arg2_type == LOC) ||
				(arg2_type == PER)){
				return true;
			}
		}
		return false;
	}
	if(relType == Symbol(L"GEN-AFF.Org-Location")){
		if((arg1_type == ORG) ){
			if( (arg2_type == GPE) || (arg2_type == LOC) ){
				return true;
			}
		}
		return false;
	}
	// ignore metonymy for now
	return true;
}


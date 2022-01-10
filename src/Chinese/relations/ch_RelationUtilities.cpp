// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Chinese/relations/ch_RelationUtilities.h"
#include "Chinese/relations/ch_RelationFinder.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntityType.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/maxent/OldMaxEntEvent.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Chinese/relations/ch_OldMaxEntRelationFinder.h"

Symbol ChineseRelationUtilities::SET_SYM = Symbol(L":SET");
Symbol ChineseRelationUtilities::COMP_SYM = Symbol(L":COMP");

std::vector<bool> ChineseRelationUtilities::identifyFalseOrHypotheticalProps(const PropositionSet *propSet,
														 const MentionSet *mentionSet) 
{
	std::vector<bool> isBad(propSet->getNPropositions(), false);	

	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);

		// negative propositions
		if (prop->getNegation() != NULL)
			isBad[prop->getIndex()] = true;

		// modal is in some set of possibility modals
		const SynNode *modal = prop->getModal();
		if (false)
			isBad[prop->getIndex()] = true;
		

		for (int l = 0; l < prop->getNArgs(); l++) {
			Argument *arg = prop->getArg(l);
			// argument is a conditional prop
			//if (arg->getType() == Argument::PROPOSITION_ARG && arg.getRoleSym() == conditional) 
			// 	  isBad[prop->getIndex()] = true;
			// bad prop with nested prop
			if (isBad[prop->getIndex()] &&
				arg->getType() == Argument::PROPOSITION_ARG &&
				(arg->getRoleSym() == Argument::OBJ_ROLE 
				//|| arg.getRoleSym() == Symbol(L"that")
				)) {
					isBad[arg->getProposition()->getIndex()] = true;

			}
		}	

	}

	return isBad;
}

bool ChineseRelationUtilities::coercibleToType(const Mention *ment, Symbol type) {
	return false;
}

Symbol ChineseRelationUtilities::stemPredicate(Symbol word, Proposition::PredType predType) {
	if (predType == Proposition::COMP_PRED)
		return Symbol(L":COMP");
	else if (predType == Proposition::SET_PRED)
		return Symbol(L":SET");
	else
		return word;
}


UTF8OutputStream& ChineseRelationUtilities::getDebugStream() {
	return OldMaxEntRelationFinder::_debugStream;
}

bool ChineseRelationUtilities::debugStreamIsOn() {
	return OldMaxEntRelationFinder::DEBUG;
}

bool ChineseRelationUtilities::isValidRelationEntityTypeCombo(Symbol validation_type, 
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

bool ChineseRelationUtilities::is2005ValidRelationEntityTypeCombo(const Mention* arg1, const Mention* arg2,
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

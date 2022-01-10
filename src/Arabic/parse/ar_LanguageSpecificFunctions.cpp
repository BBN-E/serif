// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/parse/ar_LanguageSpecificFunctions.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Arabic/common/ar_NationalityRecognizer.h"

bool ArabicLanguageSpecificFunctions::POSTheoryContainsNounPL(const PartOfSpeech* pos){
	if(posTheoryFindMatching2Substrings(pos, L"NOUN", L"PL")){
		return true;
	}
	if(posTheoryFindMatching2Substrings(pos, L"NOUN", L"DU")){
		return true;
	}
	return false;
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsNounSG(const PartOfSpeech* pos){
	if(posTheoryFindMatching2Substrings(pos, L"NOUN", L"SG")){
		return true;
	}
	return false;
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsNounAmbiguousNumber(const PartOfSpeech* pos){
	if(posTheoryFindMatchingSubstring(pos, L"NOUN_PROP")){
		return true;
	}
	if(posTheoryFindMatchingSubstring(pos, L"NOUN")){
		if(posTheoryFindMatchingSubstring(pos, L"SG")){
			return false;
		}
		if(posTheoryFindMatchingSubstring(pos, L"DU")){
			return false;
		}
		if(posTheoryFindMatchingSubstring(pos, L"PL")){
			return false;
		}
		return true;
	}	
	return false;
}


bool ArabicLanguageSpecificFunctions::POSTheoryContainsNounMasc(const PartOfSpeech* pos){
	if(posTheoryFindMatching2Substrings(pos, L"NOUN", L"MASC")){
		return true;
	}
	return false;
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsNounFem(const PartOfSpeech* pos){
	if(posTheoryFindMatching2Substrings(pos, L"NOUN", L"FEM")){
		return true;
	}
	return false;
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsNounAmbiguousGender(const PartOfSpeech* pos){
	if(posTheoryFindMatchingSubstring(pos, L"NOUN")){
		if(posTheoryFindMatchingSubstring(pos, L"MASC")){
			return false;
		}
		if(posTheoryFindMatchingSubstring(pos, L"FEM")){
			return false;
		}		
		return true;
	}
	return false;
}
bool ArabicLanguageSpecificFunctions::posTheoryFindMatching2Substrings(const PartOfSpeech* pos, const wchar_t* str1,
		const wchar_t* str2)
{
	for(int i=0; i< pos->getNPOS(); i++){
		Symbol tag = pos->getLabel(i);
		const wchar_t* tagstr = tag.to_string();
		if(wcsstr(tagstr, str1) != 0){
			if(wcsstr(tagstr, str2) != 0){
				return true;
			}
		}
	}
	return false;
	};
bool ArabicLanguageSpecificFunctions::posTheoryFindMatchingSubstring(const PartOfSpeech* pos, const wchar_t* str1)
{
	for(int i=0; i< pos->getNPOS(); i++){
		Symbol tag = pos->getLabel(i);
		const wchar_t* tagstr = tag.to_string();
		if(wcsstr(tagstr, str1) != 0){
			return true;
		}
	}
	return false;
};

void ArabicLanguageSpecificFunctions::fix2004MentionSet(MentionSet* mention_set, const TokenSequence* tok){
//	std::cout<<"Call fix2004 mentions"<<std::endl;
	//make Mrs., Mr., Ms.  per/nom
	Symbol head = ArabicSymbol(L"r}ys");
	Symbol minster = ArabicSymbol(L"wzrA'");
	Symbol the_minster = ArabicSymbol(L"AlwzrA'");

	Symbol state = ArabicSymbol(L"Aldwlp");
	Symbol republic = ArabicSymbol(L"Aljmhwryp");
	Symbol government = ArabicSymbol(L"AlHkwmp");
	Symbol person_titles[12];
	person_titles[0] = ArabicSymbol(L"Alsyd");
	person_titles[1] = ArabicSymbol(L"Alsydp");
	person_titles[2] = ArabicSymbol(L"AlsydAn");
	person_titles[3] = ArabicSymbol(L"Alsydyn");
	person_titles[5]= ArabicSymbol(L"AlsAdp");
	person_titles[6]= ArabicSymbol(L"AlsydtAn");
	person_titles[7]= ArabicSymbol(L"Alsydtyn");
	person_titles[8]= ArabicSymbol(L"AlsydAt");
	person_titles[9]= ArabicSymbol(L"AlAnstyn");
	person_titles[10]= ArabicSymbol(L"AlAnstAn");
	person_titles[11]= ArabicSymbol(L"AlAnsAt");
	std::vector<int> changed_ids;
	int i, j;
	for(i =0; i< mention_set->getNMentions(); i++){
		Mention* thisment = mention_set->getMention(i);
		if(!((thisment->getEntityType() == EntityType::getOtherType() )
			|| (thisment->getEntityType() == EntityType::getUndetType())))
		{
			continue;
		}
		Symbol hw = thisment->getNode()->getHeadWord();
		int token = thisment->getNode()->getHeadPreterm()->getStartToken();
		//std::cout<<"*** "<<hw.to_debug_string()<<" "<<token
		//	<<" "<<thisment->getEntityType().getName().to_debug_string()<<std::endl;
		if(token > 0){
			if( (hw == minster)  || (hw == the_minster) ){
				if(tok->getToken(token-1)->getSymbol() == head){
					thisment->setEntityType(EntityType::getPERType());
					thisment->mentionType = Mention::DESC;
					changed_ids.push_back(thisment->getIndex());
//					std::cout<<"fixing minster in head of ministers mention"<<std::endl;
//					thisment->dump(std::cout);
//					std::cout<<std::endl;
				}				
			}
			if( (hw == state)  || (hw == republic) || (head == government) ){
				if(tok->getToken(token-1)->getSymbol() == head){
					thisment->setEntityType(EntityType::getGPEType());
					thisment->mentionType = Mention::DESC;
					changed_ids.push_back(thisment->getIndex());
//					std::cout<<"fixing state/republic/government in head of state mention"<<std::endl;
//					thisment->dump(std::cout);
//					std::cout<<std::endl;
				}				
			}
		}
		for(j =0; j<12; j++){
			if(hw == person_titles[j]){
				thisment->setEntityType(EntityType::getPERType());
				thisment->mentionType = Mention::DESC;
				changed_ids.push_back(thisment->getIndex());
//					std::cout<<"fixing Mr/Mrs/Ms"<<std::endl;
//					thisment->dump(std::cout);
//					std::cout<<std::endl;
			}
		}


	}
}
/*Nationality words (American, French, etc.) are labeled as GPE-NAME in BBN name training
for ACE 2005 they become PER-NOM in certain cases- the examples (from Jawad) are

-Quantifier + Nationality => Nationality is PER (Some Americans)
-in/to/for +Nationality (w/o def article) => Nationality is PER (for Americans....)
-PerNom + Nationality => Nationality is GPE (President American)
-PerNom + Nationality + PersonName => Nationality is a GPE (President American George Bush)
-Nationality + PersonName => Nationality is a PER (American George Bush)
-otherwise nationality is a GPE
these rules probably don't cover all cases, but they're better than nothing

this function applies these rules to the mention set, it should be called after descriptor recognintion
on a sentence
*/
void ArabicLanguageSpecificFunctions::fix2005Nationalities(MentionSet* mention_set){
	int per_name_ment[1000];
	int n_per_name = 0;
	int per_nom_ment[1000];
	int n_per_nom = 0;
	int i, k;
	k = 0;
	
	Symbol quantifiers[3];
	quantifiers[0] = ArabicSymbol(L"bEd");	//some
	quantifiers[1] = ArabicSymbol(L"[y");	
	quantifiers[1] = ArabicSymbol(L"mEZm");
	Symbol ar_b = ArabicSymbol(L"b"); //in (in Americans -> PER)
	Symbol ar_l = ArabicSymbol(L"l");	//to/for (to Americans ->PER)



//don't print if there's no nationalities
	bool foundnat = false;
	for(i = 0; i< mention_set->getNMentions(); i++){
		if(!mention_set->getMention(i)->getEntityType().matchesGPE())
			continue;
		if(mention_set->getMention(i)->getMentionType() != Mention::NAME )
			continue;
		
		Symbol hw = mention_set->getMention(i)->getNode()->getHeadWord();
		if(ArabicNationalityRecognizer::isPossibleNationalitySymbol(hw)){
			foundnat = true;
			break;
		}
	}
	if(!foundnat)
		return;
///

//	std::cout<<std::endl;
	for(i = 0; i< mention_set->getNMentions(); i++){
		Mention* ment = mention_set->getMention(i);
		if(ment->getEntityType().matchesPER() && ment->getMentionType() == Mention::NAME){
			//std::cout<<"PerName: "<<ment->getNode()->getStartToken()<< " "<< ment->getNode()->getEndToken()<<std::endl;
			per_name_ment[n_per_name++] = i;
		}
	}
//	std::cout<<std::endl;
	for(i = 0; i< mention_set->getNMentions(); i++){
		Mention* ment = mention_set->getMention(i);
		if(ment->getEntityType().matchesPER() && ment->getMentionType() == Mention::DESC){
			//std::cout<<"PerDesc: "<<ment->getNode()->getStartToken()<< " "<< ment->getNode()->getEndToken()<<std::endl;
			per_nom_ment[n_per_nom++] = i;
		}
	}
	for(i = 0; i< mention_set->getNMentions(); i++){
		if(!mention_set->getMention(i)->getEntityType().matchesGPE())
			continue;
		if(mention_set->getMention(i)->getMentionType() != Mention::NAME )
			continue;
		
		Symbol hw = mention_set->getMention(i)->getNode()->getHeadWord();
		if(ArabicNationalityRecognizer::isPossibleNationalitySymbol(hw))
		{
//			std::cout<<"\nfound nationality: "<<hw.to_debug_string()<<" from: "<<
//				mention_set->getMention(i)->getNode()->getStartToken()<<" to "<<
//				mention_set->getMention(i)->getNode()->getEndToken()<<"    ";
//			mention_set->getMention(i)->dump(std::cout);
//			std::cout<<std::endl;
			//look for Quantifier
			const SynNode* prev_term = 
				mention_set->getMention(i)->getNode()->getHeadPreterm()->getHead()->getPrevTerminal();
			bool match_quant = false;
			bool match_indef_after_prep = false;
			if(prev_term != 0){
				Symbol prev_word = prev_term->getTag();
				for(int k = 0; k<3; k++){
					if(prev_word == quantifiers[k]){
						match_quant = true;
					}
				}
				const wchar_t* hwstr = hw.to_string();
				if(prev_word == ar_b){
					if(wcslen(hwstr) < 3){
						match_indef_after_prep = true;
					}
					else if((hwstr[0] != L'\x627')  || (hwstr[1] != L'\x644')){
						match_indef_after_prep = true;
					}
				}
				if(prev_word == ar_l){
					if(wcslen(hwstr) < 2){
						match_indef_after_prep = true;
					}
					else{
						if(((hwstr[0] == L'\x627')  && ( (hwstr[1] == L'\x644'))) || 
						(hwstr[0] == L'\x644')) {
							;
						}
						else{
							match_indef_after_prep = true;
						}
					}
				}


					

			}
			if(match_quant){
//				std::cout<<"\t\treseting nationality b/c of quantifier: prev word- "<<
//					prev_term->getHeadWord().to_debug_string()<<std::endl;
//				std::cout<<"\t\told mention: ";
//				mention_set->getMention(i)->dump(std::cout);
//				std::cout<<std::endl;
				mention_set->getMention(i)->setEntityType(EntityType(Symbol(L"PER")));
				mention_set->getMention(i)->setEntitySubtype(EntitySubtype::getUndetType());
				mention_set->getMention(i)->mentionType = Mention::DESC;
//				std::cout<<"\t\tnew mention: ";
//				mention_set->getMention(i)->dump(std::cout);
//				std::cout<<std::endl;	
			}
			if(match_indef_after_prep){
//				std::cout<<"\t\treseting nationality b/c of preposition and indefinite: prev word- "<<
//					prev_term->getHeadWord().to_debug_string()<<std::endl;
//				std::cout<<"\t\told mention: ";
//				mention_set->getMention(i)->dump(std::cout);
//				std::cout<<std::endl;
				mention_set->getMention(i)->setEntityType(EntityType(Symbol(L"PER")));
				mention_set->getMention(i)->setEntitySubtype(EntitySubtype::getUndetType());
				mention_set->getMention(i)->mentionType = Mention::DESC;
//				std::cout<<"\t\tnew mention: ";
//				mention_set->getMention(i)->dump(std::cout);
//				std::cout<<std::endl;	
			}
			
			else{
				int necessary_name_start = mention_set->getMention(i)->getNode()->getEndToken() + 1;
				bool found_name = false;
				int name = 0;
				for(k =0; k< n_per_name; k++){
					if(mention_set->getMention(per_name_ment[k])->getNode()->getStartToken() == necessary_name_start){
						found_name = true;
						name = per_name_ment[k]; 
						break;
					}
				}
				if(found_name){
//					std::cout<<"\tfollowed by name: "<<mention_set->getMention(name)->getNode()->getHeadWord().to_debug_string()<<std::endl;
				}
				bool found_premod = false;
				if(found_name){
					int unallowed_premod_end = mention_set->getMention(i)->getNode()->getEndToken()  - 1;
					for(k =0; k< n_per_nom; k++){
						if(mention_set->getMention(per_nom_ment[k])->getNode()->getEndToken() == unallowed_premod_end){
							name =per_nom_ment[k];
							found_premod = true;
							break;
						}
					}
				}
				if(found_premod){
//					std::cout<<"\tpreceded by desc: "<<mention_set->getMention(name)->getNode()->getHeadWord().to_debug_string()<<std::endl;
				}
				if(found_name && ! found_premod){
//					std::cout<<"\t\treseting nationality: "<<std::endl;
//					std::cout<<"\t\told mention: ";
//					mention_set->getMention(i)->dump(std::cout);
//					std::cout<<std::endl;
					mention_set->getMention(i)->setEntityType(EntityType(Symbol(L"PER")));
					mention_set->getMention(i)->setEntitySubtype(EntitySubtype::getUndetType());
					mention_set->getMention(i)->mentionType = Mention::DESC;
//					std::cout<<"\t\tnew mention: ";
//					mention_set->getMention(i)->dump(std::cout);
//					std::cout<<std::endl;
				}
			}	
		
	}
		
	}
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsPronFem(const PartOfSpeech* pos){
	return posTheoryFindMatching2Substrings(pos, L"PRON", L"F");
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsPronMasc(const PartOfSpeech* pos){
	return posTheoryFindMatching2Substrings(pos, L"PRON", L"M");
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsPronSg(const PartOfSpeech* pos){
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"FS")){
		return true;
	}
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"MS")){
		return true;
	}
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"1S")){
		return true;
	}
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"2S")){
		return true;
	}
	return false;
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsPronPl(const PartOfSpeech* pos){
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"FP")){
		return true;
	}
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"MP")){
		return true;
	}
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"1P")){
		return true;
	}
	if(posTheoryFindMatching2Substrings(pos, L"PRON", L"2P")){
		return true;
	}
	return false;
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsPron1p(const PartOfSpeech* pos){
	return posTheoryFindMatching2Substrings(pos, L"PRON", L"1");
}
bool ArabicLanguageSpecificFunctions::POSTheoryContainsPron2p(const PartOfSpeech* pos){
	return posTheoryFindMatching2Substrings(pos, L"PRON", L"2");
}

int ArabicLanguageSpecificFunctions::findNPHead(ParseNode* arr[], int numNodes) {
	//Note: arabic is usually head initial, but numbers are before the words they modify
	int npheadindex = 0;
	if(arr[npheadindex]->label == Symbol(L"NUMERIC")){
		int newindex = npheadindex;
		for(int headi = npheadindex; headi< numNodes; headi++){
			if(arr[headi]->label != Symbol(L"NUMERIC")) {
				newindex = headi;
				break;
			}
		}
		npheadindex = newindex;
	}
	return npheadindex;
}

int ArabicLanguageSpecificFunctions::findNPHead(const SynNode* const arr[], int numNodes) {
	//Note: arabic is usually head initial, but numbers are before the words they modify
	int npheadindex = 0;
	if(arr[npheadindex]->getTag() == Symbol(L"NUMERIC")){
		int newindex = npheadindex;
		for(int headi = npheadindex; headi< numNodes; headi++){
			if(arr[headi]->getTag() != Symbol(L"NUMERIC")) {
				newindex = headi;
				break;
			}
		}
		npheadindex = newindex;
	}
	return npheadindex;
}


/*	for(i =0; i < changed_ids.size(); i++){
		Mention* changedment = mention_set->getMention(changed_ids[i]);
		const SynNode* node = changedment->getNode();


	}
	*/


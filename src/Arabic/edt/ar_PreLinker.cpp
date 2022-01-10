// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/edt/ar_PreLinker.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/common/ParamReader.h"
#include "Arabic/parse/ar_STags.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Arabic/common/ar_NationalityRecognizer.h"


/* Assume NP Chunk Style Trees
	(NP (NN W1) (NN W2) (NPP W3 w4) (NN W5) )

	If either W2 or w5 is the head of a Nominal mention that is of the same type 
	as the NPP, link them
	Note:  This may over link cases like brother (of) Bob !!!!!! also capital (of) Boston
	ACE 2004 data does not seem to be marking title apposotives, but leave this in because
	it should still be correct
*/

void ArabicPreLinker::preLinkTitleAppositives(PreLinker::MentionMap &preLinks, const MentionSet *mentionSet){
	//add a parameter, since I'm not sure that this helps!

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);

		if (mention->getMentionType() != Mention::NAME)
			continue;

		const SynNode *node = mention->node;
		const SynNode *parent;
		const SynNode *nameNode;
		int nameindex;
		if(node->getTag() == ArabicSTags::NPP){
            parent = node->getParent();
			nameNode = node;
			for(int i=0; i< parent->getNChildren(); i++){
				if(parent->getChild(i) == nameNode){
					nameindex = i;
					break;
				}
			}
		}
		else{
			nameNode = node->getHead();
			parent = node;
			nameindex = node->getHeadIndex();
		}

		// node must have at least two children
		int nkids = parent->getNChildren();
		if (nkids < 2)
			continue;


		// right or left sibling must be mention of same type as child 
		const Mention* leftsib = 0;
		const Mention* rightsib = 0;
		if((nameindex > 1) && parent->getChild(nameindex-1)->hasMention()){
			leftsib = 
				mentionSet->getMentionByNode(parent->getChild(nameindex - 1));
		}
		if(((nameindex+1) < parent->getNChildren()) && parent->getChild(nameindex+1)->hasMention()){
			rightsib = 
				mentionSet->getMentionByNode(parent->getChild(nameindex + 1));
		}
		if(!leftsib && !rightsib){
			continue;
		}
		if(!((rightsib) &&(rightsib->getMentionType() == Mention::DESC)) ||
			((leftsib) && (leftsib->getMentionType() == Mention::DESC)))
			continue;
		
		
		// types should be the same: any coercing should have already 
		// taken place in NomPremodClassifier
		if ((leftsib) && (leftsib->getEntityType() == mention->getEntityType()))
			preLinks[mention->getIndex()] = leftsib;
		else if((rightsib) && (rightsib->getEntityType() == mention->getEntityType()))
			preLinks[mention->getIndex()] = rightsib;

		// any other necessary tests? Do I care what's to the left of this?

		// install link
		
		


	}
}


const Mention* ArabicPreLinker::preLinkPerTitles(const MentionSet *mentionSet, const Mention* mention){
	//Mr./Mrs./Ms. etc.
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
	
	if (mention->getMentionType() != Mention::DESC)
		return 0;
	if (mention->getEntityType() != EntityType::getPERType())
		return 0;
	Symbol hw = mention->getNode()->getHeadWord();
	bool is_title = false;
	for(int j =0; j<12; j++){
		if(hw == person_titles[j]){
			is_title = true;
			break;
		}
	}
	if(! is_title){
		return 0;
	}
	const SynNode* title_node = mention->getNode();
	int title_token = mention->getNode()->getHeadPreterm()->getStartToken();
	int adj_token = title_token+1;
	
	const SynNode* parent = mention->getNode()->getParent();
	const Mention* link_ment = 0;
	bool found_name = false;
	bool found_ment_btwn = false;
	int tok_dist =  10000;
	//find the closest mention that is a per/name (only find the first mention) 
	while(!found_ment_btwn && (parent != 0)){
		if(parent->getEndToken() >= (adj_token)){
			for(int k = 0; k< parent->getNChildren(); k++){
				const SynNode* n = parent->getChild(k);
				if(n->getStartToken() < adj_token){
					continue;
				}
				if(n->hasMention()){
					const Mention* oth = mentionSet->getMentionByNode(n);
					//ignore untyped mentions, since these include adjs which may occur
					if(!((oth->getEntityType() == EntityType::getOtherType() )
						|| (oth->getEntityType() == EntityType::getUndetType()))){
						found_ment_btwn = true;
					}
					if(oth->mentionType == Mention::NAME){
						found_name = true;
						if(oth->getEntityType() == EntityType::getPERType()){
							link_ment = oth;
						}
					}
				}
			}
		}
		parent = parent->getParent();
	}
	if(link_ment != 0){
		int link_token = link_ment->getNode()->getStartToken();
		if((link_token - title_token) < 2){
//			std::cout<<"****$$$$**** linking title to a name"<<std::endl;
			return link_ment;

		}
	}
	return 0;

}

const Mention* ArabicPreLinker::preLinkHeadOf(const MentionSet *mentionSet, const Mention* mention){
	return 0;
}
const int ArabicPreLinker::preLinkPerNationalities(PreLinker::MentionMap &link_pairs, const MentionSet *mention_set){
	int per_name_ment[1000];
	int n_per_name = 0;
	int per_nat_ment[1000];
	int n_per_nat = 0;

	int i, k;
	k = 0;
	int n_nat = 0;
	for(i = 0; i< mention_set->getNMentions(); i++){
		if(!mention_set->getMention(i)->getEntityType().matchesPER())
			continue;
		if(mention_set->getMention(i)->getMentionType() != Mention::DESC )
			continue;
		Symbol hw = mention_set->getMention(i)->getNode()->getHeadWord();
		if(ArabicNationalityRecognizer::isPossibleNationalitySymbol(hw)){
			per_nat_ment[n_per_nat++] = i;
		}
	}
	if(n_per_nat == 0)
		return 0;
//	std::cout<<"found per desc"<<std::endl;
	for(i = 0; i< mention_set->getNMentions(); i++){
		if(!mention_set->getMention(i)->getEntityType().matchesPER())
			continue;
		if(mention_set->getMention(i)->getMentionType() != Mention::NAME )
			continue;
		per_name_ment[n_per_name++] = i;
	}
	int nlink = 0;
	for(i = 0; i< n_per_nat; i++){
		//if we have PER-NAT PER-NAME they should be linked!
		int necessary_name_start = mention_set->getMention(per_nat_ment[i])->getNode()->getEndToken() + 1;
		bool found_link = false;
		for(k = 0; k< n_per_name; k++){
			if(mention_set->getMention(per_name_ment[k])->getNode()->getStartToken() == necessary_name_start){
				found_link = true;
//				std::cout<<"add Nationality Link btween: ";
//				mention_set->getMention(per_nat_ment[i])->dump(std::cout);
//				std::cout<<"\n and : ";
//				mention_set->getMention(per_name_ment[k])->dump(std::cout);
//				std::cout<<std::endl;

				link_pairs[nlink++] = mention_set->getMention(per_nat_ment[i]);
				link_pairs[nlink++] = mention_set->getMention(per_name_ment[k]);
				break;
			}
		}
	}
	return nlink;
	


}
void ArabicPreLinker::preLinkSpecialCases(PreLinker::MentionMap &preLinks, const MentionSet *mentionSet,
									const PropositionSet *propSet)
{

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention* mention = mentionSet->getMention(i);
		const Mention* link_ment = preLinkPerTitles(mentionSet, mention);
		if(link_ment !=0){
			preLinks[mention->getIndex()] = link_ment;
			continue;
		}
	}
	//this should improve ACE05, but it doesn't
	/*const Mention* natlinks[1000];
	int nlinks = preLinkPerNationalities(natlinks, mentionSet);
	int k = 0;
	while(k < nlinks){
		preLinks[natlinks[k]->getIndex()] = natlinks[k+1];
		k++;
		k++;
	}*/

}



const int ArabicPreLinker::postLinkSpecialCaseNames(const EntitySet* entitySet, int* entity_link_pairs, int max_pairs){
	//only link Washington to American for now...
	if(entitySet == 0){
//		std::cout<<"return 0, postlinking on empty entity"<<std::endl;	
		return 0;
	}
	int i;

	for(i = 0; i < max_pairs; i++){
		entity_link_pairs[i] = -1;
	}
	int nlinked = 0;
	for(i =0; i< entitySet->getNEntities(); i++){
		Entity* ent = entitySet->getEntity(i);
		if(ent->getNMentions() == 0){
			continue;
		}
		if(i >= max_pairs){
			if(nlinked == 0){
				return 0;
			}
			else{
				return max_pairs;	
			}
		}
		//check if ent contains only "Washington" that is not preceded by in/to 
		bool match_orgish_washington = false;
		bool clash_orgish_washington = false;
		for(int j = 0; j< ent->getNMentions(); j++){
			Mention* ment = entitySet->getMention(ent->getMention(j));
			Symbol hw = ment->getNode()->getHeadWord();
			if(hw == ArabicSymbol(L"wA$nTn")){
				Symbol prev_word = Symbol(L"-NONE-");
				if(ment->getNode()->getHeadPreterm()->getPrevTerminal() !=0){
					prev_word = ment->getNode()->getHeadPreterm()->getPrevTerminal()->getTag();
				}
				if(prev_word == ArabicSymbol(L"b")){
					clash_orgish_washington = true;
				}
				else if(prev_word == ArabicSymbol(L"l")){
					clash_orgish_washington = true;
				}
				else{
					match_orgish_washington = true;
				}
			}
			else{
				clash_orgish_washington = true;
			}
		}
		if(match_orgish_washington && !clash_orgish_washington){
//			std::cout<<"look for America"<<std::endl;
			for(int k = 0; k< entitySet->getNEntities(); k++)
			{
				if(i == k){
					continue;
				}
				Entity* oth_ent = entitySet->getEntity(k);
				if(oth_ent->getNMentions() == 0){
					continue;
				}
				bool match_america = false;
				bool match_us = false;
				Symbol name_words[20];
											 
				for(int m = 0; m< oth_ent->getNMentions(); m++)
				{
					Mention* ment = entitySet->getMention(oth_ent->getMention(m));
					if(ment->getMentionType() != Mention::NAME){
						continue;
					}
					int nwords = ment->getHead()->getTerminalSymbols(name_words, 20);
					if(nwords == 1){
						match_america = _matchAmerica(name_words[0]);
								
					}
					/*else if(nwords == 2){
						if(((name_words[0] == ArabicSymbol(L"wlA")) || 
							(name_words[0] == ArabicSymbol(L"AlwlA") )) &&
							(name_words[1] == ArabicSymbol(L"LmtHdp"))){
								std::cout<<"match u.s."<<std::endl;
								match_us =true;
							}
					}
					*/
				}
				if(match_america || match_us){
					entity_link_pairs[i] = k;
					nlinked++;
				}
			}
		}
	}
	if(nlinked == 0){
		return 0;
	}
	else{
		return entitySet->getNEntities();	
	}
}



const bool ArabicPreLinker::_matchAmerica(Symbol word){
	if(wcslen(word.to_string()) < 5){
		return false;
	}
	if(wcslen(word.to_string()) > 20){
		return false;
	}
	//these should be static class variables, but symbol initialization is fast
	Symbol america_words[33];
	america_words[0]=ArabicSymbol(L">mryky");
	america_words[1]=ArabicSymbol(L">mryKAny");
	america_words[2]=ArabicSymbol(L">mryKyp");
	america_words[3]=ArabicSymbol(L">mryKAnyAn");
	america_words[4]=ArabicSymbol(L">mryKAnyyn");
	america_words[5]=ArabicSymbol(L">mryKAnytAn");
	america_words[6]=ArabicSymbol(L">mryKAnytyn");
	america_words[7]=ArabicSymbol(L">mryKAnyAt");
	america_words[8]=ArabicSymbol(L">mryKAnywn");
	america_words[9]=ArabicSymbol(L">mryKAny");
	america_words[10]=ArabicSymbol(L">mryKAnyp");
	america_words[11]=ArabicSymbol(L">mryKAnyAn");
	america_words[12]=ArabicSymbol(L">mryKAnytAn");
	america_words[13]=ArabicSymbol(L">mryKAnyyn");
	america_words[14]=ArabicSymbol(L">mryKAnytyn");
	america_words[15]=ArabicSymbol(L">mryKAnyAt");
	america_words[16]=ArabicSymbol(L">mryKAnywn");
	america_words[17]=ArabicSymbol(L"[mryKAny");
	america_words[18]=ArabicSymbol(L"[mryKyp");
	america_words[19]=ArabicSymbol(L"[mryKAnyAn");
	america_words[20]=ArabicSymbol(L"[mryKAnyyn");
	america_words[21]=ArabicSymbol(L"[mryKAnytAn");
	america_words[22]=ArabicSymbol(L"[mryKAnytyn");
	america_words[23]=ArabicSymbol(L"[mryKAnyAt");
	america_words[24]=ArabicSymbol(L"[mryKAnywn");
	america_words[25]=ArabicSymbol(L"[mryKAny");
	america_words[26]=ArabicSymbol(L"[mryKAnyp");
	america_words[27]=ArabicSymbol(L"[mryKAnyAn");
	america_words[28]=ArabicSymbol(L"[mryKAnytAn");
	america_words[29]=ArabicSymbol(L"[mryKAnyyn");
	america_words[30]=ArabicSymbol(L"[mryKAnytyn");
	america_words[31]=ArabicSymbol(L"[mryKAnyAt");
	america_words[32]=ArabicSymbol(L"[mryKAnywn");

	Symbol cmp_word = word;
	wchar_t word_buff[25];

	if((word.to_string()[0] == L'\x627') &&
		(word.to_string()[1] == L'\x644') )
		{
			int q;
			for(q = 2; q < static_cast<int>(wcslen(word.to_string())); q++){
				word_buff[q-2] = word.to_string()[q];
			}
			word_buff[q-2] = L'\0';
			cmp_word = Symbol(word_buff);
		}
	for(int n = 0; n < 33; n++){
		if(america_words[n] == word){
			return true;
		}
	}
	return false;
}

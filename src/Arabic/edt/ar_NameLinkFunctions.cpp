// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/edt/ar_NameLinkFunctions.h"
#include "Generic/theories/SynNode.h"
#include "Generic/edt/AbbrevTable.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Arabic/edt/ar_BaseFormMaker.h"
#include "Generic/common/limits.h"
#include "Arabic/common/ar_WordConstants.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"

DebugStream &ArabicNameLinkFunctions::_debugOut = DebugStream::referenceResolverStream;

bool ArabicNameLinkFunctions::populateAcronyms(const Mention *mention, EntityType linkType) {
	Symbol words[32],cliticResolvedWords[32], resolvedWords[32];
	int nWords = mention->getHead()->getTerminalSymbols(words, 32);


	//ACE2005 appends desc words (city, state, etc.)  to a name, 
	//remove these words so names don't link 
	//because of them
	int i;
	if(nWords > 1){
		bool removeFirst = false;
		if(mention->getEntityType().matchesGPE()){
			if(ArabicWordConstants::matchGPEWord(words[0])){
				removeFirst = true;
			}
			//check that this isn't united states
			if((words[0] == ArabicSymbol(L"wlAyp")) &&
				(words[1] == ArabicSymbol(L"mtHdp")))
			{
					removeFirst = false;
			}
		}
		else if(mention->getEntityType().matchesORG()){
			if(ArabicWordConstants::matchORGWord(words[0])){
				removeFirst = true;
			}
		}
		else if(mention->getEntityType().matchesFAC()){
			if(ArabicWordConstants::matchFACWord(words[0])){
				removeFirst = true;
			}
		}
		if(mention->getEntityType().matchesLOC()){
			if(ArabicWordConstants::matchLOCWord(words[0])){
				removeFirst = true;
			}
		}
		if(removeFirst){
			for(i = 0; i < nWords-1; i++){
				words[i] = words[i+1];
			}
			words[i] = Symbol();
			nWords--;
		}
	}

	bool changed = false;
	for(i =0; i<nWords; i++){
		cliticResolvedWords[i] = BaseFormMaker::removeNameClitics(words[i]);
		//cliticResolvedWords[i] = words[i];
		if(mention->getEntityType().matchesGPE()){
			resolvedWords[i] = BaseFormMaker::getGPEBaseForm(cliticResolvedWords[i]);
			//remove w,b,l a second time since we want wAlbakistani to match bakistani
			//make this a function!
			if((cliticResolvedWords[i].to_debug_string()[0] =='A') 
				&& (cliticResolvedWords[i].to_debug_string()[1] =='l')){
				cliticResolvedWords[i] = BaseFormMaker::removeNameClitics(resolvedWords[i]);
				resolvedWords[i] = cliticResolvedWords[i];
			}
		}
		else if(mention->getEntityType().matchesPER()){
			resolvedWords[i] = BaseFormMaker::getPERBaseForm(cliticResolvedWords[i]);
			if((cliticResolvedWords[i].to_debug_string()[0] =='A') 
				&& (cliticResolvedWords[i].to_debug_string()[1] =='l')){
				cliticResolvedWords[i] = BaseFormMaker::removeNameClitics(resolvedWords[i]);
				resolvedWords[i] = cliticResolvedWords[i];
			}

		}
		else{
			resolvedWords[i] = cliticResolvedWords[i];
		}
        if(resolvedWords[i]!=words[i]){
			changed = true;
		}
	}
	if(changed){
		Symbol tableWords[32];
		//do an extra lookup incase the resolved form has an 'alias' - 
		//American  resloves to Amerik which resolves to United States
		//NOTE: if resolvedWords is not in the table, then tableWords == resolvedWords
		_debugOut << "Original Words: ";
		for(int d = 0; d< nWords; d++){
			_debugOut <<words[d].to_debug_string()<<" ";
		}
		_debugOut << "\nBaseForm Resolved To: ";
		for(int e = 0; e< nWords; e++){
			_debugOut <<resolvedWords[e].to_debug_string()<<" ";
		}
		int nResolved = AbbrevTable::resolveSymbols(resolvedWords,nWords,tableWords, 32);
		_debugOut << "\nTable Resolved To: ";
		for(int f = 0; f< nResolved; f++){
			_debugOut <<tableWords[f].to_debug_string()<<" ";
		}

		AbbrevTable::add(words, nWords, tableWords, nResolved);
		return true;
	}
	else{
		return false;
	}

	/*
	if(mention->getEntityType().matchesGPE()){
		bool changed = false;
		for(int i =0; i<nWords; i++){
			resolvedWords[i] = BaseFormMaker::getGPEBaseForm(words[i]);
			if(resolvedWords[i]!=words[i]){
				changed = true;
			}
		}
		
		if(changed){
			Symbol tableWords[32];
			//do an extra lookup incase the resolved form has an 'alias' - 
			//American  resloves to Amerik which resolves to United States
			//NOTE: if resolvedWords is not in the table, then tableWords == resolvedWords
			_debugOut << "Original Words: ";
			for(int d = 0; d< nWords; d++){
				_debugOut <<words[d].to_debug_string()<<" ";
			}
			_debugOut << "\nBaseForm Resolved To: ";
			for(int d = 0; d< nWords; d++){
				_debugOut <<resolvedWords[d].to_debug_string()<<" ";
			}
			int nResolved = AbbrevTable::resolveSymbols(resolvedWords,nWords,tableWords, 32);
			_debugOut << "\nTable Resolved To: ";
			for(int d = 0; d< nResolved; d++){
				_debugOut <<tableWords[d].to_debug_string()<<" ";
			}

			AbbrevTable::add(words, nWords, tableWords, nResolved);
			return true;
		}
		else{
			return false;
		}

	}
    return false;
	*/

}
	


void ArabicNameLinkFunctions::recomputeCounts(CountsTable &inTable, 
										CountsTable &outTable, 
										int &outTotalCount)
{
	outTotalCount = 0;
	outTable.cleanup();
	for(CountsTable::iterator iterator = inTable.begin(); iterator!=inTable.end(); ++iterator) {
		Symbol unresolved, resolved[16];
		int nResolved;
		unresolved = iterator.value().first;
		int thisCount = iterator.value().second;
		nResolved = AbbrevTable::resolveSymbols(&unresolved, 1, resolved, 16);
		for(int i=0; i<nResolved; i++) {
			outTable.add(resolved[i], thisCount);
			outTotalCount += thisCount;
		}
	}
}
EntitySet* ArabicNameLinkFunctions::mergeEntities(EntitySet* entities){
	//std::cout <<"Called Merge Entities\n";
	std::vector<std::pair<int, int> > mergedEntities;
	std::set<int> removedEntities;

	//find the entities that should be merged
	for(int i =0; i< entities->getNEntities(); i++){
		Entity* e1 = entities->getEntity(i);
		EntityType type = e1->type;
		if(removedEntities.find(i) != removedEntities.end()){
			continue;
		}
		if(type.isRecognized()){
			for(int j = i +1; j<entities->getNEntities();j++){
				if(removedEntities.find(j) != removedEntities.end()){
					continue;
				}
				Entity* e2 = entities->getEntity(j);
				if(_mentionsMatch(e1,e2, entities)){ 
					mergedEntities.push_back(std::make_pair(i,j));
					removedEntities.insert(i);
					removedEntities.insert(j);
					continue;
					
				}
			}
		}
	}
	//make a new EntitySet
	EntitySet* newSet =  _new EntitySet(entities->getNMentionSets());

	//change types before putting mention sets in new Entity Set
	//because we want "Mentions By Type" to be correct
	for(size_t i = 0; i<mergedEntities.size(); i++){
		Entity* e1=	entities->getEntity(mergedEntities[i].first);
		Entity* e2 = entities->getEntity(mergedEntities[i].second);
		EntityType type;
		//vote on the EntityType and switch type mentions
		//NOTE: must switch type of mentions before you put mention Set in 
		//new entity set
		if(e1->getNMentions() > e2->getNMentions()){
			type =  e1->getType() ;
			for(int j = 0; j< e2->getNMentions(); j++){
				entities->getMention(e2->getMention(j))->setEntityType(type);
			}
		}			
		else{ 
			type =  e2->getType();
			for(int j = 0; j< e1->getNMentions(); j++){
				entities->getMention(e1->getMention(j))->setEntityType(type);
			}
		}
	}
	for(int i = 0; i< entities->getNMentionSets(); i++){
		newSet->loadMentionSet(entities->getMentionSet(i));
	}
	//put unchanged entities in new EntitySet
	for(int i = 0; i<entities->getNEntities(); i++){
		if(removedEntities.find(i) == removedEntities.end()){
			Entity* e = entities->getEntity(i);
			Mention* m = entities->getMention(e->getMention(0));
			newSet->addNew(m->getUID(), m->getEntityType());
			int id = newSet->getEntityByMention(m->getUID())->getID();
			for(int j =1; j< e->getNMentions(); j++){
				m =entities->getMention(e->getMention(j));
				newSet->add(m->getUID(), id);
			}
		}
	}
	//put merged entities in new EntitSet
	for(size_t i = 0; i<mergedEntities.size(); i++){
		Entity* e1=	entities->getEntity(mergedEntities[i].first);
		Entity* e2 = entities->getEntity(mergedEntities[i].second);
		EntityType type;
		
		if(e1->getNMentions() > e2->getNMentions()){
			type =  e1->getType() ;
		}			
		else{ 
			type =  e2->getType();
		}
		
	
		Mention* m = entities->getMention(e1->getMention(0));
		MentionUID uid = m->getUID();
		newSet->addNew(uid, m->getEntityType());
		Entity* newEnt = newSet->getEntityByMention(uid);
		int id = newEnt->getID();
		for(int j =1; j< e1->getNMentions(); j++){
			m = entities->getMention(e1->getMention(j));
			newSet->add(m->getUID(), id);
		}
		for(int k = 0; k <e2->getNMentions(); k++){
			m = entities->getMention(e2->getMention(k));
			newSet->add(m->getUID(), id);
	}

}

	return newSet;
}
bool ArabicNameLinkFunctions::_mentionsMatch(Entity* e1, Entity* e2, EntitySet* entitySet){
	

	Symbol words1[32], resolved1[32],words2[32], resolved2[32];
	for(int i = 0; i <e1->getNMentions(); i++){
		//NOTE: This is a hack to get the sentence number from a Mention UID
		//If this becomes permanent add method to Mention
		Mention* m1 =	entitySet->getMention(e1->getMention(i));
		int nWords1 = m1->getHead()->getTerminalSymbols(words1, 32);
		int nResolved1 = AbbrevTable::resolveSymbols(words1, nWords1, resolved1, 32);

		for(int j =0; j < e2->getNMentions(); j++){
			Mention* m2 =	entitySet->getMention(e2->getMention(j));
			int nWords2 = m2->getHead()->getTerminalSymbols(words2, 32);
			int nResolved2 = AbbrevTable::resolveSymbols(words2, nWords2, resolved2, 32);
			if((nWords1 > 0) && (nWords1 == nWords2)){
				bool all_same = false;
				for(int k =0; k < nWords1; k++){
					if(words1[k] != words2[k]){
						all_same = false;
						break;
					}
					else{
						all_same= true;
					}

				}


				if(all_same) {
					return true;
				}
			}
			
		}
	}
	return false;
}





	
int  ArabicNameLinkFunctions::getLexicalItems(Symbol words[], int nWords, Symbol results[], int max_results) {
		if (nWords > max_results) {
			SessionLogger::warn("get_lexical_items")
				<< "GenericArabicNameLinkFunctions::getLexicalItems(): "
				<< "nWords > max_results, truncating words\n";
		}
		if(nWords == 1){
			results[0] = words[0];
			return 1;
		}
		else{ //the arabic name finder includes commas and clitics in the names... get rid of these
			Symbol ignore_start[5];
			Symbol punc[9];
			Symbol ignore_end[1];
			ignore_start[0] = ArabicSymbol(L"w");
			ignore_start[1] = ArabicSymbol(L"b");
			ignore_start[2] = ArabicSymbol(L"l");
			ignore_start[3] = ArabicSymbol(L"f");
			ignore_start[4] = ArabicSymbol(L"k");
			ignore_end[0] = ArabicSymbol(L"y");
			wchar_t buff[2];
			buff[0] = L'\x60c';
			buff[1] = L'\0';
			punc[0] = Symbol(buff);
			buff[0] = L'\xAB';
			punc[1] = Symbol(buff);
			buff[0] = L'\xBB';
			punc[2] = Symbol(buff);
			buff[0] = L'\xAD';
			punc[3] = Symbol(buff);
			buff[0] = L'\x61F';
			punc[4] = Symbol(buff);
			buff[0] = L'\x61B';
			punc[5] = Symbol(buff);
			punc[6] = Symbol(L"''");
			punc[7] = Symbol(L".");
			punc[8] = Symbol(L"-");
			int i = 0;
			int start = 0;
			int j = 0;
			while((i < nWords) && (i < max_results) && (i == start)){

				for(j = 0; j< 6; j++){
					if(ignore_start[j] == words[i]){
						start++;
					}
				}
				for(j = 0; j< 9; j++){
					if(punc[j] == words[i]){
						start++;
					}
				}
	

				i++;
			}
			int end = nWords-1;
			i = end;
			while((i > start) && (i == end)){
				for(j = 0; j< 9; j++){
					if(punc[j] == words[i]){
						end--;
					}
				}
				for(j = 0; j< 1; j++){
					if(ignore_end[j] == words[i]){
						end--;
					}
				}
				i--;
			}
			end++;


			j = 0;
			Symbol ignore = ArabicSymbol(L"bn");
			for (i = start; i < end && i < max_results; i++) {
				if(words[i] != ignore){ 	
					results[j] = words[i];
					j++;
				}
				else{
//					std::cerr<<"REMOVING BIN FROM NAME!\n";
				}
			}
			if(j == 0){
				for(i = 0; ((i < nWords) &&(i< max_results)); i++){
					results[i] = words[i];
				}
				return i;
			}
			else{
				return j;
			}
		}
	}

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iomanip>
#include <map>
#include <set>
#include "boost/regex.hpp"
#include "Generic/state/TrainingLoader.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelationSet.h"

using namespace std;


class MenOffsets {
	static const int THR=2;
	EDTOffset _headStart, _headEnd, _menStart, _menEnd;
	bool _isGood;
	mutable int _closest;
public:
	MenOffsets() : _isGood(false) {}
	MenOffsets(const Mention* m, const TokenSequence* ts) : _closest(-1), _isGood(false) {
		const SynNode* sn=m->getNode();
		_menStart = ts->getToken(sn->getStartToken())->getStartEDTOffset();
		_menEnd = ts->getToken(sn->getStartToken())->getEndEDTOffset();
		sn = m->getHead();
		if ( ! sn ) return;
		_headStart = ts->getToken(sn->getStartToken())->getStartEDTOffset();
		_headEnd = ts->getToken(sn->getStartToken())->getEndEDTOffset();
		_isGood = true;
	}
	//MenOffsets(const MenOffsets& mo) : _headStart(mo._headStart), _headEnd(mo._headEnd),
	//	_menStart(mo._menStart), _menEnd(mo._menEnd), _closest(mo._closest) {}
	bool isGood() const { return _isGood; }
	bool headWithin(const MenOffsets& mo, int thr=THR) const {
		return _isGood && mo._isGood && abs(_headStart.value()-mo._headStart.value()) <= thr && abs(_headEnd.value()-mo._headEnd.value()) <= thr;
	}
	bool acceptIfCloser(const MenOffsets& mo) const {
		if ( ! _isGood || ! mo._isGood ) return false;
		bool accept=false;
		int dist=abs(mo._menStart.value()-_menStart.value())+abs(mo._menEnd.value()-_menEnd.value());
		if ( _closest < 0 || dist < _closest ) {
			_closest = dist;
			accept = true;
		}
		return accept;
	}
};


//typedef std::map<std::wstring,int> HWordTypeCounts; //TEMP
//typedef std::map<std::wstring,HWordTypeCounts> HWordCounts; //TEMP

typedef std::map<int,int> MentionMappings;

int findMentionPairs(const DocTheory* dtFrom, const DocTheory* dtTo, MentionMappings& mmFrom, MentionMappings& mmTo);
void saveDocStates(DocTheory* dt, StateSaver* ss);
int changeRelations(const DocTheory* dtFrom, DocTheory* dtTo, const MentionMappings& mmFrom, const MentionMappings& mmTo, const bool generate_ignore = false);
const Mention* getRelationCapableParent(const Mention* m);
//bool checkMentionETypes(DocTheory* dt);  //TEMP
//void collectETypesForHeadwords(const DocTheory* dt, HWordCounts& hwc);  //TEMP
//void printHeadwordsWithETypes(const HWordCounts& hwc);  //TEMP

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "MentionMapper.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		bool verbose=ParamReader::isParamTrue("verbose");

		std::string log_file = ParamReader::getRequiredParam("mention_mapper_log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
		const wchar_t *context_name = L"mention-mapping";
		SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(), 1, &context_name);

		char fileListFrom[501], fileListTo[501];
		char fileFrom[501], fileTo[501], fileToSuffix[501];
		std::string fileNew;
		UTF8Token token;
		ParamReader::getRequiredNarrowParam(fileListFrom, L"file_list_from", 500);
		ParamReader::getRequiredNarrowParam(fileListTo, L"file_list_to", 500);

		if (!ParamReader::getNarrowParam(fileToSuffix, L"file_to_suffix", 500)) {
			strcpy(fileToSuffix, "-rel-altered");
		}

		int totDocsFrom=TrainingLoader::countDocumentsInFileList(fileListFrom);
		int totDocsTo=TrainingLoader::countDocumentsInFileList(fileListTo);
		if ( totDocsFrom != totDocsTo ) {
			std::wcerr << "error: different total number of documents: " << totDocsFrom << " vs. " << totDocsTo;
			exit(-1);
		}
		wchar_t state_tree_name[]=L"DocTheory following stage: doc-relations-events";

		UTF8InputStream fileListStreamFrom, fileListStreamTo;
		fileListStreamFrom.open(fileListFrom);
		fileListStreamTo.open(fileListTo);

		//HWordCounts hwc; //TEMP

		int docs=0;
		while (1) {
			fileListStreamFrom >> token;
			StringTransliterator::transliterateToEnglish(fileFrom, token.chars(), 500);
			fileListStreamTo >> token;
			StringTransliterator::transliterateToEnglish(fileTo, token.chars(), 500);
			if ( ! strlen(fileFrom) || ! strlen(fileTo) ) break;
			//fileNew = std::string(fileTo) + "-rel-altered"; //!!!!!!!!!!!!!!!!!!
			fileNew = std::string(fileTo) + fileToSuffix; //!!!!!!!!!!!!!!!!!!

			StateLoader *stateLoaderFrom = _new StateLoader(fileFrom);
			StateLoader *stateLoaderTo = _new StateLoader(fileTo);
			StateSaver *stateSaverNew = _new StateSaver(fileNew.c_str());

			int numDocsFrom = TrainingLoader::countDocumentsInFile(fileFrom);
			int numDocsTo = TrainingLoader::countDocumentsInFile(fileTo);
			if ( numDocsFrom != numDocsTo ) {
				std::wcerr << "error: different number of documents: " << numDocsFrom << " vs. " << numDocsTo;
				exit(-1);
			}
			DocTheory* dtFrom, *dtTo;
			if ( verbose ) std::cerr << "\nprocess documents: " << std::setw(8) << docs;
			for (int i = 0; i < numDocsFrom; i++) {
				MentionMappings mmFrom, mmTo;

				dtFrom =  _new DocTheory(0);
				dtFrom->loadFakedDocTheory(stateLoaderFrom, state_tree_name);
				dtFrom->resolvePointers(stateLoaderFrom);
				//collectETypesForHeadwords(dtFrom, hwc);		if ( 0 ) { //TEMP
				//checkMentionETypes(dtFrom);			if ( 0 ) {
				dtTo =  _new DocTheory(0);
				dtTo->loadFakedDocTheory(stateLoaderTo, state_tree_name);
				dtTo->resolvePointers(stateLoaderTo);

				findMentionPairs(dtFrom, dtTo, mmFrom, mmTo);
				changeRelations(dtFrom, dtTo, mmFrom, mmTo, ParamReader::isParamTrue("generate_ignore"));
				saveDocStates(dtTo,stateSaverNew);
				delete dtTo;
				//} //TEMP
				delete dtFrom;
				docs++;
				if ( verbose ) std::cerr << "\b\b\b\b\b\b\b\b" << std::setw(8) << docs;
			}
			delete stateLoaderFrom;
			delete stateLoaderTo;
			delete stateSaverNew;
		}
		fileListStreamFrom.close();
		fileListStreamTo.close();
		if ( verbose ) std::cerr << std::endl;

		//printHeadwordsWithETypes(hwc); //TEMP

	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

	return 0;
}


void saveDocStates(DocTheory* dt, StateSaver* ss) {
	//dt->saveSentenceBreaksToStateFile(ss);
	wchar_t state_tree_name[]=L"Sentence breaks: ";
	ObjectIDTable::initialize();
	ss->beginStateTree(state_tree_name);
	ss->saveSymbol(dt->getSentenceTheory(0)->getDocID());
	ss->saveInteger(dt->getNSentences());
	ss->beginList(L"sentence-breaks");

	for(int j = 0; j < dt->getNSentences(); j++){
		const Sentence* sent = dt->getSentence(j);
		if(sent->isAnnotated()){
			ss->saveSymbol(Symbol(L"ANNOTATABLE_REGION"));
		}
		else{
			ss->saveSymbol(Symbol(L"UNANNOTATABLE_REGION"));
		}
		ss->saveInteger(0); //!!! FAKE
		ss->saveInteger(0); //!!! FAKE
	}
	ss->endList();
	ss->endStateTree();

	ObjectIDTable::initialize();
	dt->updateObjectIDTable();
	ss->beginStateTree(L"DocTheory following stage: doc-relations-events");
	ss->saveInteger(dt->getNSentences());
	dt->saveState(ss);
	ss->endStateTree();
}

int findMentionPairs(const DocTheory* dtFrom, const DocTheory* dtTo, MentionMappings& mmFrom, MentionMappings& mmTo) {
	typedef std::map<const Mention*,MenOffsets> MentionsWithOffsets; // index ==> (start,end)
	MentionsWithOffsets mensFrom, mensTo;
	if ( ! dtFrom->getNSentences() ) return 0;
	std::string docId=dtFrom->getSentenceTheory(0)->getDocID().to_debug_string();

	for ( int sentFrom = 0; sentFrom < dtFrom->getNSentences(); sentFrom++ ) {
		const SentenceTheory* stFrom=dtFrom->getSentenceTheory(sentFrom);
		const MentionSet* msFrom=stFrom->getMentionSet();
		for ( int m=0; m < msFrom->getNMentions(); m++ ) {
			const Mention* menFrom=msFrom->getMention(m);
			if ( ! menFrom->isOfRecognizedType() ) continue;
			mensFrom[menFrom] = MenOffsets(menFrom,stFrom->getTokenSequence());
		}
	}
	for ( int sentTo = 0; sentTo < dtTo->getNSentences(); sentTo++ ) {
		const SentenceTheory* stTo=dtTo->getSentenceTheory(sentTo);
		const MentionSet* msTo=stTo->getMentionSet();
		for ( int m=0; m < msTo->getNMentions(); m++ ) {
			const Mention* menTo=msTo->getMention(m);
			if ( ! menTo->isOfRecognizedType() ) continue;
			mensTo[menTo] = MenOffsets(menTo,stTo->getTokenSequence());
		}
	}

	//int totalFound=0;
	for ( MentionsWithOffsets::const_iterator mvociFrom=mensFrom.begin(); mvociFrom != mensFrom.end(); mvociFrom++ ) {
		//const Mention* xxx=mvociFrom->first; int xxxi=xxx->getUID();
		bool found=false;
		for ( MentionsWithOffsets::const_iterator mvociTo=mensTo.begin(); mvociTo != mensTo.end(); mvociTo++ ) {
			/*	std::wcerr << "\n" << mvociFrom->first->getUID() << "->" << mvociTo->first->getUID() 
					<< ": " << mvociFrom->second.first << "," << mvociFrom->second.second << " - "
					<< mvociTo->second.first << "," << mvociTo->second.second << "\n\n"; 	*/

			//const Mention* yyy=mvociTo->first; int yyyi=yyy->getUID();
			const Mention* menToParent=getRelationCapableParent(mvociTo->first); //here we might introduce some noise
			if ( ! menToParent ) continue;
			if ( mvociFrom->second.headWithin(mvociTo->second) && mvociFrom->second.acceptIfCloser(mvociTo->second) ) {
//!!!! later try different comparison methods, e.g. the entity with the closest distance... !!!//
				mmFrom[mvociFrom->first->getUID().toInt()] = menToParent->getUID().toInt(); //mvociTo->first->getUID();
				mmTo[menToParent->getUID().toInt()] = mvociFrom->first->getUID().toInt();  //mvociTo->first->getUID()
				std::cout << "\n" << docId << "\t" << mvociFrom->first->getUID() << "\t" << mvociTo->first->getUID();
				std::cout << "\t\"" << mvociFrom->first->getNode()->toDebugTextString() << "\" <=";
				if ( menToParent != mvociTo->first ) std::cout << "|";
				std::cout << "=> \"" << menToParent->getNode()->toDebugTextString() << "\"";
				//totalFound++;
				found = true;
			}
		}
		if ( ! found ) {
			std::cout << "\n" << docId << "\t" << mvociFrom->first->getUID() << "\t-1";
			std::cout << "\t\"" << mvociFrom->first->getNode()->toDebugTextString() << "\" <==> ???";
		}
	}
	return 1;
}

/*! map relations as they were found in the constrained parse to the unconstrained parse

for all relations on the From-side, find both mentions participating in it, find their mappings on the To-side 
and if they exist, induce a relation on the To-side. Then, for all mentions on the To-side, check if they have no
corresponding mentions on the From-side, and if yes, create IGNORE-relations for all mention pairs involving them
\param dtFrom doc theory from the From-side (constrained parse)
\param dtTo doc theory from the To-side (unconstrained parse)
\param mmFrom mapping of mentions from From to To
\param mmTo mapping of mentions from To to From
\return 0 (intended: number of created relations on the To-side)
*/
int changeRelations(const DocTheory* dtFrom, DocTheory* dtTo, const MentionMappings& mmFrom, const MentionMappings& mmTo, const bool generate_ignore) {
	typedef std::map<int,std::map<int,RelMention*> > AllDocRelations;
	AllDocRelations newDocToRelations;
	std::string docId=dtFrom->getSentenceTheory(0)->getDocID().to_debug_string();

	int sentFrom, sentTo;
	for ( sentFrom = 0; sentFrom < dtFrom->getNSentences(); sentFrom++ ) { //look at all sentences on the From-side
		SentenceTheory* stFrom=dtFrom->getSentenceTheory(sentFrom);
		const RelMentionSet* relmsFrom=stFrom->getRelMentionSet();
		for ( int r=0; r < relmsFrom->getNRelMentions(); r++ ) { //...and each Relation there
			RelMention* relmFrom=relmsFrom->getRelMention(r);
			std::cout << "\n" << docId << ": " << relmFrom->toDebugString();
			int menuidFromL = relmFrom->getLeftMention()->getUID().toInt();
			int menuidFromR = relmFrom->getRightMention()->getUID().toInt();
			//make sure mentions of the relation are mapped to one and only one sentence
			MentionMappings::const_iterator mmciL = mmFrom.find(menuidFromL);
			MentionMappings::const_iterator mmciR = mmFrom.find(menuidFromR);
			if ( mmciL == mmFrom.end() ) { std::cout << "  ==> can't map L=" << menuidFromL; continue; }
			if ( mmciR == mmFrom.end() ) { std::cout << "  ==> can't map R=" << menuidFromR; continue; }
			int sentTo = Mention::getSentenceNumberFromUID(Mention::UID(mmciL->second));
			if ( sentTo != Mention::getSentenceNumberFromUID(Mention::UID(mmciR->second)) ) {
				std::cout << "  ==> map to diff sentences " << sentTo << ":" << Mention::getSentenceNumberFromUID(Mention::UID(mmciR->second));
				continue;
			}
			const SentenceTheory* stTo=dtTo->getSentenceTheory(sentTo);
			const MentionSet* msetTo=stTo->getMentionSet();
			//const Mention* menToL=getRelationCapableParent(msetTo->getMention(Mention::getIndexFromUID(mmciL->second)));
			//const Mention* menToR=getRelationCapableParent(msetTo->getMention(Mention::getIndexFromUID(mmciR->second)));
			const Mention* menToL=msetTo->getMention(Mention::getIndexFromUID(Mention::UID(mmciL->second)));
			const Mention* menToR=msetTo->getMention(Mention::getIndexFromUID(Mention::UID(mmciR->second)));
			if ( ! menToL || ! menToR ) continue;
			std::map<int,RelMention*>& relms=newDocToRelations[sentTo];
			int id=(int)relms.size();
			relms[id] = _new RelMention(menToL,menToR,relmFrom->getType(),sentTo,id,relmFrom->getScore());

			std::cout << "  ==> " << relms[id]->toDebugString();
		}
	}

	for ( sentTo = 0; sentTo < dtTo->getNSentences(); sentTo++ ) { //look at all sentences on the To-side
		SentenceTheory* stTo=dtTo->getSentenceTheory(sentTo);
		std::map<int,RelMention*>& relms=newDocToRelations[sentTo];

		//mark as "IGNORE" all entity mentions that can not be mapped to the From-side (they might or might not result in relations)
		const MentionSet* msetTo=stTo->getMentionSet();
		for ( int idToL = 0; idToL < msetTo->getNMentions(); idToL++ ) {
			Mention* menToL=msetTo->getMention(idToL);
			if ( ! menToL->isOfRecognizedType() ) continue;
			Mention::Type mtype=menToL->getMentionType();
			if ( mtype == Mention::NONE || mtype == Mention::APPO || mtype == Mention::LIST ) continue;
			MentionMappings::const_iterator mmciL = mmTo.find(menToL->getUID().toInt());
			int sentToL=( mmciL == mmTo.end() ) ? -1 : Mention::getSentenceNumberFromUID(Mention::UID(mmciL->second));
			for ( int idToR = idToL+1; idToR < msetTo->getNMentions(); idToR++ ) { //!!! MAYBE START FROM 0 AGAIN ???
				Mention* menToR=msetTo->getMention(idToR);
				if ( ! menToR->isOfRecognizedType() ) continue; // ---//---
				Mention::Type mtype=menToR->getMentionType();
				if ( mtype == Mention::NONE || mtype == Mention::APPO || mtype == Mention::LIST ) continue;
				MentionMappings::const_iterator mmciR = mmTo.find(menToR->getUID().toInt());
				int sentToR=( mmciR == mmTo.end() ) ? -1 : Mention::getSentenceNumberFromUID(Mention::UID(mmciR->second));
				
				if ( sentToL != sentToR || sentToL == -1 ) { //mark this pair
					if (generate_ignore) {
					  int id=(int)relms.size();
					  relms[id] = _new RelMention(menToL,menToR,L"IGNORE",sentTo,id,0);
					}
					std::cout << "\n" << docId << ": IGNORE  <== " << menToL->getUID() << "(" << sentToL 
						<< ")," << menToR->getUID() << "(" << sentToR << ")";
				}
			}
		}

		RelMentionSet* relmsTo=_new RelMentionSet();
		for ( std::map<int,RelMention*>::const_iterator mrmci=relms.begin(); mrmci != relms.end(); mrmci++ )
			relmsTo->takeRelMention(mrmci->second);
		stTo->adoptSubtheory(SentenceTheory::RELATION_SUBTHEORY, relmsTo);
	}

	dtTo->takeDocumentRelMentionSet(_new RelMentionSet(0));
	dtTo->takeRelationSet(_new RelationSet(0)); //!!! we don't care about RelationSet, only RelMentionSet is relevant!
	return 0;
}

/*! climb up the syntactic tree till you find a mention that is of a type that can be involved in relations
\param m suggested mention to start from
\return found relation-capable mention or 0 if none
\note yes, I like the function name too :-)
*/
const Mention* getRelationCapableParent(const Mention* m) {
	while ( m ) {
		Mention::Type mtype=m->getMentionType();
		if ( mtype != Mention::NONE && mtype != Mention::APPO && mtype != Mention::LIST ) return m;
		m = m->getParent();
	}
	return 0;
}


/*! TEMP - has nothing to do with the project: check if all mentions in an entity have the same 
	entity type as the entity itself
*/
bool checkMentionETypes(DocTheory* dt) {
	bool toret=true;
	EntitySet* es=dt->getEntitySet();
	for ( int i=0; i < es->getNEntities(); i++ ) {
		Entity* ent=es->getEntity(i);
		std::wstring etype=ent->getType().getName().to_string();
		for ( int k=0; k < ent->getNMentions(); k++ ) {
			Mention* men=es->getMention(ent->getMention(k));
			std::wstring metype=men->getEntityType().getName().to_string();

			std::wstring mname=men->getHead()->toTextString();
			if ( mname.find(L"forces") != mname.npos ) {
				std::wcout << "doc=" << dt->getSentenceTheory(0)->getDocID().to_string() 
					<< ", ent=" << ent->getCanonicalNameOneWord() << "(" << etype << "), men=" 
					<< men->getNode()->toTextString() << "(" << metype << ")\n";
			}

			/*if ( metype != etype ) {
				toret = false;
				std::wcout << "doc=" << dt->getSentenceTheory(0)->getDocID().to_string() 
					<< ", ent=" << ent->getCanonicalNameOneWord() << "(" << etype << "), men=" 
					<< men->getNode()->toTextString() << "(" << metype << ")\n";
			}*/

		}
	}
	return toret;
}

/*
void collectETypesForHeadwords(const DocTheory* dt, HWordCounts& hwc) {
	for ( int s = 0; s < dt->getNSentences(); s++ ) {
		SentenceTheory* st=dt->getSentenceTheory(s);
		const MentionSet* ms=st->getMentionSet();
		for ( int m = 0; m < ms->getNMentions(); m++ ) {
			Mention* men=ms->getMention(m);
			if ( ! men->isOfRecognizedType() ) continue;
			Mention::Type mtype=men->getMentionType();
			if ( mtype == Mention::NONE || mtype == Mention::APPO || mtype == Mention::LIST ) continue;
			std::wstring hw=men->getNode()->getHeadWord().to_string();
			hwc[hw][men->getEntityType().getName().to_string()]++;
		}
	}
}

void printHeadwordsWithETypes(const HWordCounts& hwc) {
	for ( HWordCounts::const_iterator hwcci=hwc.begin(); hwcci != hwc.end(); hwcci++ ) {
		std::wstring bestTag;
		int bestCount=-1;
		int totalCount=0;
		HWordTypeCounts::const_iterator hwtcci;
		for ( hwtcci = hwcci->second.begin(); hwtcci != hwcci->second.end(); hwtcci++ ) {
			totalCount += hwtcci->second;
			if ( hwtcci->second > bestCount ) {
				bestCount = hwtcci->second;
				bestTag = hwtcci->first;
			}
		}
		std::wcout << hwcci->first << "\t" << bestTag << "\t" << setprecision(1) << (double)(bestCount)/totalCount << "\t" << totalCount << "\n";
	}
}
*/

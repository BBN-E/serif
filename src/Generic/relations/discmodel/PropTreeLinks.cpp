// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/relations/discmodel/PropTreeLinks.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/version.h"

PropTreeLinks::PropTreeLinks(int i, const DocTheory* dt) {
	if ( ! dt || ! ParamReader::isParamTrue("use_proptrees") ) return;

	SNodeContent::Language lang;

//#ifdef ENGLISH_LANGUAGE
	if (SerifVersion::isEnglish()) {
		lang = SNodeContent::ENGLISH;
	} else {
//#else
		lang = SNodeContent::UNKNOWN;
	}
//#endif

	SPropForest forest;
	SForestUtilities::getPropForestFromSentence(dt,dt->getSentence(i),forest,lang);
	//std::wcerr << "\nfor sentence " << i << " extracted forest:\n";
	//SForestUtilities::printForest(forest,std::wcerr);

	SPropForest::const_iterator pfci;
	for ( pfci=forest.begin(); pfci != forest.end(); pfci++ ) {
		/*std::wcerr << "\n======TREE:\n" << pfci->tree->toString() << "\nMENTIONS: ";
		for ( RelevantMentions::const_iterator r=pfci->allEntityMentions.begin(); r != pfci->allEntityMentions.end(); r++ )
			std::wcerr << r->first->getUID() << "(" << r->second.first->getID() << ") "; */
		RelevantMentions::const_iterator rmci1,rmci2;
		for ( rmci1 = pfci->allEntityMentions.begin(); rmci1 != pfci->allEntityMentions.end(); rmci1++ ) {
			rmci2 = rmci1;
			rmci2++;
			for ( ; rmci2 != pfci->allEntityMentions.end(); rmci2++ ) {
				if ( rmci1 == rmci2 || rmci1->first->getIndex() == rmci2->first->getIndex() ) continue;
				links[std::pair<int,int>(rmci1->first->getIndex(),rmci2->first->getIndex())] =
					_new TreeNodeChain(pfci->tree,rmci1->second.first,rmci2->second.first,dictionary);
				//std::wcerr << "\nfor " << this << " compute (" << rmci1->first->getIndex() << "," << rmci2->first->getIndex() << ")";
				//!!! we compute connections in only one direction!!!!
			}
		}
	}
	//all the SPropTree's will go but STreeNode's and SNodeContent's will survive,
	//and thus we can keep using all the PropTreeLinks objects
	SForestUtilities::cleanPropForest(forest); 
}


PropTreeLinks::~PropTreeLinks() {
	AllMentionLinks::iterator amli;
	for ( amli = links.begin(); amli != links.end(); amli++ )
		delete amli->second;
	links.clear();
	SForestUtilities::cleanPropForest(forest);

	TNEDictionary::iterator tnedi;
	for ( tnedi = dictionary.begin(); tnedi != dictionary.end(); tnedi++ )
		delete tnedi->second;
	//!!!!!!! THIS HOWEVER WILL NOT FREE STreeNode's AND SNodeContent's
	//MEMORY POOLS MUST BE CLEANED FOR THAT
}


const TreeNodeChain* PropTreeLinks::getLink(int i, int j) const {
	//std::wcerr << "\nask " << this << " about (" << i << "," << j << ")";
	AllMentionLinks::const_iterator amlci=links.find(std::pair<int,int>(i,j));
	if ( amlci != links.end() ) return amlci->second;
	else amlci=links.find(std::pair<int,int>(j,i));
	return ( amlci != links.end() ? amlci->second : 0 );
}

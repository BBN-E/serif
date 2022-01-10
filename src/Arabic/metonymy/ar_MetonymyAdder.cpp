// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Arabic/metonymy/ar_MetonymyAdder.h"
#include "Generic/common/SymbolHash.h"

#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"

#include "Generic/common/SymbolUtilities.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

using namespace std;


//Symbol ArabicMetonymyAdder::_gpeLoc[8];
//Symbol ArabicMetonymyAdder::_gpeOrg[1];

Symbol ArabicMetonymyAdder::_gpeLoc[] = {ArabicSymbol(L"fy"),
ArabicSymbol(L"mn"),
ArabicSymbol(L"b"),
ArabicSymbol(L"En"),
ArabicSymbol(L"fwq"),
ArabicSymbol(L"dAxl"),
ArabicSymbol(L"xArj"),
ArabicSymbol(L"AlY")
};

Symbol ArabicMetonymyAdder::_gpeOrg[] = {ArabicSymbol(L"f")};

ArabicMetonymyAdder::ArabicMetonymyAdder() : _debug(L"metonymy_debug")  {

	_use_gpe_roles = ParamReader::isParamTrue("use_gpe_roles");
	string metonymyDir = ParamReader::getRequiredParam("metonymy_dir");
	string file = metonymyDir+"/cities.txt";
	_cities = _new SymbolHash(100);
	loadSymbolHash(_cities, file.c_str());

}
void ArabicMetonymyAdder::addMetonymyTheory(const MentionSet *mentionSet,
								 const PropositionSet *propSet)
{
//	std::cout<<"add Metonymy Theory: "<<_use_gpe_roles<<std::endl;
	if(!_use_gpe_roles)
		return;
	int nmentions = mentionSet->getNMentions();
	Mention* thisment = 0;
	const SynNode* thisNode = 0;
	const SynNode* prevTerm = 0;
	for(int i=0; i< nmentions; i++){
		thisment = mentionSet->getMention(i);
		if(!thisment->getEntityType().matchesGPE()){
			continue;
		}
		if(_hasLocRole(thisment)){
			thisment->setRoleType(EntityType::getLOCType());
			//thisment->setMetonymyMention();
		}
		else if(_hasOrgRole(thisment, true)){
			thisment->setRoleType(EntityType::getORGType());
			//thisment->setMetonymyMention();
		}
		else{
			thisment->setRoleType(EntityType::getGPEType());
		}

	}


}
bool ArabicMetonymyAdder::_hasLocRole(const Mention *ment){
	const SynNode* thisNode = ment->getNode();
	const SynNode* prevTerm = thisNode->getPrevTerminal();
	for(int j=0; j <_nloc; j++){
		if((prevTerm !=0) &&(prevTerm->getHeadWord() == _gpeLoc[j]))
			return true;
	}
	return false;
}
bool ArabicMetonymyAdder::_hasOrgRole(const Mention* ment, bool checkedForLoc){

	const SynNode* thisNode = ment->getNode();
	//const SynNode* thisNode = ment->getHead();
	const SynNode* prevTerm = thisNode->getPrevTerminal();
	for(int j=0; j <_norg; j++){
		if((prevTerm !=0) &&(prevTerm->getHeadWord() == _gpeOrg[j])){
			return true;
		}
	}
	if(!checkedForLoc)
		return false;
	if(ment->getMentionType() == Mention::NAME){
		Symbol hw = Symbol(L"NONE");
		
		const Mention* menChild = ment;
		const SynNode* head = menChild->node;
		while((menChild = menChild->getChild()) != 0)
			head = menChild->node;

		if(head == 0){
		}
		else{
			hw = SymbolUtilities::getFullNameSymbol(head);
		}
		if(hw == ArabicSymbol(L"wA$nTn")){
			if(prevTerm == 0){
				return true;
			} 
			else {
				if (!( (prevTerm->getHeadWord()== ArabicSymbol(L"b")) ||
				(prevTerm->getHeadWord()== ArabicSymbol(L"l")))){
					return true;
				}
			}
		}
		if(_cities->lookup(hw) != 0)
			return true;
	}

	return false;
}
void ArabicMetonymyAdder::loadSymbolHash(SymbolHash *hash, const char* file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	if (stream.fail()) {
		string err = "Problem opening ";
		err.append(file);
		throw UnexpectedInputException("ArabicMetonymyAdder::loadSymbolHash()",
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

	if (!stream.eof() && stream.fail()) {
		char buffer[500];
		sprintf(buffer, "Error reading file: %s", file);
		throw UnexpectedInputException("ArabicMetonymyAdder::loadSymbolHash()", buffer);
	}
	stream.close();

}

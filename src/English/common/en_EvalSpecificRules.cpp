// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/common/en_EvalSpecificRules.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/discmodel/PIdFSentence.h"


#ifdef _WIN32
	#define swprintf _snwprintf
#endif


using namespace std;

void EnglishEvalSpecificRules::NamesToNominals(const SynNode* root, Mention *ment, EntityType &etype){
	//getting a parse node
	const SynNode* ment_node = ment->getNode();
	bool is_desc = false;
	if (ment_node->getStartToken() == ment_node->getEndToken()) {
		int head_token = ment_node->getHead()->getStartToken();
		Symbol terminals[500];
		root->getTerminalSymbols(terminals, 500);
		std::wstring temp = terminals[head_token].to_string();
		std::transform(temp.begin(), temp.end(), temp.begin(), towlower);
		Symbol lc_word = Symbol(temp.c_str());
		if ((lc_word == Symbol(L"state")) || (lc_word == Symbol(L"defense")))  {
			if (head_token > 0) {
				temp = terminals[head_token-1].to_string();
				std::transform(temp.begin(), temp.end(), temp.begin(), towlower);
				lc_word = Symbol(temp.c_str());
				if (lc_word == Symbol(L"of")) {
					ment->mentionType = Mention::DESC;
					ment->setEntityType(EntityType::getORGType());
					is_desc = true;
				}
			}
		} 
	} 
	if (!is_desc) {
		ment->mentionType = Mention::NAME;
		ment->setEntityType(etype);
	}
}

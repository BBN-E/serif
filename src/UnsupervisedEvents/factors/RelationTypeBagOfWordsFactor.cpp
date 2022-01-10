#include <string>
#include <algorithm>
#include "Generic/common/leak_detection.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "English/parse/en_LanguageSpecificFunctions.h"
#include "RelationTypeBagOfWordsFactor.h"

using std::wstring;

void RelationTypeBagOfWordsFactor::gatherWords(const SynNode* node) {
	static const Symbol NN(L"NN");
	static const Symbol NNS(L"NNS");
	static const Symbol NNP(L"NNNP");
	static const Symbol NNPS(L"NNPS");

	if (node->isPreterminal()) {
		Symbol tag = node->getTag();
		if (EnglishLanguageSpecificFunctions::isNPtypePOStag(tag)
				|| EnglishLanguageSpecificFunctions::isVerbPOSLabel(tag)
				|| EnglishLanguageSpecificFunctions::isPreplikePOSLabel(tag)
				|| EnglishLanguageSpecificFunctions::isAdjective(tag))
		{
			const SynNode* term = node->getChild(0);
			assert(term);
			wstring termString = term->getTag().to_string();
			std::transform(termString.begin(), termString.end(), 
					termString.begin(), ::tolower);
			addFeature(termString);
		}
	} else {
		for (int i=0; i<node->getNChildren(); ++i) {
			gatherWords(node->getChild(i));
		}
	}
}



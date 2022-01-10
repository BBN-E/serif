// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/BuckWalter/ar_BuckWalterFunctions.h"

Symbol BuckwalterFunctions::ANALYZED = Symbol(L"ANALYZED");
Symbol BuckwalterFunctions::UNKNOWN = Symbol(L"UNKNOWN");
Symbol BuckwalterFunctions::NULL_SYMBOL = Symbol(L"NULL");
Symbol BuckwalterFunctions::RETOKENIZED = Symbol(L"RETOKENIZED");
Symbol BuckwalterFunctions::NON_ARABIC = Symbol(L"NON_ARABIC");


int BuckwalterFunctions::pullLexEntriesUp(LexicalEntry **result, int size, LexicalEntry* initialEntry)throw (UnrecoverableException)
{
	int numSeg = initialEntry->getNSegments();
	if(numSeg == 0){
		result[0] = initialEntry;
		return 1;
	}
	else{
		int count = 0;
		LexicalEntry* temp_results[20];
		for(int i =0; i<numSeg; i++){
			LexicalEntry* curr_seg = initialEntry->getSegment(i);
			int num_ret = pullLexEntriesUp(temp_results, 20, curr_seg);
			for(int j = 0; j<num_ret; j++){
				result[count++] = temp_results[j];
				if(count >=size){
					throw UnrecoverableException("BuckwalterFunctions::pullLexicalEntriesUp()",
						"number of lexical entries exceeds Max");
				}
			}
		}
		return count;
	}
}

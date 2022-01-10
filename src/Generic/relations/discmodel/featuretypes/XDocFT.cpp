// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SexpReader.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/NameLinkFunctions.h"
#include <boost/algorithm/string.hpp>

#include "Generic/relations/discmodel/featuretypes/XDocFT.h"

DTFeature *XDocFT::makeEmptyFeature() const {
	return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
							  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
							  SymbolConstants::nullSymbol);
}

int XDocFT::extractFeatures(const DTState &state,
							DTFeature **resultArray) const
{
	RelationObservation *o = static_cast<RelationObservation*>(
		state.getObservation(0));



	Symbol tag = state.getTag();
	SymbolArray *m1 = o->getMention1Name();
	SymbolArray *m2 = o->getMention2Name();

	Symbol enttype1 = o->getMention1()->getEntityType().getName();
	Symbol enttype2 = o->getMention2()->getEntityType().getName();

	int n_results = 0;

	Symbol mostFreqType = SymbolConstants::nullSymbol;
	int n_most_freq = 0;
	
	KeyPair pair(*m1, *m2);
	Table::iterator iter = first_then_second->find(pair);
	if (iter != first_then_second->end()) {
		EntryList entryList = (*iter).second;
		int less_frequent_cnt = (entryList.n1 < entryList.n2) ? entryList.n1 : entryList.n2;
		Entry *entry = entryList.entry;
		while (entry != 0) {
			if (wcsstr(tag.to_string(), entry->relType.to_string()) != 0) {

				float percent = entry->nTogether / (float)less_frequent_cnt;

				if (percent >= .25) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype1, enttype2, 
								P1RelationFeatureType::AT_LEAST_25_PERCENT);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								P1RelationFeatureType::AT_LEAST_25_PERCENT);
				}
				if (percent >= .50) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype1, enttype2, 
								P1RelationFeatureType::AT_LEAST_50_PERCENT);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								P1RelationFeatureType::AT_LEAST_50_PERCENT);
				}
				if (percent >= .75) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype1, enttype2, 
								P1RelationFeatureType::AT_LEAST_75_PERCENT);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								P1RelationFeatureType::AT_LEAST_75_PERCENT);

				}
				if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
				resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype1, enttype2, 
								P1RelationFeatureType::AT_LEAST_ONE);
				resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								P1RelationFeatureType::AT_LEAST_ONE);

				if (entry->nTogether > 10) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype1, enttype2, 
								P1RelationFeatureType::AT_LEAST_TEN);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								P1RelationFeatureType::AT_LEAST_TEN);
				}
				if (entry->nTogether > 100) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype1, enttype2, 
								P1RelationFeatureType::AT_LEAST_ONE_HUNDRED);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								P1RelationFeatureType::AT_LEAST_ONE_HUNDRED);
				}
				if (entry->nTogether > 1000) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype1, enttype2, 
								P1RelationFeatureType::AT_LEAST_ONE_THOUSAND);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								P1RelationFeatureType::AT_LEAST_ONE_THOUSAND);
				}

				
			}
			if (entry->nTogether > n_most_freq) {
				n_most_freq = entry->nTogether;
				mostFreqType = entry->relType;
			}
			entry = entry->next;
		}
		if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) {
			SessionLogger::warn("DT_feature_limit") <<"XDocFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		}else{
			resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
							mostFreqType);

			if (wcsstr(tag.to_string(), mostFreqType.to_string()) != 0) {
				resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
							P1RelationFeatureType::MOST_FREQ_TYPE_MATCH);
			}
		}
	}

	mostFreqType = SymbolConstants::nullSymbol;
	n_most_freq = 0;

	iter = second_then_first->find(pair);
	if (iter != second_then_first->end()) {
		EntryList entryList = (*iter).second;
		Entry *entry = entryList.entry;
		while (entry != 0) {
			if (wcsstr(tag.to_string(), entry->relType.to_string()) != 0) {
				if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
				resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							  enttype2, enttype1, 
							  P1RelationFeatureType::AT_LEAST_ONE);
				resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
							  P1RelationFeatureType::AT_LEAST_ONE);

				if (entry->nTogether > 10) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype2, enttype1, 
								P1RelationFeatureType::AT_LEAST_TEN);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
							  P1RelationFeatureType::AT_LEAST_TEN);
				}
				if (entry->nTogether > 100) {
					if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) break;
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
								enttype2, enttype1, 
								P1RelationFeatureType::AT_LEAST_ONE_HUNDRED);
					resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
							  P1RelationFeatureType::AT_LEAST_ONE_HUNDRED);
				}
			}
			if (entry->nTogether > n_most_freq) {
				n_most_freq = entry->nTogether;
				mostFreqType = entry->relType;
			}
			entry = entry->next;
		}
		if (n_results > DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 3) {
			SessionLogger::warn("DT_feature_limit") <<"XDocFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		}else{
			resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
							mostFreqType);

			if (wcsstr(tag.to_string(), mostFreqType.to_string()) != 0) {
				resultArray[n_results++] = _new DTQuadgramFeature(this, tag,
							SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
							P1RelationFeatureType::MOST_FREQ_TYPE_MATCH);
			}
		}
	}

	return n_results;
}

void XDocFT::initializeTable() {
	std::string filename = ParamReader::getRequiredParam("xdoc_relations_file");
	SexpReader reader(filename.c_str());

	first_then_second = _new Table(300);
	second_then_first = _new Table(300);

	const int MAX_ARRAY_SYMBOLS = 16;
	Symbol tmpArray[MAX_ARRAY_SYMBOLS];
	int nTmpArray = 0;
	int first_count, second_count, type_count;
	UTF8Token token;

	_debugOut.init(Symbol(L"xdoc_relations_feature_debug_file"));
	while(reader.hasMoreTokens()) {
		reader.getLeftParen();  // open main entry 
		
		nTmpArray = reader.getSymbolArray(tmpArray, MAX_ARRAY_SYMBOLS);
		_debugOut << "found FIRST: ";
		for (int i=0; i<nTmpArray; i++) {
			const wchar_t *orig = tmpArray[i].to_string();
			size_t len = wcslen(orig);
			wchar_t *lwr = _new wchar_t[len+1];
			wcsncpy(lwr,orig,len+1);
#if defined(_WIN32)
			_wcslwr(lwr);
#else
			boost::to_lower(lwr);
#endif
			tmpArray[i] = Symbol(lwr);
			_debugOut << tmpArray[i].to_debug_string() << " ";
		}
		_debugOut << "\n";
		SymbolArray firstArray(tmpArray, nTmpArray);
		
		token = reader.getWord();
		first_count = _wtoi(token.chars());
		_debugOut << "	FIRST COUNT: " << first_count << "\n";

		nTmpArray = reader.getSymbolArray(tmpArray, MAX_SYMBOLS);
		_debugOut << "	SECOND: ";
		for(int j=0; j<nTmpArray; j++)  {
			const wchar_t *orig = tmpArray[j].to_string();
			size_t len = wcslen(orig);
			wchar_t *lwr = _new wchar_t[len+1];
			wcsncpy(lwr,orig,len+1);
#if defined(_WIN32)
			_wcslwr(lwr);
#else
			boost::to_lower(lwr);
#endif
			tmpArray[j] = Symbol(lwr);
			_debugOut << tmpArray[j].to_debug_string() << " ";
		}
		_debugOut << "\n";
		SymbolArray secondArray(tmpArray, nTmpArray);

		token = reader.getWord();
		second_count = _wtoi(token.chars());
		_debugOut << "	SECOND COUNT: " << second_count << "\n";

		token = reader.getWord();
		Symbol type = token.symValue();
		_debugOut << "	TYPE: " << type.to_debug_string() << "\n";

		token = reader.getWord();
		type_count = _wtoi(token.chars());
		_debugOut << "	TYPE COUNT: " << type_count << "\n";

		reader.getRightParen();  // close main entry

		KeyPair firstThenSecond(firstArray, secondArray);
		Entry *entryOne = _new Entry;
		entryOne->nTogether = type_count;
		entryOne->relType = type;
		entryOne->next = 0;

		Table::iterator iter1 = first_then_second->find(firstThenSecond);
		if (iter1 != first_then_second->end()) {
			EntryList entryList = (*iter1).second;
			Entry *tmp = entryList.entry;
			entryOne->next = tmp;
			entryList.entry = entryOne;
		}
		else {
			EntryList entryList;
			entryList.n1 = first_count;
			entryList.n2 = second_count;
			entryList.entry = entryOne;
			(*first_then_second)[firstThenSecond] = entryList;
		}

		KeyPair secondThenFirst(secondArray, firstArray);
		Entry *entryTwo = _new Entry;
		entryTwo->nTogether = type_count;
		entryTwo->relType = type;
		entryTwo->next = 0;

		Table::iterator iter2 = second_then_first->find(secondThenFirst);
		if (iter2 != second_then_first->end()) {
			EntryList entryList = (*iter2).second;
			Entry *tmp = entryList.entry;
			entryTwo->next = tmp;
			entryList.entry = entryTwo;
		}
		else {
			EntryList entryList;
			entryList.n1 = second_count;
			entryList.n2 = first_count;
			entryList.entry = entryTwo;
			(*second_then_first)[secondThenFirst] = entryList;
		}

		
	}
}


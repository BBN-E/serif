// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTrainer/FeatureVectorTable.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include <string>
#include "Generic/common/NgramScoreTable.h"
#include "math.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnexpectedInputException.h"

const float FeatureVectorTable::targetLoadingFactor = static_cast<float>(0.7);
FeatureVectorTable::FeatureVectorTable(UTF8InputStream& posin, VocabPruner* pruner ){
	
	int numPOSEntries;
    posin >> numPOSEntries;
    _table = _new Table(numPOSEntries);
	_size = 0;
	_wordFeat = WordFeatures::build();
	Symbol word;
	Symbol features;
	int numTags;
	Symbol first_word_yes = Symbol(L"H1");
	UTF8Token token;
	VocabularyTable* prunedVocab = pruner->vocabularyTable;
	for(int i = 0; i<numPOSEntries; i++){
        //read a line- this is mostly copied from PrunerPOS 
		posin >> token;
        if (posin.eof()) return;
        if (wcscmp(token.chars(), L"(") != 0)
			throw UnexpectedInputException("FeatureVectorTable::FeatureVectorTable()",
                "ERROR: problem parsing part-of-speech input file");
        posin >> token;
		word = token.symValue();
        posin >> token;
	    features = token.symValue();
        posin >> numTags;
        for (int j = 0; j < numTags; j++) {
            posin >> token;
			posin >> token;
        }
        posin >> token;

		if (wcscmp(token.chars(), L")") != 0)
			throw UnexpectedInputException("FeatureVectorTable::FeatureVectorTable()",
                "ERROR: problem parsing part-of-speech input file");
		//if word has been pruned add wordfeatures, word to table
		if(!prunedVocab->find(word)){
			Symbol word_features = word;
			word_features = _wordFeat->features(word_features, features==first_word_yes);
			Add(word_features, word);
		}
	}
}
void FeatureVectorTable::Add(Symbol feature, Symbol word){
	//new feature
	LinkedTag* old_words = Lookup(feature); 
	if( old_words == 0 ){
		LinkedTag* new_word = _new LinkedTag(word);
		(*_table)[feature] = new_word;
		_size++;
	}
	//feature in table, add word if necessary
	else{
		LinkedTag* iter = old_words;
		bool found_word = false;
		while(iter != 0){
			if(iter->tag==word){
				found_word = true;
				break;
			}
			iter = iter->next;
		}
		if(!found_word){
			LinkedTag* new_word = _new LinkedTag(word);
			new_word->next = old_words;
			(*_table)[feature] = new_word;
		}
	}
}

LinkedTag* FeatureVectorTable::Lookup(Symbol feature){
	Table::iterator iter;
    iter = _table->find(feature);
    if (iter == _table->end()) {
        return 0;
    }
    return (*iter).second;
}

void FeatureVectorTable::Print(char *filename){
	UTF8OutputStream debugOut;
	NgramScoreTable* feat_table = _new NgramScoreTable(1,_size);
	Symbol ngram[1];
    char filename2[501];
 	sprintf(filename2, "%s.vectors", filename);
	debugOut.open(filename2);
	debugOut <<_size;
	debugOut <<'\n';
	Table::iterator iter;
	for (iter = _table->begin() ; iter != _table->end() ; ++iter) {
		//count the number of unknown words that have this vector
		// also print the details of the Unknown word vector to .vector
		debugOut << "(";
		debugOut << (*iter).first.to_string();
		LinkedTag* value = (*iter).second;
		int count = 1;
		while (value->next != 0)
		{
			value = value->next;
			count++;
		}
		debugOut << ' ';
		debugOut << count;
		debugOut<<' ';
		debugOut <<static_cast<double>(1/count);;
		debugOut<<' ';
		debugOut<<log((static_cast<double>(1))/count);
		debugOut<<' ';
		value = (*iter).second;
		while (value != 0) {
			debugOut << ' ';
			debugOut << value->tag.to_string();
			value = value->next;
		}
		debugOut << ")\n";		
		
		//add the log prob of the feature vector to the feat_table
		double logprob = log((static_cast<double>(1))/count);
		ngram[0] = (*iter).first;
		
		if(feat_table->lookup(ngram)!=0)
            throw UnexpectedInputException("FeatureVectorTable::Print()",
               "ERROR: problem building feature ngram table");
		feat_table->add(ngram, static_cast<float>(logprob));
	}
	//print the feat_table.  This is the table that will be used in decoding
	feat_table->print(filename);
	delete feat_table;
	debugOut.close();
}



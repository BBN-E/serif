// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "ExtractBigrams.h"

const int NUM_VOCAB_BUCKETS = 150000;
const int NUM_BIGRAM_BUCKETS = 15000000;

ExtractBigrams::ExtractBigrams() {
	voc = _new VocMap(NUM_VOCAB_BUCKETS);
	bigramCount = _new BigramCount(NUM_BIGRAM_BUCKETS);
	vMax = 0;
}

ExtractBigrams::~ExtractBigrams() {
	delete voc;
	delete bigramCount;
}

int ExtractBigrams::lookupVocCode(wstring& str, VocMap * vocMap, int& vocMax) {
    VocMap::iterator iter = vocMap->find(str);
    if (iter == vocMap->end()) {
        (*vocMap)[str] = vocMax;
        return vocMax++;
    } else {
        return (*iter).second;
    }
}

void ExtractBigrams::addBigram(Bigram& bigram) {
    BigramCount::iterator iter = bigramCount->find(bigram);
    if (iter == bigramCount->end()) {
        (*bigramCount)[bigram] = 1;
    } else {
        ++((*iter).second);
    }
}

void ExtractBigrams::printVocabulary(VocMap * vocMap, int vocMax, wofstream& out) {
    wstring* str = new wstring[vocMax];
    for (VocMap::iterator iter = vocMap->begin();
         iter != vocMap->end(); ++iter) 
    {
        str[(*iter).second] = (*iter).first;
    }
    for (int i = 0; i < vocMax; i++) {
        out << str[i] << endl;
    }
    delete [] str;
}

vector <wstring> ExtractBigrams::getVocabulary() {
	wstring* str = new wstring[vMax];
	for (VocMap::iterator iter = voc->begin(); iter != voc->end(); ++iter) {
        str[(*iter).second] = (*iter).first;
    }

	vector <wstring> vocab;
	for (int i = 0; i < vMax; i++) {
		vocab.push_back(str[i]);
    }
	delete [] str;
	return vocab;
}

void ExtractBigrams::printBigrams(ofstream& out) {
	for (BigramCount::iterator iter = bigramCount->begin();
         iter != bigramCount->end(); ++iter) 
    {
        out << (*iter).first._h << ' ' << (*iter).first._f 
        << ' ' << (*iter).second << endl;
    }
}

ExtractBigrams::BigramCount * ExtractBigrams::getBigrams() {
	return bigramCount;
}

void ExtractBigrams::extractBigrams(vector <wstring> tokens) {
	tokens.insert(tokens.begin(), L"---NULL---");
	tokens.push_back(L"---NULL---");
	if (tokens.size() > 1) {
		for (size_t i = 0; i < tokens.size() - 1; i++) {
			int hist = lookupVocCode(tokens[i], voc, vMax);
			int fut = lookupVocCode(tokens[i + 1], voc, vMax);
			Bigram b(hist, fut);
			addBigram(b);
		}
	}
}

int ExtractBigrams::printFiles(string prefix) {
	string bigramFile = prefix + ".bigram";
    string vocFile = prefix + ".voc";

	ofstream bigramOut(bigramFile.c_str());
    if (!bigramOut) { 
        cout << "Unable to open file: " << bigramFile << endl;
        return 1;
    }
    printBigrams(bigramOut);
    bigramOut.close();
    
    wofstream vocOut(vocFile.c_str());
    if (!vocOut) { 
        cout << "Unable to open file: " << vocFile << endl;
        return 1;
    }
    printVocabulary(voc, vMax, vocOut);
    vocOut.close();
	return 0;
}

/*
int main(int argc, char* argv[]) 
{
    if (argc != 3) {
        cout << "Usage: ExtractBigrams InputFile OutputPrefix" << endl;
        return 1;
    }
    
    string prefix = argv[2];
    string bigramFile = prefix + ".bigram";
    string vocFile = prefix + ".voc";
    
    wifstream in(argv[1]);
    if (!in) {
        cout << "Unable to open file: " << argv[1] << endl;
        return 1;
    }
    while (in) {
        wstring line;
        getline(in, line);
        line = L"---NULL--- " + line + L" ---NULL---";
        vector<wstring> token;
        WSTokenizer::tokenize(line, token);
        if (token.size() > 1) {
            for (size_t i = 0; i < token.size() - 1; i++) {
                int hist = lookupVocCode(token[i], voc, vMax);
                int fut = lookupVocCode(token[i + 1], voc, vMax);
                Bigram b(hist, fut);
                addBigram(b);
            }
        }
    }
    in.close();
    
    ofstream bigramOut(bigramFile.c_str());
    if (!bigramOut) { 
        cout << "Unable to open file: " << bigramFile << endl;
        return 1;
    }
    printBigrams(bigramOut);
    bigramOut.close();
    
    wofstream vocOut(vocFile.c_str());
    if (!vocOut) { 
        cout << "Unable to open file: " << vocFile << endl;
        return 1;
    }
    printVocabulary(voc, vMax, vocOut);
    vocOut.close();
}
*/

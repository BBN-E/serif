// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include <iostream>
#include "MICluster.h"

const int NUM_MIDDLE_CLASSES = 500;

MICluster::MICluster() {
}

MICluster::~MICluster() {
}

void MICluster::loadVocabulary(vector <wstring> elements) {
	t.open(elements);
}

void MICluster::loadBigram(int hist, int fut, int count) {
	t.loadBigram(hist, fut, count);
}

void MICluster::doClusters(string bitsFile) {
	t.clusterHFMiddleOut(NUM_MIDDLE_CLASSES, bitsFile+".middle_classes.txt");
    t.printBits(MIClassTable::HISTIDX, bitsFile);
}

/*
int main(int argc, char* argv[]) {
    
    MIClassTable t;
    if (argc != 2) {
        cout << "Usage: MICluster prefix" << endl;
        return 1;
    }
    string prefix = argv[1];
    string vocFile = prefix + ".voc";
    string bigramFile = prefix + ".bigram";
    string bitsFile = prefix + ".hBits";
    t.open(vocFile);
    t.loadEvents(bigramFile);
    t.clusterHFMiddleOut(NUM_MIDDLE_CLASSES);
    t.printBits(MIClassTable::HISTIDX, bitsFile);
    return 0;
}
*/

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include <iostream>
#include <list>
#include <algorithm>
#include <ctime>
#include "MIClassTable.h"
#include "NLogN.h"

using namespace std;

double MIClassTable::maxSplitRatio = 10;
int MIClassTable::minClassSize = 100;
int MIClassTable::maxReclassIters = 20;
double MIClassTable::minReclassGain = 0;

void MIClassTable::clusterMiddleOut(int hN, int fN) {
    sortWords(HISTIDX);
    time_t t;
    time(&t); 
    cout << ctime(&t);
    cout << "clustering histories" << endl;
    clusterMiddle(HISTIDX, hN);
    sortWords(FUTIDX);
    time(&t);
    cout << ctime(&t);
    cout << "clustering futures" << endl;
    clusterMiddle(FUTIDX, fN);
    setupClasses();
    time(&t);
    cout << ctime(&t);
    cout << "shuffling words" << endl;
    shuffleWords(maxReclassIters, minReclassGain);
    time(&t);
    cout << ctime(&t);
    cout << "splitting classes" << endl;
    for (int i = 0; i < hN; i++) {
        recursivelySplitClass(HISTIDX, i);
    }
    for (int i = 0; i < fN; i++) {
        recursivelySplitClass(FUTIDX, i);
    }
    time(&t);
    cout << ctime(&t);
    cout << "merging classes" << endl;
    clearFrontier(HISTIDX);
    for (int cl = 0; cl < hN; cl++)
        addToFrontier(HISTIDX, cl);
    mergeFrontier(HISTIDX);
    clearFrontier(FUTIDX);
    for (int cl = 0; cl < fN; cl++)
        addToFrontier(FUTIDX, cl);
    mergeFrontier(FUTIDX);
    time(&t);
    cout << ctime(&t);
    cout << "NumHClasses: " << info[HISTIDX].maxClass << endl;
    cout << "NumFClasses: " << info[FUTIDX].maxClass << endl;
}

void MIClassTable::clusterHFMiddleOut(int hN, const std::string& bitsFile) {
    sortWords(HISTIDX);
    time_t t;
    time(&t); 
	time(&cur_time);
    cout << ctime(&t);
    cout << "Clustering histories...";
    clusterMiddle(HISTIDX, hN);
    info[FUTIDX].maxClass = info[HISTIDX].maxClass;
    for (int i = 0; i < info[HISTIDX].maxClass; i++) {
        const list<MIWord*>& wordList = miClass(HISTIDX, i).getWords();
        for (list<MIWord*>::const_iterator j = wordList.begin(); 
             j != wordList.end(); j++)
        {
            MIWord& w = **j;
            miClass(FUTIDX, i).addWord(w);
        }
    }
    setupClasses();
	cout << endl;
    time(&t);
    cout << ctime(&t);
    cout << "Shuffling words...";
    shuffleHFWords(maxReclassIters, minReclassGain);
	//mrf want to add printing here for the middle set of classes
	if(bitsFile != ""){
		UTF8OutputStream out;
		out.open(bitsFile.c_str());
		if (out.fail()) {
		    cerr << "can't open out file" << endl;
			return;
		}
		int n_words = (int)info[HISTIDX].wordToClass.size();
		for(int i =0; i < n_words; i++){
			const std::wstring name = info[HISTIDX].voc->get(i);
			int c = info[HISTIDX].wordToClass[i];
		}
		out.close();

	}
	cout << "\b\b\b100%\n";
    time(&t);
    cout << ctime(&t);
    cout << "Splitting classes...\n";
    for (int i = 0; i < hN; i++) {
        recursivelySplitClass(HISTIDX, i);
    }
    time(&t);
    cout << ctime(&t);
    cout << "Merging classes...\n";
    clearFrontier(HISTIDX);
	for (int cl = 0; cl < hN; cl++) {
        addToFrontier(HISTIDX, cl);
	}
    mergeFrontier(HISTIDX);
    time(&t);
    cout << ctime(&t);
    cout << "NumHClasses: " << info[HISTIDX].maxClass << endl;
}

void MIClassTable::printBits(int hIdx, const std::string& bitsFile) {
    //wofstream out(bitsFile.c_str());
	UTF8OutputStream out;
	out.open(bitsFile.c_str());
	if (out.fail()) {
//    if (!out) {
        cerr << "can't open out file" << endl;
        return;
    }
    MIWord::useVoc(*info[hIdx].voc);
    int c = 0;
    while (!onFrontier(hIdx, c)) {
        c++;
    }
    MIClass& cl = miClass(hIdx, c);
    cl.printBits(L"1", out);
	out.close();
}

void MIClassTable::clusterMiddle(int hIdx, int n) {
    int currentWord = 0;
    setupSeedClasses(hIdx, n, currentWord);
	printDot(cur_time);
    int nextClass = info[hIdx].maxClass++;
    
    addToFrontier(hIdx, nextClass);
	printDot(cur_time);
    allocateLossCache(hIdx);
    removeFromFrontier(hIdx, nextClass);
	printDot(cur_time);
    
    clusterHighFreqWords(hIdx, minClassSize, currentWord, nextClass);
	printDot(cur_time);
    clusterLowFreqWords(hIdx, currentWord, nextClass);
	printDot(cur_time);
    
    if (nextClass != info[hIdx].maxClass - 1) {
        removeFromFrontier(hIdx, info[hIdx].maxClass - 1);
		printDot(cur_time);
        miClass(hIdx, nextClass) = miClass(hIdx, info[hIdx].maxClass - 1);
        addToFrontier(hIdx, nextClass);
    }
	printDot(cur_time);
    miClass(hIdx, info[hIdx].maxClass - 1).clear();
    info[hIdx].maxClass--;
    freeLossCache(hIdx);
	printDot(cur_time);
}

void MIClassTable::shuffleWords(int maxIterations, double minGain) {
    for (int i = 0; i < maxIterations; i++) {
        double gain = reclassMiddle(HISTIDX);
        setupClasses();
        gain += reclassMiddle(FUTIDX);
        setupClasses();
        if (gain <= minGain) {
            break;
        }
    }
}

void MIClassTable::shuffleHFWords(int maxIterations, double minGain) {
	int prev_percentage = -1;
    for (int i = 0; i < maxIterations; i++) {
        double gain = reclassHFMiddle();
        setupClasses();
        if (gain <= minGain) {
            break;
        }
		printPercentage(i, prev_percentage, maxIterations);
    }
}

void MIClassTable::clusterHighFreqWords(int hIdx, int minCount, 
                                        int& currentWord, 
                                        int& nextClass)
{
    int numH = info[hIdx].numWords;
    for (; currentWord < numH; currentWord++)  {
        int w = info[hIdx].sortedWords[currentWord].word;
        if (info[hIdx].sortedWords[currentWord].count < minCount) {
            break;
        }
        miClass(hIdx, nextClass).clear();
        miClass(hIdx, nextClass).addWord(word(hIdx, w));
        clearLossCache(hIdx, nextClass);
        addToFrontier(hIdx, nextClass);
        int best1;
        int best2;
        selectBestMerge(hIdx, best1, best2);
        if (miClass(hIdx, best1).numWords() > miClass(hIdx, best2).numWords()) {
            mergeInto(hIdx, best2, best1);
            nextClass = best2;
        } else {
            mergeInto(hIdx, best1, best2);
            nextClass = best1;
        }
    }
}

void MIClassTable::clusterLowFreqWords(int hIdx, int& currentWord,
                                       int& nextClass)
{
    int numH = info[hIdx].numWords;
    for (; currentWord < numH; currentWord++)  {
        int w = info[hIdx].sortedWords[currentWord].word;
        miClass(hIdx, nextClass).clear();
        miClass(hIdx, nextClass).addWord(word(hIdx, w));
        clearLossCache(hIdx, nextClass);
        addToFrontier(hIdx, nextClass);
        int best;
        selectBestSingleMerge(hIdx, nextClass, best);
        mergeInto(hIdx, nextClass, best);
    }
}

double MIClassTable::reclassMiddle(int hIdx) {
    double gain = 0.0;
    for (int i = 0; i < info[hIdx].numWords; i++) {
        int w = info[hIdx].sortedWords[i].word;
        int c1 = info[hIdx].wordToClass[w];
        if (miClass(hIdx, c1).numWords() == 1) {
            continue;
        }
        double removeChange = miClass(hIdx, c1).removeMIChange(cWord(hIdx, w));
        double maxChange = 0;
        int maxcl = c1;
        for (int c = 0 ; c < info[hIdx].maxClass; c++) {
            if (c != c1) {
                double change = removeChange + 
                miClass(hIdx, c).addMIChange(cWord(hIdx, w));
                if (change > maxChange) {
                    maxChange = change;
                    maxcl = c;
                }
            }
        }
        if (maxcl != c1) {
            gain += maxChange / static_cast<double>(globalTotal);
            miClass(hIdx, c1).removeWord(cWord(hIdx, w));
            miClass(hIdx, maxcl).addWord(cWord(hIdx, w));
            info[hIdx].wordToClass[w] = maxcl;
        }
    }
    return gain;
}

double MIClassTable::reclassHFMiddle()
{
    double gain = 0.0;
    int numWords = info[HISTIDX].numWords;
    int numClasses = info[HISTIDX].maxClass;
    int* classCount = new int[numClasses];
    MICountClass* dClassCount = new MICountClass[numClasses];
    for (int cwi = 0; cwi < numWords; cwi++) {		
        int w = info[HISTIDX].sortedWords[cwi].word;
        int c1 = info[HISTIDX].wordToClass[w];
        if (miClass(HISTIDX, c1).numWords() == 1) {
            continue;	
        }
        for (int j = 0; j < numClasses; j++) {
            classCount[j] = 0;
        }
        MIWord& fw = word(FUTIDX, w);
        int wordAsFutureCount = fw.getTotalCount();
        int numFC = fw.getNumFutures();
        for (int i = 0; i < numFC; i++) {
            classCount[info[HISTIDX].wordToClass[fw.futureAt(i)]]
            	+= fw.countAt(i);
        }
        int dccLen = 0;
        for (int c = 0; c < numClasses; c++) {
            if (classCount[c] > 0) {
                dClassCount[dccLen].miClass = c;
                dClassCount[dccLen].count = classCount[c];
                dccLen++;
            }
        }
        int wwCount = word(HISTIDX, w).getCount(w);
        double removeChange = computeMIRemoveChange(c1, w, classCount,
                                                    wordAsFutureCount, wwCount);
        double maxChange = 0;
        int maxcl = c1;
        for (int c = 0; c < numClasses; c++) {
            if (c1 != c) {
                double moveChange = computeMIMoveChange(c1, c, w, classCount,
                                                dClassCount, dccLen,
                                                wordAsFutureCount, wwCount);
                double change = removeChange + moveChange;
                if (change > maxChange) {
                    maxChange = change;
                    maxcl = c;
                }
            }
        }
        if (maxcl != c1) {
            gain += maxChange / static_cast<double>(globalTotal);
            moveHFWord(w, c1, maxcl);
        }
    }
    delete [] classCount;
    delete [] dClassCount;
    return gain;
}

void MIClassTable::recursivelySplitClass(int hIdx, int cl) {
    if (miClass(hIdx, cl).numWords() > 1) {
        int leftChild = info[hIdx].maxClass++;
        int rightChild = info[hIdx].maxClass++;
        splitClass(hIdx, cl, leftChild, rightChild);
        recursivelySplitClass(hIdx, leftChild);
        recursivelySplitClass(hIdx, rightChild);
    }
}

void MIClassTable::mergeFrontier(int hIdx) {
    allocateLossCache(hIdx);
    while (info[hIdx].numOnFrontier > 1) {
        int best1, best2;
        selectBestMerge(hIdx, best1, best2);
        mergeIntoNew(hIdx, best1, best2);
    }
    freeLossCache(hIdx);
}

void MIClassTable::setupSeedClasses(int hIdx, int n, int& currentWord) {
    info[hIdx].maxClass = 0;
    int numWords = info[hIdx].numWords;
    for (; (currentWord < numWords) && (info[hIdx].maxClass < n); 
         currentWord++)
    {
        int w = info[hIdx].sortedWords[currentWord].word;
        int nextClass = info[hIdx].maxClass++;
        miClass(hIdx, nextClass).clear();
        miClass(hIdx, nextClass).addWord(word(hIdx, w));
        addToFrontier(hIdx, nextClass);
    }
}

void MIClassTable::sortWords(int idx) {
    for (int i = 0; i < info[idx].numWords; i++) {
        info[idx].sortedWords[i].word = i;
        info[idx].sortedWords[i].count = word(idx, i).getTotalCount();
    }
    sort(info[idx].sortedWords.begin(), info[idx].sortedWords.end(),
         countWordComparator);
}

void MIClassTable::setupClasses() {
    setupWordToClassMap(HISTIDX);
    setupWordToClassMap(FUTIDX);
    reconstructClasses(HISTIDX);
    reconstructClasses(FUTIDX);
}

void MIClassTable::setupWordToClassMap(int idx) {
    for (int i = 0; i < info[idx].maxClass; i++) {
        const list<MIWord*>& wordList = miClass(idx, i).getWords();
        for (list<MIWord*>::const_iterator j = wordList.begin();
             j != wordList.end(); j++)
        {
            MIWord* w = *j;
            info[idx].wordToClass[w->index()] = i;
			printDot(cur_time);
        }
    }
}

void MIClassTable::reconstructClasses(int hIdx) {
    int fIdx = 1 - hIdx;
    int numClasses = info[hIdx].maxClass;
    for (int i = 0; i < numClasses; i++) {
        miClass(hIdx, i).clear();
        miClass(hIdx, i).enableDenseCounts(numClasses);
    }
    int numWords = info[hIdx].numWords;
    for (int i = 0; i < numWords; i++) {
        cWord(hIdx, i).clear();
        MIWord& oldWord = word(hIdx, i);
        MIWord& newWord = cWord(hIdx, i);
        for (int j = 0; j < oldWord.getNumFutures(); j++) {
            int future = oldWord.futureAt(j);
            int count = oldWord.countAt(j);
            newWord.add(info[fIdx].wordToClass[future], count);
        }
        miClass(hIdx, info[hIdx].wordToClass[i]).addWord(newWord);
    }
}

void MIClassTable::splitClass(int hIdx, int pc, int lc, int rc)
{
    MIClass& parent = miClass(hIdx, pc);
    MIClass& leftChild = miClass(hIdx, lc);
    MIClass& rightChild = miClass(hIdx, rc);
    leftChild.setParent(parent);
    rightChild.setParent(parent);
    parent.setLeftChild(leftChild);
    parent.setRightChild(rightChild);
    leftChild.clear();
    rightChild.clear();
    const list<MIWord*>& wordList = parent.getWords();
    list<MIWord*>::const_iterator iter = wordList.begin();
    leftChild.addWord(**iter++);
    rightChild.addWord(**iter++);
    for (; iter != wordList.end(); ++iter) {
        MIWord &word = **iter;
        double deltalw = miClass(hIdx, lc).addMIChange(word);
        double deltarw = miClass(hIdx, rc).addMIChange(word);
        if (!isBalanced(1, (miClass(hIdx, lc).numWords() + 
                            miClass(hIdx, rc).numWords())))
        {
            if (deltarw < deltalw) {
                leftChild.addWord(word);
            } else {
                rightChild.addWord(word);
            }
        } else {
            double deltalr = miClass(hIdx, lc).addMIChange(miClass(hIdx, rc));
            if (deltalw < deltalr) {
                if (deltarw < deltalr) {
                    leftChild.merge(rightChild);
                    rightChild.clear();
                    rightChild.addWord(word);
                } else {
                    leftChild.addWord(word);
                }
            } else {
                if (deltarw < deltalw) {
                    leftChild.addWord(word);
                } else {
                    rightChild.addWord(word);
                }
            }
        }
    }
    balance(leftChild, rightChild);
}

double MIClassTable::computeMIRemoveChange(int c, int w, const int* classCount,
                                           int wordAsFutureCount,
                                           int wwCount)
{
    double removeChange = miClass(HISTIDX, c).removeMIChange(cWord(HISTIDX, w));
    removeChange +=
        (NLogN::val(miClass(FUTIDX, c).getTotalCount()) -
         (NLogN::val(miClass(FUTIDX, c).getTotalCount() - wordAsFutureCount)));
    if (classCount[c] - wwCount > 0) {
        int ccBeforeCt = (miClass(HISTIDX, c).getCount(c) -
                          cWord(HISTIDX, w).getCount(c));
        int ccAfterCt = ccBeforeCt - (classCount[c] - wwCount);
        removeChange += NLogN::val(ccAfterCt) - NLogN::val(ccBeforeCt);
    }
    return removeChange;
}

double MIClassTable::computeMIMoveChange(int c1, int c2, int w, 
                                         const int* classCount,
                                         MICountClass* dClassCount, int dccLen,
                                         int wordAsFutureCount, int wwCount)
{
    double addChange = miClass(HISTIDX, c2).addMIChange(cWord(HISTIDX,w));
    addChange +=
        (NLogN::val(miClass(FUTIDX, c2).getTotalCount()) -
         NLogN::val(miClass(FUTIDX, c2).getTotalCount() + wordAsFutureCount));
    double reclassChange = 0.0;
    if (classCount[c1] - wwCount > 0) {
        int c1c2beforect = (miClass(HISTIDX, c1).getCount(c2) -
                            cWord(HISTIDX, w).getCount(c2));
        int c1c2afterct = c1c2beforect + (classCount[c1] - wwCount);
        reclassChange += NLogN::val(c1c2afterct) - NLogN::val(c1c2beforect);
    }
    if (classCount[c2] + wwCount > 0) {
        int c2c1BeforeCt = (miClass(HISTIDX, c2).getCount(c1) +
                            cWord(HISTIDX, w).getCount(c1));
        int c2c1AfterCt = c2c1BeforeCt - (classCount[c2]+wwCount);
        reclassChange += NLogN::val(c2c1AfterCt) - NLogN::val(c2c1BeforeCt);
        
        int c2c2BeforeCt = (miClass(HISTIDX, c2).getCount(c2) +
                            cWord(HISTIDX, w).getCount(c2));
        int c2c2AfterCt = c2c2BeforeCt + (classCount[c2] + wwCount);
        reclassChange += NLogN::val(c2c2AfterCt) - NLogN::val(c2c2BeforeCt);
    }
    for (int i = 0; i < dccLen; i++) {
        int cp = dClassCount[i].miClass;
        int count = dClassCount[i].count;
        if ((cp != c1) && (cp != c2)) {
            reclassChange +=
            	miClass(HISTIDX, cp).reclassMIChange(c1, c2, count);
        }
    }
    return addChange + reclassChange;
}

void MIClassTable::moveHFWord(int w, int c1, int c2) {
    MIWord& fw = word(FUTIDX, w);
    int numFC = fw.getNumFutures();
    for (int i = 0; i < numFC; i++) {
        int h = fw.futureAt(i);
        int ct = fw.countAt(i);
        cWord(HISTIDX, h).remove(c1, ct);
        cWord(HISTIDX, h).add(c2, ct);
        int hClass = info[HISTIDX].wordToClass[h];
        miClass(HISTIDX, hClass).remove(c1, ct);
        miClass(HISTIDX, hClass).add(c2, ct);
    }
    miClass(HISTIDX, c1).removeWord(cWord(HISTIDX, w));
    miClass(HISTIDX, c2).addWord(cWord(HISTIDX, w));
    info[HISTIDX].sortedWords[c1].count -= cWord(HISTIDX, w).getTotalCount();
    info[HISTIDX].sortedWords[c2].count += cWord(HISTIDX,w).getTotalCount();
    info[HISTIDX].wordToClass[w] = c2;
    
    MIWord& hw = word(HISTIDX, w);
    numFC = hw.getNumFutures();
    for (int i = 0; i < numFC; i++) {
        int f = hw.futureAt(i);
        int ct = hw.countAt(i);
        cWord(FUTIDX, f).remove(c1, ct);
        cWord(FUTIDX, f).add(c2, ct);
        int fClass = info[FUTIDX].wordToClass[f];
        miClass(FUTIDX, fClass).remove(c1, ct);
        miClass(FUTIDX, fClass).add(c2, ct);
    }
    miClass(FUTIDX, c1).removeWord(cWord(FUTIDX, w));
    miClass(FUTIDX, c2).addWord(cWord(FUTIDX, w));
    info[FUTIDX].sortedWords[c1].count -= cWord(FUTIDX, w).getTotalCount();
    info[FUTIDX].sortedWords[c2].count += cWord(FUTIDX, w).getTotalCount();
    info[FUTIDX].wordToClass[w] = c2;
}

bool MIClassTable::isBalanced(double leftCount, double rightCount) {
    double ratio = (leftCount > rightCount) ? 
    	leftCount / rightCount : rightCount / leftCount;
    return ratio <= maxSplitRatio;
}

void MIClassTable::balance(MIClass& left, MIClass& right) {
    double numLeft = left.numWords();
    double numRight = right.numWords();
    if (numLeft > numRight) {
        balance(right, left);
        return;
    }
    if (isBalanced(left.numWords(), right.numWords())) {
        return;
    }
    while (!isBalanced(left.numWords(), right.numWords()))  {
        double maxChange = -HUGE_VAL;
        MIWord* maxWord = 0;
        const list<MIWord*>& rightWords = right.getWords();
        for(list<MIWord*>::const_iterator i = rightWords.begin();
            i != rightWords.end(); i++)
        {
            MIWord& word = **i;
            double change = right.removeMIChange(word) + 
                left.addMIChange(word);
            if (change > maxChange) {
                maxChange = change;
                maxWord = &word;
            }
        }
        right.removeWord(*maxWord);
        left.addWord(*maxWord);
        numRight--;
        numLeft++;
    }
}

void MIClassTable::addToFrontier(int idx, int cl) {
    info[idx].frontier[cl] = true;
    info[idx].numOnFrontier++;
}

void MIClassTable::clearFrontier(int hIdx)
{
    info[hIdx].numOnFrontier = 0;
    for (int i = 0; i < info[hIdx].numWords * 2; i++) {
        info[hIdx].frontier[i] = false;
    }
}

void MIClassTable::removeFromFrontier(int idx, int cl) {
    info[idx].frontier[cl] = false;
    info[idx].numOnFrontier--;
}

void MIClassTable::allocateLossCache(int hIdx)
{
    int num = 0;
    for (int i = 0; i < info[hIdx].maxClass; i++) {
        if (onFrontier(hIdx, i)) {
            miClass(hIdx, i).setIndex(num++);
        }
    }
    int dim = info[hIdx].numOnFrontier;
    info[hIdx].lossCacheSize = dim;
    info[hIdx].lossCache = new double *[dim];
    for (int i = 0; i < dim; i++)  {
        info[hIdx].lossCache[i] = new double[dim];
        for (int j = 0; j < dim; j++) {
            info[hIdx].lossCache[i][j] = -1;
        }
    }
}

void MIClassTable::clearLossCache(int hIdx, int cl)
{
    int item = miClass(hIdx, cl).index();
    for (int i = 0; i < info[hIdx].lossCacheSize; i++)
    {
        info[hIdx].lossCache[item][i] = -1;
        info[hIdx].lossCache[i][item] = -1;
    }
}

void MIClassTable::freeLossCache(int hIdx)
{
    int size = info[hIdx].lossCacheSize;
    if (size == 0) {
        return;
    }
    for (int i = 0; i < size; i++) {
        delete [] info[hIdx].lossCache[i];
    }
    delete [] info[hIdx].lossCache;
    info[hIdx].lossCache = 0;
    info[hIdx].lossCacheSize = 0;
}

double MIClassTable::selectBestMerge(int hIdx, int& best1, int& best2) {
    int maxClass = info[hIdx].maxClass;
    double minLoss = HUGE_VAL;
    for (int c1 = 0; c1 < maxClass; c1++) {
        if (onFrontier(hIdx, c1)) {
            for (int c2 = c1 + 1 ; c2 < maxClass; c2++) {
                if (onFrontier(hIdx, c2)) {
                    double loss = computeMergeLoss(hIdx, c1, c2);
                    if (loss < minLoss) {
                        minLoss = loss;
                        best1 = c1;
                        best2 = c2;
                    }
                }
            }
        }
    }
    return minLoss;
}

double MIClassTable::selectBestSingleMerge(int hIdx, int classToMerge,
                                           int& best) 
{
    double minLoss = HUGE_VAL;
    for (int c1 = 0; c1 < info[hIdx].maxClass; c1++) {
        if (onFrontier(hIdx, c1) && (c1 != classToMerge)) {
            double loss = computeMergeLoss(hIdx, c1, classToMerge);
            if (loss < minLoss)	{
                minLoss = loss;
                best = c1;
            }
        }
    }
    return minLoss;
}

double MIClassTable::computeMergeLoss(int hIdx, int c1, int c2) {
    int idx1 = miClass(hIdx, c1).index();
    int idx2 = miClass(hIdx, c2).index();
    double loss = info[hIdx].lossCache[idx1][idx2];
    if (loss < 0) {
        info[hIdx].lossCache[idx1][idx2] =
        - miClass(hIdx, c1).addMIChange(miClass(hIdx, c2));
        loss = info[hIdx].lossCache[idx1][idx2];
    }
    return loss;
}

void MIClassTable::mergeInto(int hIdx, int c1, int c2) {
    miClass(hIdx, c2).merge(miClass(hIdx, c1));
    removeFromFrontier(hIdx, c1);
    clearLossCache(hIdx, c2);
}

int MIClassTable::mergeIntoNew(int hIdx, int c1, int c2)
{
    int newClass = info[hIdx].maxClass++;
    miClass(hIdx, newClass).merge(miClass(hIdx, c1), miClass(hIdx, c2));
    miClass(hIdx, newClass).setIndex(miClass(hIdx, c1).index());
    removeFromFrontier(hIdx, c1);
    removeFromFrontier(hIdx, c2);
    addToFrontier(hIdx, newClass);
    clearLossCache(hIdx, c1);
    return newClass;
}

void MIClassTable::open(const string& vocFile) {
    globalTotal = 0;
    Vocabulary* voc = new Vocabulary(vocFile);
    info[HISTIDX].voc = voc;
    info[FUTIDX].voc = voc;
    initInfo(HISTIDX);
    initInfo(FUTIDX);
}

void MIClassTable::open(vector <wstring> elements) {
    globalTotal = 0;
    Vocabulary* voc = new Vocabulary(elements);
    info[HISTIDX].voc = voc;
    info[FUTIDX].voc = voc;
    initInfo(HISTIDX);
    initInfo(FUTIDX);
}

void MIClassTable::loadEvents(const string& eventsFile) {
    ifstream in(eventsFile.c_str());
    if (!in) {
        throw string("Unable to open events file: " + eventsFile);
    }
    while (in) {
        int hist, fut, count;
        in >> hist >> fut >> count;
        globalTotal += count;
        word(HISTIDX, hist).add(fut, count);
        word(FUTIDX, fut).add(hist, count);
    }
}

void MIClassTable::loadBigram(int hist, int fut, int count) {
	globalTotal += count;
    word(HISTIDX, hist).add(fut, count);
	word(FUTIDX, fut).add(hist, count);
}

void MIClassTable::initInfo(int idx) {
    int numWords = info[idx].voc->numElements();
    info[idx].numWords = numWords;
    info[idx].words.reserve(numWords);
    info[idx].cWords.reserve(numWords);
    for (int i = 0; i < numWords; i++) {
        info[idx].words.push_back(new MIWord(i));
        info[idx].cWords.push_back(new MIWord(i));
    }
    info[idx].sortedWords.resize(numWords);
    info[idx].wordToClass.resize(numWords);
    info[idx].maxClass = 0;
    info[idx].classes.reserve(numWords * 2);
    for (int i = 0; i < numWords * 2; i++) {
        info[idx].classes.push_back(new MIClass(i));
    }
    info[idx].frontier = new bool[numWords * 2];
    for (int i = 0; i < info[idx].numWords * 2; i++) {
        info[idx].frontier[i] = false;
    }
    info[idx].numOnFrontier = 0;
}

bool MIClassTable::countWordComparator(const MICountWord& w1,
                                       const MICountWord& w2)
{
    if (w1.count > w2.count) {
        return true;
    }
    if (w1.count < w2.count) { 
        return false;
    }
    return (w1.word > w2.word);
}

void MIClassTable::printDot(time_t &t) {
	time_t cur_time;
	time(&cur_time);
	if (cur_time - t >= 60) {
		cout << ".";
		t = cur_time;
	}
}

void MIClassTable::printPercentage(int cur_count, int &prev_percentage, int total_count) {
	int percentage = int((cur_count / float(total_count)) * 100);
	if (percentage > prev_percentage) {
		if (percentage > 0)
			cout << "\b\b";
		if (percentage > 10)
			cout << "\b";
		cout << percentage << "%";
		prev_percentage = percentage;
	}
}

MIClass& MIClassTable::miClass(int idx, int c) const {
	if (int(info[idx].classes.size()) > c)
		return *info[idx].classes[c];
	else
		throw "Number of classes is too small. Increase your input data size.";
}

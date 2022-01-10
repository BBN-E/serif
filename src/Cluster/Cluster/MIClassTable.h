// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MI_CLASS_TABLE_H
#define MI_CLASS_TABLE_H

#include <vector>
#include <string>
#include <fstream>
#include "MIWord.h"
#include "MIClass.h"
#include "Vocabulary.h"

struct MICountWord {
    int word;
    int count;
};

struct MICountClass {
    int miClass;
    int count;
};

struct MIClassInfo {
    Vocabulary* voc;
    std::vector<MIWord*> words;
    std::vector<MIWord*> cWords;
    std::vector<MICountWord> sortedWords;
    std::vector<MIClass*> classes;
    std::vector<int> wordToClass;
    bool* frontier;
    double** lossCache;
    int numWords;
    int maxClass;
    int numOnFrontier;
    int lossCacheSize;
};

class MIClassTable {
private:
    static double maxSplitRatio;
    static int minClassSize;
    static int maxReclassIters;
    static double minReclassGain;
    int globalTotal;
    MIClassInfo info[2];
public:
    static const int HISTIDX = 0;
    static const int FUTIDX = 1;
    void open(const std::string& vocFile);
	void open(vector <wstring> elements);
    void loadEvents(const std::string& eventsFile);
	void loadBigram(int hist, int fut, int count);
    void clusterMiddleOut(int hN, int fN);
    void clusterHFMiddleOut(int hN, const std::string& bitsFile = "");
    void printBits(int hIdx, const std::string& bitsFile);
private:
    void clusterMiddle(int hIdx, int n);
    void shuffleWords(int maxIterations, double minGain);
    void shuffleHFWords(int maxIterations, double minGain);
    void clusterHighFreqWords(int hIdx, int minCount, int& currentWord,
                              int& nextClass);
    void clusterLowFreqWords(int hIdx, int& currentWord, int& nextClass);
    double reclassMiddle(int hIdx);
    double reclassHFMiddle();
    void recursivelySplitClass(int hIdx, int cl);
    void mergeFrontier(int hIdx);
    void setupSeedClasses(int hIdx, int n, int& currentWord);
    void sortWords(int idx);
    void setupClasses();
    void setupWordToClassMap(int idx);
    void reconstructClasses(int hIdx);
    void splitClass(int hIdx, int pc, int lc, int rc);
    double computeMIRemoveChange(int c, int w, const int* classCount,
                                 int wordAsFutureCount, int wwCount);
    double computeMIMoveChange(int c1, int c2, int w, const int* classCount,
                               MICountClass* dClassCount, int dccLen,
                               int wordAsFutureCount, int wwCount);
    void moveHFWord(int w, int c1, int c2);
    bool isBalanced(double leftCount, double rightCount);
    void balance(MIClass& left, MIClass& right);
    inline bool onFrontier(int idx, int c) const {return info[idx].frontier[c];}
    void addToFrontier(int idx, int cl);
    void removeFromFrontier(int idx, int cl);
    void clearFrontier(int hIdx);
    void allocateLossCache(int hIdx);
    void clearLossCache(int hIdx, int item);
    void freeLossCache(int hIdx);
    double selectBestMerge(int hIdx, int& best1, int& best2);
    double selectBestSingleMerge(int hIdx, int classToMerge, int& best);
    double computeMergeLoss(int hIdx, int c1, int c2);
    void mergeInto(int hIdx, int c1, int c2);
    int mergeIntoNew(int hIdx, int c1, int c2);
    void initInfo(int idx);
    static bool countWordComparator(const MICountWord& w1, 
                                    const MICountWord& w2);
    inline MIWord& word(int idx, int w) const {return *info[idx].words[w];}
    inline MIWord& cWord(int idx, int w) const {return *info[idx].cWords[w];}
	MIClass& miClass(int idx, int c) const;
    
	/*
	inline MIClass& miClass(int idx, int c) const {
			return *info[idx].classes[c];
    }
   */
	time_t cur_time;
	void printDot(time_t &t);
	void printPercentage(int cur_count, int &prev_percentage, int total_count);
};

#endif

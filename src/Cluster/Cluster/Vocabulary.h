// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VOCABULARY_H
#define VOCABULARY_H

#include <string>
#include <vector>

using namespace std;

class Vocabulary {
private:
    static const int DEFAULT_VOCAB_SIZE = 70000;
public:
    Vocabulary(const string& filename);
	Vocabulary(vector <wstring> elems);
    ~Vocabulary() {}
    int numElements() const;
    const wstring& get(int index) const;
private:
    vector<wstring> elements;
};

#endif

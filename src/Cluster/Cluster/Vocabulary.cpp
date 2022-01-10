// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Vocabulary.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

Vocabulary::Vocabulary(const string& filename)
{
    elements.reserve(DEFAULT_VOCAB_SIZE);
    wifstream in(filename.c_str());
    if (!in) {
        throw string("Unable to open vocabulary file: " + filename);
    }
    while (in) {
        wstring line;
        getline(in, line);
        elements.push_back(line);
    }
    in.close();
}


Vocabulary::Vocabulary(vector <wstring> elems)
{
	elements = elems;
}


int Vocabulary::numElements() const {
    return static_cast<int>(elements.size());
}

const wstring& Vocabulary::get(int index) const {
    return elements[index];
}

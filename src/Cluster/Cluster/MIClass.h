// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MI_CLASS_H
#define MI_CLASS_H

#include <list>
#include <string>
#include <fstream>
#include "MIBaseElement.h"
#include "MIWord.h"
#include "Generic/common/UTF8OutputStream.h"

class MIClass : public MIBaseElement {
public:
    MIClass(int index);
    ~MIClass() {}
    bool operator==(const MIClass& other) const;
    MIClass& operator=(const MIClass& other);
    void addWord(MIWord& word);
    void removeWord(MIWord& word);
    int numWords() const;
    const std::list<MIWord*>& getWords() const;
    void clear();
    void setIndex(int index);
    void setParent(MIClass& parent);
    void setLeftChild(MIClass& leftChild);
    void setRightChild(MIClass& rightChild);
    int index() const;
    MIClass& parent() const;
    MIClass& leftChild() const;
    MIClass& rightChild() const;
    void merge(MIClass& other);
    void merge(MIClass& left, MIClass& right);
    void printBits(std::wstring prefix, std::wofstream& out);
	void printBits(std::wstring prefix, UTF8OutputStream& out);
private:
    int _index;
    MIClass* _parent;
    MIClass* _leftChild;
    MIClass* _rightChild;
    std::list<MIWord*> wordList;
};

#endif

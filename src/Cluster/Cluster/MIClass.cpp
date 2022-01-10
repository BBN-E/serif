// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/leak_detection.h" // This must be the first #include

#include "MIClass.h"
#include "Generic/common/ParamReader.h"

using namespace std;

MIClass::MIClass(int index) {
    _index = index;
    _parent = 0;
    _leftChild = 0;
    _rightChild = 0;
}

void MIClass::addWord(MIWord& word) {
    wordList.push_back(&word);
    MIBaseElement::add(word);
}

void MIClass::removeWord(MIWord& word) {
    wordList.remove(&word);
    MIBaseElement::remove(word);
}

bool MIClass::operator==(const MIClass& other) const {
    return _index == other._index;
}

MIClass& MIClass::operator=(const MIClass& other) {
    clear();
    for (list<MIWord*>::const_iterator iter = other.wordList.begin();
         iter != other.wordList.end(); iter++)
    {
        wordList.push_back(*iter);
    }
    MIBaseElement::operator=(other);
    _index = other._index;
    _parent = other._parent;
    _leftChild = other._leftChild;
    _rightChild = other._rightChild;
    return *this;
}

int MIClass::numWords() const {
    return static_cast<int>(wordList.size());
}

const std::list<MIWord*>& MIClass::getWords() const {
    return wordList;
}

void MIClass::clear() {
    wordList.clear();
    MIBaseElement::clear();
    _parent = 0;
    _leftChild = 0;
    _rightChild = 0;
}

void MIClass::setIndex(int index) {
    _index = index;
}

void MIClass::setParent(MIClass& parent) {
    _parent = &parent;
}

void MIClass::setLeftChild(MIClass& leftChild) {
    _leftChild = &leftChild;
}

void MIClass::setRightChild(MIClass& rightChild) {
    _rightChild = &rightChild;
}

int MIClass::index() const {
    return _index;
}

MIClass& MIClass::parent() const {
    return *_parent;
}

MIClass& MIClass::leftChild() const {
    return *_leftChild;
}

MIClass& MIClass::rightChild() const {
    return *_rightChild;
}

void MIClass::merge(MIClass& other) {
    typedef list<MIWord*>::iterator Iter;
    for (Iter i = other.wordList.begin(); i != other.wordList.end(); ++i) {
        wordList.push_back(*i);
    }
    MIBaseElement::add(other);
}

void MIClass::merge(MIClass& left, MIClass& right) {
    clear();
    typedef list<MIWord*>::iterator Iter;
    for (Iter i = left.wordList.begin(); i != left.wordList.end(); ++i) {
        wordList.push_back(*i);
    }
    for (Iter i = right.wordList.begin(); i != right.wordList.end(); ++i) {
        wordList.push_back(*i);
    }
    MIBaseElement::add(left);
    MIBaseElement::add(right);
    _leftChild = &left;
    _rightChild = &right;
    left._parent = this;
    right._parent = this;
}

void MIClass::printBits(wstring prefix, wofstream& out) {
    if ((_leftChild == 0) && (_rightChild == 0)) {
        MIWord& w = *wordList.front();
        wstring name = w.name();

		//For Serif word cluster file
		//out << name << L" " << prefix << endl;

		//For other cluster file
		out << prefix;
		int n_spaces = int (64 - prefix.length() + 1);
		if (n_spaces <= 0)
			n_spaces = 1;
		for (int i = 0; i < n_spaces; i++)
			out << L" ";
//		wchar_t buf[1024];
		out << name << endl;
		
    }
    if (_leftChild != 0) {
        _leftChild->printBits(prefix + L'0', out);
    }
    if (_rightChild != 0) {
        _rightChild->printBits(prefix + L'1', out);
    }
}

void MIClass::printBits(wstring prefix, UTF8OutputStream& utf8out) {
    if ((_leftChild == 0) && (_rightChild == 0)) {
        MIWord& w = *wordList.front();
        wstring name = w.name();
		//ParamReader::isParamTrue("serif_style_cluster_output")
		if(true){
			//For Serif word cluster file
			utf8out<<name<<" "<<prefix<<"\n";
		} else {
			//For other cluster file
			utf8out << prefix;
			int n_spaces = int(64 - prefix.length() + 1);
			if (n_spaces <= 0)
				n_spaces = 1;
			for (int i = 0; i < n_spaces; i++)
				utf8out << L" ";
			utf8out << name.c_str() << L"\n";
		}
    }
    if (_leftChild != 0) {
        _leftChild->printBits(prefix + L'0', utf8out);
    }
    if (_rightChild != 0) {
        _rightChild->printBits(prefix + L'1', utf8out);
    }
}

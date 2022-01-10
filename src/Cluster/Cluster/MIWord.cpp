// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "MIWord.h"

using namespace std;

const Vocabulary* MIWord::pVoc = 0;

MIWord::MIWord(int index) {
    _index = index;
}

MIWord& MIWord::operator=(const MIWord& w) {
    MIBaseElement::operator=(w);
    _index = w._index;
    return *this;
}

bool MIWord::operator==(const MIWord& w) const {
    return _index == w._index;
}

bool MIWord::operator!=(const MIWord& w) const {
    return _index != w._index;
}

bool MIWord::operator<(const MIWord &w) const {
    return ((getTotalCount() < w.getTotalCount()) ||
            ((getTotalCount() == w.getTotalCount()) && (_index < w._index)));
}

bool MIWord::operator>(const MIWord& w) const {
    return ((getTotalCount() > w.getTotalCount()) ||
            ((getTotalCount() == w.getTotalCount()) && (_index > w._index)));
}

bool MIWord::operator<=(const MIWord& w) const {
    return ((getTotalCount()<w.getTotalCount()) ||
            ((getTotalCount() == w.getTotalCount()) && (_index <= w._index)));
}

bool MIWord::operator>=(const MIWord& w) const {
    return ((getTotalCount() > w.getTotalCount()) ||
            ((getTotalCount() == w.getTotalCount()) && (_index >= w._index)));
}

void MIWord::setIndex(int index) {
    _index = index;
}

int MIWord::index() const {
    return _index;
}

const wstring& MIWord::name() const {
    return pVoc->get(_index);
}

void MIWord::useVoc(const Vocabulary &voc) {
    pVoc = &voc;
}

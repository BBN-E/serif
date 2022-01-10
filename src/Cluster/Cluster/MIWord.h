// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MI_WORD_H
#define MI_WORD_H

#include <string>
#include "Vocabulary.h"
#include "MIBaseElement.h"

class MIWord : public MIBaseElement {
public:
    MIWord(int index = 0);
    ~MIWord() {}
    MIWord &operator=(const MIWord& w);
    bool operator==(const MIWord& w) const;
    bool operator!=(const MIWord& w) const;
    bool operator<(const MIWord& w) const;
    bool operator>(const MIWord& w) const;
    bool operator<=(const MIWord& w) const;
    bool operator>=(const MIWord& w) const;
    void setIndex(int index);
    int index() const;
    const std::wstring& name() const;
    static void useVoc(const Vocabulary& voc);
private:
    int _index;
    static const Vocabulary* pVoc;
};

#endif

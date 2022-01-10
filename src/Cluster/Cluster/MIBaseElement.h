// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MI_BASE_ELEMENT_H
#define MI_BASE_ELEMENT_H

class MIBaseElement {
private:
    struct FutureCountPair {
        int future;
        int count;
    };
    int totalCount;
    int numFutures;
    int maxFutures;
    int maxDenseCounts;
    FutureCountPair* futureMap;
    int* denseCounts;
    int findLocation(int future) const;
    int lookupCount(int future) const;
public:
    MIBaseElement();
    ~MIBaseElement();
    MIBaseElement& operator=(const MIBaseElement& other);
    void add(int future, int count);
    void remove(int future, int count);
    void add(const MIBaseElement& other);
    void remove(const MIBaseElement& other);
    double addMIChange(int future, int count) const;
    double removeMIChange(int future, int count) const;
    double addMIChange(const MIBaseElement& other) const;
    double addMIChangeDense(const MIBaseElement& other) const;
    double removeMIChange(const MIBaseElement& other) const;
    double reclassMIChange(int oldFuture, int newFuture, int count) const;
    void enableDenseCounts(int size);
    void clear();
    inline int futureAt(int i) const {return futureMap[i].future;}
    inline int countAt(int i) const {return futureMap[i].count;}
    inline int getTotalCount() const {return totalCount;}
    inline int getNumFutures() const {return numFutures;}
    inline int getCount(int future) const {
        if (denseCounts) {
            return denseCounts[future];
        } else {
            return lookupCount(future);
        }
    }
};

#endif

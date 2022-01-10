// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "MIBaseElement.h"
#include "NLogN.h"

MIBaseElement::MIBaseElement() {
    totalCount = 0;
    numFutures = 0;
    maxFutures = 1;
    futureMap = new FutureCountPair[1];
    denseCounts = 0;
    maxDenseCounts = 0;
}

MIBaseElement::~MIBaseElement() {
    delete [] futureMap;
    delete [] denseCounts;
}

MIBaseElement& MIBaseElement::operator=(const MIBaseElement& other) {
    delete [] futureMap;
    futureMap = new FutureCountPair[other.maxFutures];
    for (int i = 0; i < other.numFutures; i++) {
        futureMap[i] = other.futureMap[i];
    }
    delete [] denseCounts;
    if (other.denseCounts) {
        denseCounts = new int[other.maxDenseCounts];
        for (int i = 0; i < other.maxDenseCounts; i++) {
            denseCounts[i] = other.denseCounts[i];
        }
    } else {
        denseCounts = 0;
    }
    totalCount = other.totalCount;
    numFutures = other.numFutures;
    maxFutures = other.maxFutures;
    maxDenseCounts = other.maxDenseCounts;
    return *this;
}

int MIBaseElement::findLocation(int future) const {
    if ((numFutures == 0) || (future <= futureMap[0].future)) {
        return 0;
    }
    int first = 0;
    int last = numFutures - 1;
    if (future > futureMap[last].future) {
        return last + 1;
    }
    while (true) {
        int mid = (first + last) / 2;
        if (mid == first) {
            return mid + 1;
        }
        if (future == futureMap[mid].future) {
            return mid;
        }
        if (future < futureMap[mid].future) {
            last = mid;
        } else {
            first = mid;
        }
    }
}

void MIBaseElement::add(int future, int count) {
    totalCount += count;
    int loc = findLocation(future);
    if ((loc == numFutures) || (futureMap[loc].future != future)) {
        if (numFutures == maxFutures) {
            maxFutures *= 2;
            FutureCountPair* temp = futureMap;
            futureMap = new FutureCountPair[maxFutures];
            for (int i = 0; i < numFutures; i++) {
                futureMap[i] = temp[i];
            }
            delete [] temp;
        }
        for (int i = numFutures; i > loc; i--) {
            futureMap[i] = futureMap[i - 1];
        }
        futureMap[loc].future = future;
        futureMap[loc].count = 0;
        numFutures++;
    }
    futureMap[loc].count += count;
    if (denseCounts) {
        denseCounts[future] += count;
    }
}

void MIBaseElement::remove(int future, int count) {
    totalCount -= count;
    int loc = findLocation(future);
    futureMap[loc].count -= count;
    if (denseCounts) {
        denseCounts[future] -= count;
    }
}

void MIBaseElement::add(const MIBaseElement& other) {
    for (int i = 0; i < other.numFutures; i++) {
        add(other.futureMap[i].future, other.futureMap[i].count);
    }
}

void MIBaseElement::remove(const MIBaseElement& other) {
    for (int i = 0; i < other.numFutures; i++) {
        remove(other.futureMap[i].future, other.futureMap[i].count);
    }
}

double MIBaseElement::addMIChange(int future, int count) const {
    int c1 = getCount(future);
    return ((NLogN::val(totalCount) - NLogN::val(totalCount + count)) +
            (NLogN::val(c1 + count) - NLogN::val(c1)));
}

double MIBaseElement::removeMIChange(int future, int count) const {
    int c1 = getCount(future);
    return ((NLogN::val(totalCount) - NLogN::val(totalCount - count)) +
            (NLogN::val(c1 - count) - NLogN::val(c1)));
}

double MIBaseElement::addMIChange(const MIBaseElement& other) const {
    if (denseCounts) {
        return addMIChangeDense(other);
    }
    int i = 0;
    int j = 0;
    double change = NLogN::val(totalCount) + NLogN::val(other.totalCount)
        - NLogN::val(totalCount + other.totalCount);
    while (true) {
        if (j < other.numFutures) {
            while ((i < numFutures) && 
                   (futureMap[i].future < other.futureMap[j].future)) {
                i++;
            }
        }
        if (i < numFutures) {
            while ((j < other.numFutures) &&
                   (futureMap[i].future > other.futureMap[j].future)) {
                j++;
            }
        }
        if ((i == numFutures) || (j == other.numFutures)) {
            break;
        }
        if (futureMap[i].future == other.futureMap[j].future) {
            int c1 = futureMap[i].count;
            int c2 = other.futureMap[j].count;
            change += NLogN::val(c1 + c2) - 
                (NLogN::val(c1) + NLogN::val(c2));
            i++;
            j++;
        }
    }
    return change;
}

double MIBaseElement::addMIChangeDense(const MIBaseElement& other) const {
    double change = NLogN::val(totalCount) + NLogN::val(other.totalCount)
        - NLogN::val(totalCount + other.totalCount);
    for (int i = 0; i < other.numFutures; i++) {
        int future = other.futureMap[i].future;
        int c1 = denseCounts[future];
        int c2 = other.futureMap[i].count;
        if ((c1 != 0) && (c2 != 0)) {
            change += NLogN::val(c1 + c2) - 
            (NLogN::val(c1) + NLogN::val(c2));
        }
    }
    return change;
}

double MIBaseElement::removeMIChange(const MIBaseElement& other) const {
    int i = 0;
    int j = 0;
    double change = NLogN::val(totalCount - other.totalCount) +
        NLogN::val(other.totalCount) - NLogN::val(totalCount);
    while (true) {
        if (j < other.numFutures) {
            while ((i < numFutures) && 
                   (futureMap[i].future < other.futureMap[j].future)) {
                i++;
            }
        }
        if (i < numFutures) {
            while ((j < other.numFutures) &&
                   (futureMap[i].future > other.futureMap[j].future)) {
                j++;
            }
        }
        if ((i == numFutures) || (j == other.numFutures)) {
            break;
        }
        if (futureMap[i].future == other.futureMap[j].future) {
            int c1 = futureMap[i].count;
            int c2 = other.futureMap[j].count;
            change += NLogN::val(c1) -
                (NLogN::val(c1 - c2) + NLogN::val(c2));
            i++;
            j++;
        }
    }
    return -change;
}

double MIBaseElement::reclassMIChange(int oldFuture, int newFuture, 
                                      int count) const
{
    int oldCount = getCount(oldFuture);
    int newCount = getCount(newFuture);
    return ((NLogN::val(oldCount - count) - NLogN::val(oldCount)) +
            (NLogN::val(newCount + count) - NLogN::val(newCount)));
}

int MIBaseElement::lookupCount(int future) const {
    int loc = findLocation(future);
    if ((loc < numFutures) && (futureMap[loc].future == future)) {
        return futureMap[loc].count;
    } else {
        return 0;
    }
}

void MIBaseElement::enableDenseCounts(int size) {
    delete [] denseCounts;
    denseCounts = new int[size];
    maxDenseCounts = size;
    for (int i = 0; i < size; i++) {
        denseCounts[i] = 0;
    }
    for (int i = 0; i < numFutures; i++) {
        int future = futureMap[i].future;
        int count = futureMap[i].count;
        denseCounts[future] = count;
    }
}

void MIBaseElement::clear() {
    delete [] futureMap;
    delete [] denseCounts;
    totalCount = 0;
    numFutures = 0;
    maxFutures = 1;
    futureMap = new FutureCountPair[1];
    denseCounts = 0;
    maxDenseCounts = 0;
}

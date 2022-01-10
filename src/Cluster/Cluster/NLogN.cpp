// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "NLogN.h"

double* NLogN::table = NLogN::setupTable();

double* NLogN::setupTable() {
    table = new double[TABLE_SIZE];
    table[0] = 0;
    for (int i = 1; i < TABLE_SIZE; i++) {
        double c = i;
        table[i] = c * log(c) / M_LN2;
    }
    return table;
}

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef N_LOG_N_H
#define N_LOG_N_H

#define _USE_MATH_DEFINES
#include <cmath>

class NLogN {
private:
    static const int TABLE_SIZE = 100000;
    static double* table;
    static double* setupTable();
public:
    static inline double val(int count) {
        if (count >= TABLE_SIZE) {
            double c = count;
            return c * log(c) / M_LN2;
        } else {
            return table[count];
        }
    }
};

#endif

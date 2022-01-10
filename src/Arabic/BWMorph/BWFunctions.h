// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef AR_BW_FUNCTIONS_H
#define AR_BW_FUNCTIONS_H

#include "Generic/common/Symbol.h"

#include <string>


class BWFunctions {
public:

static bool isDiacritic(wchar_t c);
static bool isPartOfDictKey(wchar_t c);
};
#endif

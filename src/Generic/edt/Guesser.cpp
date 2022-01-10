// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/edt/Guesser.h"
#include "Generic/edt/xx_Guesser.h"

Symbol Guesser::FEMININE(L"FEMININE");
Symbol Guesser::MASCULINE(L"MASCULINE");
Symbol Guesser::NEUTER(L"NEUTER");
Symbol Guesser::NEUTRAL(L"NEUTRAL");
Symbol Guesser::SINGULAR(L"SINGULAR");
Symbol Guesser::PLURAL(L"PLURAL");
Symbol Guesser::UNKNOWN(L":UNKNOWN");

boost::shared_ptr<Guesser::GuesserInstance> &Guesser::getInstance() {
    static boost::shared_ptr<GuesserInstance> instance(_new GuesserInstanceFor<GenericGuesser>());
    return instance;
}

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/EventUtilities.h"
#include "Generic/events/xx_EventUtilities.h"

boost::shared_ptr<EventUtilities::EventUtilitiesInstance> &EventUtilities::getInstance() {
    static boost::shared_ptr<EventUtilitiesInstance> instance(_new EventUtilitiesInstanceFor<GenericEventUtilities>());
    return instance;
}

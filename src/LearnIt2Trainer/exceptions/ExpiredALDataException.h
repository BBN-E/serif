#ifndef _EXPIRED_TRAINER_EXCEPTION_
#define _EXPIRED_TRAINER_EXCEPTION_

#include "Generic/common/UnrecoverableException.h"

class ExpiredALDataException : public UnrecoverableException {
public:
	ExpiredALDataException(const char* source);
};

#endif

#ifndef _EXPIRED_TRAINER_EXCEPTION_
#define _EXPIRED_TRAINER_EXCEPTION_

#include "Generic/common/UnrecoverableException.h"

class ExpiredTrainerException : public UnrecoverableException {
public:
	ExpiredTrainerException(const char* source);
};

#endif

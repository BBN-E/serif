#ifndef _ACTIVE_LEARNING_UTILITIES_H_
#define _ACTIVE_LEARNING_UTILITIES_H_

#include <string>
#pragma warning(push, 0)
#include <boost/thread.hpp>
#pragma warning(pop)
// TODO: move mongoose to Externals
#include "mongoose/mongoose.h"

namespace ActiveLearningUtilities {
	void sendUIReadyNotification(const std::string& emailAddress, unsigned int port);

	bool runActiveLearning(const std::string& documentRoot, 
		boost::thread& optimizationThread, mg_callback_t event_handler);
};


#endif

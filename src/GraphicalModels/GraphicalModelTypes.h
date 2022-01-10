#ifndef _GRAPHICAL_MODEL_TYPES_H_
#define _GRAPHICAL_MODEL_TYPES_H_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace GraphicalModel { 
	class Message;
	typedef boost::shared_ptr<Message> Message_ptr;
};

#endif


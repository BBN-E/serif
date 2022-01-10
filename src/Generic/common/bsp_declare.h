#ifndef _BSP_DECLARE_H_
#define _BSP_DECLARE_H_

#include <boost/shared_ptr.hpp>
#define BSP_DECLARE(X) class X; typedef boost::shared_ptr<X> X##_ptr;

#endif

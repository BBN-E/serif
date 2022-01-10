#include "Generic/common/leak_detection.h"

#include "Generic/nestedNames/NestedNameRecognizer.h"
#include "Generic/nestedNames/xx_NestedNameRecognizer.h"

boost::shared_ptr<NestedNameRecognizer::Factory> &NestedNameRecognizer::_factory() {
	static boost::shared_ptr<NestedNameRecognizer::Factory> factory(_new GenericNestedNameRecognizerFactory());
	return factory;
}

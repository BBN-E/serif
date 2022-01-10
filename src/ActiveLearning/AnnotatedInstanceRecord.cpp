#include "AnnotatedInstanceRecord.h"

const int AnnotatedInstanceRecord::NO_ANNOTATION = 0;
const int AnnotatedInstanceRecord::POSITIVE_ANNOTATION = 1;
const int AnnotatedInstanceRecord::NEGATIVE_ANNOTATION = -1;

AnnotatedInstanceRecord::AnnotatedInstanceRecord(size_t instance_hash, int annotation) 
: _instance_hash(instance_hash), _annotation(annotation) 
{
}

size_t AnnotatedInstanceRecord::instance_hash() const {
	return _instance_hash;
}

int AnnotatedInstanceRecord::annotation() const {
	return _annotation;
}

void AnnotatedInstanceRecord::setAnnotation(int ann) {
	_annotation = ann;
}

#ifndef _ANNOTATED_INSTANCE_RECORD_H_
#define _ANNOTATED_INSTANCE_RECORD_H_

#include <stddef.h>

class AnnotatedInstanceRecord {
public:
	AnnotatedInstanceRecord(size_t instance_hash, int annotation);
	size_t instance_hash() const;
	int annotation() const;
	void setAnnotation(int ann);

	static const int NO_ANNOTATION; // = 0
	static const int POSITIVE_ANNOTATION; // = 1
	static const int NEGATIVE_ANNOTATION; // = -1
private:
	size_t _instance_hash;
	int _annotation;
};

#endif

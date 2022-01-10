#ifndef _LEARNIT_DOC_ANNOTATION_H_
#define _LEARNIT_DOC_ANNOTATION_H_

#include <string>

class Annotation {
public:
	Annotation(const std::wstring& annotationString, int start_token, int end_token) 
		: annotationString(annotationString), start_token(start_token), 
		end_token(end_token) {}

	std::wstring annotationString;
	int start_token;
	int end_token;

};

// sorts annotations by start tag position
// sort by earlier start_token, falling back for ties to *later* end_token,
// so that less deeply nested annotations have their start tags later
struct annotation_start_compare {
	inline bool operator()(const Annotation& x, const Annotation& y) const {
		if (x.start_token<y.start_token) {
			return true;
		} else if (x.start_token==y.start_token) {
			if (x.end_token==y.end_token) {
				return x.annotationString<y.annotationString;
			} else {
				return x.end_token>y.end_token;
			}	
		} else {
			return false;
		}
	}
};

// sorts annotations by end tag position
// sort by later end_token, falling back for ties to *later* start_token,
// so that more deeply nested annotations have their end tags earlier 
struct annotation_end_compare {
	inline bool operator()(const Annotation& x, const Annotation& y) const {
		if (x.end_token<y.end_token) {
			return true;
		} else if (x.end_token==y.end_token) {
			if (x.start_token==y.start_token) {
				return x.annotationString>y.annotationString;
			} else {
				return x.start_token>y.start_token;
			}
		} else {
			return false;
		}
	}
};


#endif

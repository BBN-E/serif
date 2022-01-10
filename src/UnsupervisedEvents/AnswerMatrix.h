#ifndef _ANSWER_MATRIX_H_
#define _ANSWER_MATRIX_H_

#include <vector>
#include "ACEDecoder.h"

typedef std::vector<double> LabelVector;
class AnswerMatrix {
public:
	AnswerMatrix(size_t n_entities, size_t n_labels);
	AnswerMatrix(const EntityLabelling& labelling, size_t n_labels);

	const LabelVector& forEntity(size_t e) const; 
	LabelVector& forEntity(size_t e);
	size_t size() const { return _matrix.size(); }
	bool hasRole(size_t role) const;
	bool assignedARole(size_t entity) const;
	void normalize();
private:
	std::vector<LabelVector > _matrix;
};

#endif


#include "AnswerMatrix.h"
#include <boost/foreach.hpp>
#include "ACEDecoder.h"

AnswerMatrix::AnswerMatrix(size_t n_entities, size_t n_labels) 
	: _matrix(n_entities, std::vector<double>(n_labels, 0.0))
{ }


AnswerMatrix::AnswerMatrix(const EntityLabelling& labelling, size_t n_labels) 
	: _matrix(labelling.size(), std::vector<double>(n_labels, 0.0)) 
{
	for (size_t i=0; i<labelling.size(); ++i) {
		LabelAndScore ls = labelling[i];
		if (ls.first != -1) {
			// note: currently ignores score
			_matrix[i][ls.first] = 1.0;
		}
	}
}

const std::vector<double>& AnswerMatrix::forEntity(size_t e) const { 
	return _matrix[e];
}

std::vector<double>& AnswerMatrix::forEntity(size_t e) { 
	return _matrix[e];
}

bool AnswerMatrix::hasRole(size_t role) const {
	BOOST_FOREACH(const std::vector<double>& scores, _matrix) {
		if (scores[role] > 0.0) {
			return true;
		}
	}
	return false;
}

bool AnswerMatrix::assignedARole(size_t entity) const {
	BOOST_FOREACH(double d, _matrix[entity]) {
		if (d>0.0) {
			return true;
		}
	}
	return false;
}

void AnswerMatrix::normalize() {
	BOOST_FOREACH(std::vector<double>& scores, _matrix) {
		double sum = 0.0;
		BOOST_FOREACH(double score, scores) {
			sum += score;
		}
		if (sum > 0.0) {
			BOOST_FOREACH(double& score, scores) {
				score/=sum;
			}
		}
	}
}


#ifndef _TEMPORAL_FEATURE_H_
#define _TEMPORAL_FEATURE_H_

#include <string>
#include <sstream>
#include <vector>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"

BSP_DECLARE(TemporalFeature)
class DocTheory;
BSP_DECLARE(TemporalInstance)

class TemporalFeature {
public:
	const Symbol& relation() const;
	const Symbol& type() const;
	virtual std::wstring pretty() const = 0;
	virtual std::wstring dump() const = 0;
	virtual std::wstring metadata() const = 0;
	virtual bool equals(const TemporalFeature_ptr& other) const = 0;
	size_t hash() const;
	virtual ~TemporalFeature();
	unsigned int idx() const;
	void setIdx(unsigned int val);

	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const = 0;

	void setWeight(unsigned int alphabet, double weight);
	double weight(unsigned int alphabet) const;
	bool passesThreshold(double threshold) const;
protected:
	TemporalFeature(const Symbol& type, const Symbol& relation);
	virtual size_t calcHash() const = 0;

	bool topEquals(const TemporalFeature_ptr& other) const;
	size_t topHash() const;
	void metadata(std::wstringstream& str) const;
	bool relationNameMatches(TemporalInstance_ptr inst) const;
private:
	Symbol _relation;
	Symbol _type;
	mutable size_t _hash;
	mutable bool _init_hash;
	unsigned int _idx;
	std::vector<double> _weights;
};

class TemporalFeatureHash {
public:
	std::size_t operator()(const TemporalFeature_ptr& p) const {
		return p->hash();
	}
};

class TemporalFeatureEquality {
public:
	bool operator()(const TemporalFeature_ptr& a,
			const TemporalFeature_ptr& b) const {
		return a->equals(b);
	}
};

#endif


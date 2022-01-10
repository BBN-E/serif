#ifndef _PARTICULAR_FEATURES_H_
#define _PARTICULAR_FEATURES_H_

#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "TemporalFeature.h"

class DocTheory;
BSP_DECLARE(TemporalInstance)
BSP_DECLARE(ParticularYearFeature)
BSP_DECLARE(ParticularMonthFeature)
BSP_DECLARE(ParticularDayFeature)

class ParticularXFeature : public TemporalFeature {
public:
	std::wstring pretty() const;
	std::wstring dump() const;
	std::wstring metadata() const;
	bool equals(const TemporalFeature_ptr& other) const;
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const = 0;
protected:
	ParticularXFeature(const Symbol& type, const Symbol& relation = Symbol());
	size_t calcHash() const;
};

class ParticularDayFeature : public ParticularXFeature {
public:
	ParticularDayFeature(const Symbol& relation = Symbol());
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	bool equals(const TemporalFeature_ptr& other) const; 
};

class ParticularMonthFeature : public ParticularXFeature {
public:
	ParticularMonthFeature(const Symbol& relation = Symbol());
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	bool equals(const TemporalFeature_ptr& other) const; 
};

class ParticularYearFeature : public ParticularXFeature {
public:
	ParticularYearFeature(const Symbol& relation = Symbol());
	virtual bool matches(TemporalInstance_ptr inst, const DocTheory* dt, 
			unsigned int sn) const;
	bool equals(const TemporalFeature_ptr& other) const; 
};
#endif

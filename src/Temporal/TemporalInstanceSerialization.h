#ifndef _TEMPORAL_INSTANCE_SERIALIZATION_H_
#define _TEMPORAL_INSTANCE_SERIALIZATION_H_

#include <vector>
#include <string>
#include <boost/serialization/access.hpp> 
#include <boost/serialization/vector.hpp> 
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "TemporalTypes.h"

class TemporalInstance;
class Symbol;

BSP_DECLARE(TemporalInstanceData)

class TemporalInstanceData {
public:
	// default constructor for serialization use only
	TemporalInstanceData();
	static TemporalInstanceData_ptr create(const TemporalInstance& inst, const TemporalFV& vec);
	size_t hashValue() const;
	const TemporalFV& fv() const;
	TemporalFV& fv();
	unsigned int type() const;
private:
	TemporalInstanceData(unsigned int t, size_t hv, const TemporalFV& vec);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(TemporalInstanceData, unsigned int,
		size_t, const TemporalFV&);
	size_t _hashValue;
	TemporalFV _fv;
	unsigned int _type;
	bool _dummy;

	friend class boost::serialization::access;
	template<class Archive> void serialize(Archive& archive,
		const unsigned int version) {
			archive & _hashValue;
			archive & _type;
			archive & _fv;
			_dummy = false;
	}
};

#endif

#ifndef _INSTANCE_HASHES_H_
#define _INSTANCE_HASHES_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(InstanceHashes)

class InstanceHashes {
public:
	InstanceHashes();
	void registerInstance(unsigned int i, const std::string& keyData);
	void registerInstance(unsigned int i, size_t hashVal);
	size_t hash(int i) const;
	int instance(size_t hsh) const;
	size_t size() const;
private:
	std::map<unsigned int, size_t> _instanceToHash;
	std::map<size_t, unsigned int> _hashToInstance;
};

class UnknownInstanceHashException : public std::runtime_error {
public:
	UnknownInstanceHashException(size_t hash);
	size_t hash;
};
#endif

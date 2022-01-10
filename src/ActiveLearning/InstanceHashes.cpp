#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "InstanceHashes.h"

#include <boost/functional/hash.hpp>

using std::make_pair;

InstanceHashes::InstanceHashes() {}

size_t InstanceHashes::hash(int i) const {
	return _instanceToHash.find(i)->second;
}

int InstanceHashes::instance(size_t hsh) const {
	std::map<size_t, unsigned int>::const_iterator probe =
		_hashToInstance.find(hsh);
	if (probe != _hashToInstance.end()) {
		return probe->second;
	} else {
		throw UnknownInstanceHashException(hsh);
	}
}

void InstanceHashes::registerInstance(unsigned int i, const std::string &keyData) {
	registerInstance(i, boost::hash<std::string>()(keyData));
}

void InstanceHashes::registerInstance(unsigned int i, size_t hashValue) {
	_instanceToHash.insert(make_pair(i, hashValue));
	if (_hashToInstance.find(hashValue) != _hashToInstance.end()) {
		SessionLogger::warn("active_learning_hash_collision") << "Instance hash collision for hash "
			<< hashValue << ". Lookup by "
			<< "hashes will only return the first instance with this hash.";
	} else {
		_hashToInstance.insert(make_pair(hashValue, i));
	}
}

size_t InstanceHashes::size() const {
	return _instanceToHash.size();
}

UnknownInstanceHashException::UnknownInstanceHashException(size_t hash) 
: hash(hash), std::runtime_error("Instance hash not found")
{}


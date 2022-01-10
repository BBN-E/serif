#ifndef _LEARNIT2_CLIENT_INSTANCE_H_
#define _LEARNIT2_CLIENT_INSTANCE_H_

#include <string>

// string preview
class ClientInstance {
public:
	ClientInstance(size_t _inst_hash, const std::wstring& _preview_string,
		int _annotation);
	std::wstring preview_string;
	size_t inst_hash;
	int annotation;
};

std::string jsonify(const ClientInstance& clientInst);

#endif

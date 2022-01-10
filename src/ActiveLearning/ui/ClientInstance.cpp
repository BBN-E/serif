#include "ClientInstance.h"

#include <string>
#include <sstream>
#include "JSONUtils.h"

using std::string; using std::wstring; using std::stringstream;

string jsonify(const ClientInstance& clientInst) {
	stringstream json;
	json << "{\"hash\" : '" << clientInst.inst_hash << "', \"annotation\" : " 
		<< clientInst.annotation << ", \"preview_string\" : " 
		<< JSONUtils::jsonify(clientInst.preview_string) << "}";
	return json.str();
}

ClientInstance::ClientInstance(
	size_t _inst_hash, const std::wstring& _preview_string, int _annotation)
: inst_hash(_inst_hash), preview_string(_preview_string), annotation(_annotation)
{}


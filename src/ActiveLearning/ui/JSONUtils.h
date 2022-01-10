#ifndef _JSON_UTILS_H_
#define _JSON_UTILS_H_

#include <string>
#include <sstream>
#include <vector>

namespace JSONUtils {	
	// JSON representation of a string
	std::string jsonify(const std::wstring& str);
	
	// gives the JSON representation of a vector of arbitary type as an array
	// must appear in header to be available if ever compiled as a
	// library, since it's a template
	template <typename T>
	static void jsonify(std::stringstream& json, const std::vector<T>& vec)
	{
		json << "[ ";
		bool first = true;
		for (typename std::vector<T>::const_iterator it = vec.begin(); it!=vec.end(); ++it) {
			if (first) {
				first = false;
			} else {
				json << ", ";
			}
			json << jsonify(*it);
		}
		json << "]";
	}

	// ensures < and > characters are preserved
	std::wstring html_escape(const std::wstring& str);
};

#endif

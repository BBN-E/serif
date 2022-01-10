#include "JSONUtils.h"
#include <string>
#include <boost/algorithm/string/replace.hpp>

using std::string; using std::wstring;
using boost::replace_all;

string JSONUtils::jsonify(const std::wstring& str) {
	string ret(str.length(), ' ');

	for (size_t i=0; i<str.length(); ++i) {
		wchar_t w = str[i];
		char c = (char)w;
		if (c == w || c == '\n' || c == '\t') {
			ret[i] = c;
		} else {
			ret[i] = '?';
		}
	}
	boost::replace_all(ret, "\"", "\\\"");
	boost::replace_all(ret, "\n", "\\n");
	boost::replace_all(ret, "\r", "\\r");
	boost::replace_all(ret, "\t", "\\t");
	return "\"" + ret + "\"";
}

void replace(std::wstring& s, const std::wstring& f, const std::wstring& r) {
	size_t pos = 0;

	while ((pos = s.find(f, pos)) != wstring::npos) {
		s.replace(pos, f.length(), r);
		++pos;
	}
}

wstring JSONUtils::html_escape(const wstring& str) {
	wstring ret = str;
	// for some reason boost's replace_all was crashing here in 
	// release mode on Linux, so we do it by hand
	//replace_all(ret, L"<", L"&lt;");
	//replace_all(ret, L">", L"&gt;");
	replace(ret, L"<", L"&lt;");
	replace(ret, L">", L"&gt;");
	return ret;
}

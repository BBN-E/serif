#pragma once

template <typename T>
class CanopyUtils {
public:
	typedef std::multimap<T, int> KeyToFeatures;
	typedef std::pair<typename KeyToFeatures::const_iterator, 
		typename KeyToFeatures::const_iterator> Features;
};

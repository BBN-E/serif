// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EDIT_DISTANCE_H
#define EDIT_DISTANCE_H

#include <string>
#include <vector>


class EditDistance {
public:
	
	template<typename T>
	inline size_t distance(
		const std::basic_string<T> & str1,
		const std::basic_string<T> & str2
	)
	{
		if(crow.size() < str2.size()+1)
		{
			crow.resize(str2.size()+1);
			nrow.resize(str2.size()+1);
		}
		
		for(size_t t = 0; t != str2.size() + 1; ++t)
			crow[t] = t;
		
		for(size_t i = 0; i != str1.size(); ++i)
		{
			nrow[0] = i + 1;
			for(size_t j = 0; j != str2.size(); ++j)
			{
				size_t d1 = crow[j+1] + 1;
				size_t d2 = nrow[j] + 1;
				size_t d3 = crow[j] + ((str1[i] == str2[j]) ? 0 : 1);
				
				nrow[j+1] = std::min(d1, std::min(d2, d3));
			}
			nrow.swap(crow);
		}
		
		return crow[str2.size()];
	}
	
	template<typename T>
	inline float similarity(
		const std::basic_string<T> & str1,
		const std::basic_string<T> & str2
	)
	{
		return 1.0f - float(distance(str1, str2)) /
			std::max(str1.size(),str2.size());
	}
	
private:
	
	std::vector<size_t> crow, nrow;
};

#endif // #ifndef EDIT_DISTANCE_H


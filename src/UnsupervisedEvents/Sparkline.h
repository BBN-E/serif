#ifndef _SPARKLINE_H_
#define _SPARKLINE_H_
#include <vector>
#include <string>
#include <iostream>

class SparkBar {
	public:
		SparkBar(const std::vector<double>& data, double range_min,
				double range_max, int height, int bar_width, int bar_pad,
				const std::vector<std::wstring>& colorMap,
				bool hide_too_low = false);
		size_t numBars() const;
		int barHeight(size_t i) const;
		int graphWidth() const;
	private:
		std::vector<double> _data;
		std::vector<std::wstring> _colorMap;
		double _range_max;
		double _range_min;
		bool _hide_too_low;
		int _height;
		int _bar_width;
		int _bar_pad;

		friend std::wostream& operator<<(std::wostream& out, const SparkBar& bar);
};

std::wostream& operator<<(std::wostream& out, const SparkBar& bar);
#endif


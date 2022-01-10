#include "Sparkline.h" 

using std::vector;
using std::wstring;
using std::wostream;
using std::endl;


SparkBar::SparkBar(const std::vector<double>& data, double range_min,
		double range_max, int height, int bar_width, int bar_pad,
		const std::vector<std::wstring>& colorMap, bool hide_too_low) 
: _data(data), _range_min(range_min), _range_max(range_max), _colorMap(colorMap),
	_hide_too_low(hide_too_low), _height(height), _bar_width(bar_width),
	_bar_pad(bar_pad)
{

}

size_t SparkBar::numBars() const {
	size_t bars = 0;
	if (_hide_too_low) {
		for (size_t i=0; i<_data.size(); ++i) {
			if (barHeight(i) > 0) {
				++bars;
			}
		}
	} else {
		bars = _data.size();
	}
	return bars;
}

int SparkBar::barHeight(size_t i) const {
	return int((_data[i]/_range_max * _height) + 0.5);
}

int SparkBar::graphWidth() const {
	return numBars() * (_bar_width + _bar_pad);
}

std::wostream& operator<<(std::wostream& out, const SparkBar& bar) {
	out << endl << L"<div class='bbn_sparkbar' style='background:#FFFFFF;width:" 
		<< bar.graphWidth() << L"px;" << L"height: " << bar._height << L"px"
		<< L"; position:relative;display:inline-block;overflow:hidden'>";
	double offset = 0;
	for (size_t i=0; i<bar._data.size(); ++i) {
		double v= bar.barHeight(i);
		if ( v > 0 || !bar._hide_too_low) {
			out << L"<div style='display:inline-block;position:absolute;overflow:hidden;left:"<<offset
				<< L";width:" << bar._bar_width << "px;height:" 
				<< v
				<< L"px;top:" << (bar._height - v) << L";background:" << bar._colorMap[i]  << L"'></div>";
			offset+=bar._bar_width + bar._bar_pad;
		}
	}
	out << L"</div>" << endl;
	return out;
}


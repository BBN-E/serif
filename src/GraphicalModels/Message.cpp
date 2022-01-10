#include "Message.h"
#include <iostream>
#include <boost/foreach.hpp>

void GraphicalModel::Message::dump(std::wostream& out) const {
	out << L"(";
	bool first = true;
	BOOST_FOREACH(double d, _msg) {
		if (first) {
			first = false;
		} else {
			out << L", ";
		}
		out << d;
	}
	out << L")";
}

std::wostream& operator<<(std::wostream& out, const GraphicalModel::Message& msg) {
	msg.dump(out);
	return out;
}


void GraphicalModel::Message::toHTMLTDs(std::wostream& out) const {
	BOOST_FOREACH(double d, _msg) {
		out << L"<td>" << d << L"</td>";
	}
}



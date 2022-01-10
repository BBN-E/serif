#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <vector>
#include <iostream>
#include "Generic/common/bsp_declare.h"

namespace GraphicalModel {
	BSP_DECLARE(Message)
	typedef std::vector<Message_ptr> MessageVector;
	class Message {
		public:
			Message(unsigned int sz) : _msg(sz), _sent(false) {}
			unsigned int size() const { return _msg.size(); }
			void clear() {
				_sent = false;
			}
			bool sent() const { return _sent; }
			void send() { _sent = true; }
			//double& operator[] (unsigned int i) {return _msg[i];}
			void set(unsigned int i, double val) {
				_msg[i] = val;
			}
			double operator[] (unsigned int i) const { return _msg[i];}
			// point-wise product
			void product(const Message& otherMsg) {
				vectorProductFromTo(otherMsg._msg, _msg);
			}

			void productTo(std::vector<double>& to) const {
				vectorProductFromTo(_msg, to);
			}

			void dump(std::wostream& out) const;
			void toHTMLTDs(std::wostream& out) const;
		private:
			std::vector<double> _msg;
			bool _sent;

			static void vectorProductFromTo(
					const std::vector<double>& from, std::vector<double>& to) 
			{
				assert(from.size() == to.size());
				std::vector<double>::iterator it = to.begin();
				std::vector<double>::const_iterator jt = from.begin();

				while (it != to.end()) {
					// addition because we're in log-space
					*(it++) += *(jt++);
				}
			};
	};

};

std::wostream& operator<<(std::wostream& out, const GraphicalModel::Message& msg);

#endif


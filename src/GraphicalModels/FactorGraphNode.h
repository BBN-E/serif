#ifndef _FACTOR_GRAPH_NODE_H_
#define _FACTOR_GRAPH_NODE_H_

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/bsp_declare.h"
#include "GraphicalModelTypes.h"
#include "Message.h"

namespace GraphicalModel {
	template <typename NodeType>
	class FactorGraphNode {
		public:
			FactorGraphNode() {}

			bool is_leaf() const {
				return outgoingMessages().size() == 1;
			}

			void sendMessages() {
				for (unsigned int i=0; i<outgoingMessages().size(); ++i) {
					if (canSend(i)) {
						calcMessage(i);
						_outgoingMessages[i]->send();
						//_neighbors[i]->receiveMessage();
					}
				}
			}

			void receiveMessage() {
				static_cast<NodeType*>(this)->receiveMessageImplementation();
			}

			void addNeighbor(unsigned int otherSideIdx,
					Message_ptr incomingMessage, Message_ptr outgoingMessage)
			{
				_neighborSlots.push_back(otherSideIdx);
				_incomingMessages.push_back(incomingMessage);
				_outgoingMessages.push_back(outgoingMessage);
			}

			void clearMessages() {
				std::vector<Message_ptr>::const_iterator msgIt =
					incomingMessages().begin();
				for (; msgIt != incomingMessages().end(); ++msgIt) {
					(*msgIt)->clear();
				}

				msgIt = outgoingMessages().begin();
				for (; msgIt != outgoingMessages().end(); ++msgIt) {
					(*msgIt)->clear();
				}
			}

			unsigned int nNeighbors() const {
				return outgoingMessages().size();
			}

			virtual void dumpOutgoingMessagesToHTML(const std::wstring& rowTitle,
					std::wostream& out, bool verbose=false) const 
			{
				out << L"<tr>";
				out << L"<td><b>" << rowTitle << L"</b></td>";
				MessageVector::const_iterator msg = outgoingMessages().begin();
				for (; msg!=outgoingMessages().end(); ++msg) {
					(*msg)->toHTMLTDs(out);
				}
				out << L"</tr>";
			}
		/*
void debugMessages(std::wostream& out) const {
	BOOST_FOREACH(const Message_ptr& msg, incomingMessages()) {
		out << L"\t(";
		bool first = true;
		for (int i=0; i<msg->size(); ++i) {
			if (first) {
				first = false;
			} else {
				out << L", ";
			}
			out << (*msg)[i];
		}
		out << L")";
		out << endl;
	}
	out << endl;
}*/

		protected:
			void calcMessage(unsigned int neighbor) {
				static_cast<NodeType*>(this)->calcMessageImplementation(neighbor);
			}
			//void calcMarginals() const;
			/*const std::vector<unsigned int> neighborSlots() const {
			}*/
			std::vector<Message_ptr>& outgoingMessages() {
				return _outgoingMessages;
			}

			const std::vector<Message_ptr>& outgoingMessages() const {
				return _outgoingMessages;
			}

			const std::vector<Message_ptr>& incomingMessages() const{
				return _incomingMessages;
			}

		private:
			std::vector<unsigned int> _neighborSlots;
			MessageVector _outgoingMessages;
			MessageVector _incomingMessages;

			bool canSend(unsigned int msg) const {
				if (_outgoingMessages[msg]->sent()) {
					return false;
				}

				for (size_t i=0; i<_incomingMessages.size(); ++i) {
					if (i == msg) {
						if (_outgoingMessages[msg]->sent()) {
							return false;
						}
					} else if (!_incomingMessages[i]->sent()) {
						return false;
					}
				}
				return true;
			}
	};

	template <typename FactorType, typename VariableType>
	void connect(FactorType& factor, VariableType& variable) 
	{
		unsigned int factorSideIdx = factor.nNeighbors();
		unsigned int variableSideIdx = variable.nNeighbors();
		Message_ptr toFactor = boost::make_shared<Message>(variable.nValues());
		Message_ptr toVariable = boost::make_shared<Message>(variable.nValues());

		factor.addNeighbor(variableSideIdx, toFactor, toVariable);
		variable.addNeighbor(factorSideIdx, toVariable, toFactor);
	}
};

#endif


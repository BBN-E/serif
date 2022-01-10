#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include <vector>
#include "GraphicalModelTypes.h"
#include "FactorGraphNode.h"
#include "Message.h"
#include "Factor.h"
#include "VectorUtils.h"

namespace GraphicalModel {

class NoMarginalsWithUnsentMessages {};

class Variable : public FactorGraphNode<Variable> {
	public:
		Variable(unsigned int n_vals) 
			: _n_values(n_vals), _dirtyMarginals(true), _marginals(n_vals)
	{} 
		unsigned int nValues() const { return _n_values;}
		const std::vector<double>& marginals() const {
			if (_dirtyMarginals) {
				calcMarginals();
			}
			return _marginals;
		}

		void setMarginals(const std::vector<double>& mgnls) {
			// this will be invalidated as soon as we send messages
			_marginals = mgnls;
			_dirtyMarginals = false;
		}

		void receiveMessageImplementation() {
			//FactorGraphNode<Variable>::receiveMessage();
			_dirtyMarginals = true;
		}

		void productOfIncomingMessages(std::vector<double>& out) const {
			assert(out.size() == _n_values);
			zero(out);
			
			std::vector<Message_ptr>::const_iterator it = 
				incomingMessages().begin();

			for (; it!=incomingMessages().end(); ++it) {
				const Message_ptr& msg = *it;
				if (!msg->sent()) {
					throw NoMarginalsWithUnsentMessages();
				}

				msg->productTo(out);
			}
		}

	protected:
		void calcMessageImplementation(unsigned int neighbor) {
			Message& msg = *outgoingMessages()[neighbor];
			msg.clear();
			for (size_t incoming = 0; incoming < incomingMessages().size(); ++incoming) {
				if (incoming != neighbor) {
					msg.product(*incomingMessages()[incoming]);
				}
			}
		}

	private:
		unsigned int _n_values;
		mutable bool _dirtyMarginals;
		mutable std::vector<double> _marginals;

		void calcMarginals() const {
			productOfIncomingMessages(_marginals);
			normalizeLogProbs(_marginals);

			_dirtyMarginals = false;
		}
};
};

#endif


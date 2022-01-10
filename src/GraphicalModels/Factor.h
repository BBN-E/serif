#ifndef _FACTOR_H_
#define _FACTOR_H_

#include <vector>
#include <algorithm>
#include "Generic/common/SessionLogger.h"
#include "GraphicalModelTypes.h"
#include "FactorGraphNode.h"
#include "Message.h"
#include "Variable.h"
#include "pr/Constraint.h"

namespace GraphicalModel {
	class Alphabet;

template <typename FactorType>
class Factor : public FactorGraphNode<FactorType> {
	public:
		Factor() {} 
		unsigned int id;
		void setID(unsigned int ID) { id = ID; }
	protected:
		/*
		// the message from a factor f to a variable x_i is
		// the nested sum over all variables in the factor 
		// except x_i of the product of the incoming messages and the factor function
		// computing this is kind of messy...
		// this implementation is slow but (relatively) clear.  It will
		// probably need speeding up later
		void calcMessageImplementation(unsigned int neighbor) {
			// the message will contain one slot for each possible value
			// of the *receiving* variable.  Each of those will be the sum
			// (log-sum since we're in log-space) of values summed over all
			// the other variables involved, so we need to figure out how many
			// that is in order to allocate memory to hold the scores
			unsigned int num_other_assignments = 1;
			std::vector<unsigned int> neighborNValues(outgoingMessages().size());

			for (int i=0; i<neighbors().size(); ++i) {
				Variable* var = neighbors()[i];
				neighborNValues[i] = var->nValues();
				if (i!=neighbor) {
					num_other_assignments *= neighborNValues[i];
				}
			}

			// we keep one vector of values for each slot of the output message
			std::vector<std::vector<double > > scores(neighborNValues[neighbor],
					std::vector<double>(num_other_assignments, 0.0));
			// the next index to fill in each output vector
			std::vector<unsigned int> nextIndex(neighborNValues[neighbor], 0);
			// the assignment to all the variables involved in the factor
			// currently under consideration
			std::vector<unsigned int> assignment(neighbors().size(), 0);

			bool done = false;
			while (!done) {
				// contribution of this assignment is the weight from the
				// factor itself, plus the incoming messages of all other
				// variables for this assignment
				double val = factor(assignment);
				for (unsigned int i=0; i<assignment.size(); ++i) {
					if (i != neighbor) {
						// addition because we're in log space
						val += (*(incomingMessages()[i]))[assignment[i]];
					}
				}
				// add the score to the list for the appropriate output slot
				scores[assignment[neighbor]][nextIndex[assignment[neighbor]]++] = val;
				// increment to the next assignment to consider
				// we treat the assignment as a sort of variable-based
				// number
				++assignment[0];
				for (unsigned int j=0; j<assignment.size(); ++j) {
					// 'carrying'
					if (assignment[j] == neighborNValues[j]) {
						// we're done when we attempt to carry out of the
						// last digit
						if (j == assignment.size() - 1) {
							done = true;
							break;
						} else {
							assignment[j] = 0;
							++assignment[j+1];
						}
					}
				}
			}

			// fill the outgoing message by calculating the log sum of the scores
			// for each slot
			Message& msg = *outgoingMessages()[neighbor];	
			for (unsigned int i=0; i<msg.size(); ++i) {
				msg.set(i, logSumExp(scores[i]));
			}
		}*/

		double factor(const std::vector<unsigned int>& assignment) const {
			return static_cast<const FactorType*>(this)->factor(assignment);
		}

		void receiveMessageImplementation() {
			// do nothing
		}
		friend class FactorGraphNode<FactorType>;
};

template <typename FactorType>
class UnaryFactor : public Factor<FactorType> {
	public:
		UnaryFactor() : Factor<FactorType>() {}
	protected:
		void calcMessageImplementation(unsigned int neighbor) {
			assert(neighbor == 0);
			unaryCalcMessage();
		}

		friend class FactorGraphNode<FactorType>;
		double factor(const std::vector<unsigned int>& assignment) const {
			assert(assignment.size() == 1);
			return static_cast<const FactorType*>(this)->factorImpl(assignment[0]);
		}

		double factor(unsigned int assignment) const {
			return static_cast<const FactorType*>(this)->factorImpl(assignment);
		}
		// subclasses should implement
		// double factorImpl(unsigned int)
	private:
		void unaryCalcMessage() const {
			// fill the outgoing message by calculating the log sum of the scores
			// for each slot
			// this is necessayr because of two-stage template lookup
			// see http://gcc.gnu.org/onlinedocs/gcc/Name-lookup.html
			Message& msg = *this->outgoingMessages()[0];	
			for (unsigned int i=0; i<msg.size(); ++i) {
				msg.set(i, factor(i));
			}
		}
};

template<typename GraphType>
class Modification {
public:
	Modification() : magnitude(0.0), constraint(0) {}
	Modification(Constraint<GraphType>* constraint, double amount) 
		: constraint(constraint), magnitude(amount) {}
	Modification(const Modification& mod) 
		: magnitude(mod.magnitude), constraint(mod.constraint) {}

	void clear() {
		magnitude = 0.0;
		constraint = 0;
	}

	bool operator<(const Modification& rhs) const {
		return magnitude < rhs.magnitude;
	}

	double magnitude;
	Constraint<GraphType>* constraint;
};

template <typename GraphType>
class ModificationInfo {
	public:
		ModificationInfo() : positive(), negative() {}

		void clear() {
			positive.clear();
			negative.clear();
		}

		void registerModification(Constraint<GraphType>* constraint, double amount) {
			Modification<GraphType> mod(constraint, amount);
			if (positive.constraint) {
				positive = (std::max)(positive, mod);
			} else {
				positive = mod;
			}

			if (negative.constraint) {
				negative = (std::min)(negative, mod);
			} else {
				negative = mod;
			}
		}

		double conflict() const {
			if (positive.constraint && negative.constraint) {
				return (std::min)(positive.magnitude, -negative.magnitude);
			} else {
				return 0.0;
			}
		}

		bool operator<(const ModificationInfo& rhs) const {
			return conflict() < rhs.conflict();
		}

		void combine(const ModificationInfo& rhs) {
			positive = (std::max)(positive, rhs.positive);
			negative = (std::min)(negative, rhs.negative);
		}

		Modification<GraphType> positive;
		Modification<GraphType> negative;
};

template <typename GraphType, typename FactorType>
class ModifiableUnaryFactor : public UnaryFactor<FactorType> {
	public:
		ModifiableUnaryFactor(unsigned int table_size) 
			: UnaryFactor<FactorType>(), _modifiers(table_size),
			_maxModifiers(table_size) {}

		void clearModifiers() {
			std::vector<double>::iterator it = _modifiers.begin();
			for (; it!=_modifiers.end(); ++it) {
				*it = 0.0;
			}
			typename std::vector<ModificationInfo<GraphType> >::iterator jt = 
				_maxModifiers.begin();
			for (; jt!=_maxModifiers.end(); ++jt) {
				jt->clear();
			}
		}

		void adjustModifier(unsigned int assignment, double amount, 
				Constraint<GraphType>* constraint)
		{
			_modifiers[assignment] += amount;
			_maxModifiers[assignment].registerModification(constraint, amount);
		}

		size_t nModifiers() const {
			return _modifiers.size();
		}

		const ModificationInfo<GraphType>& maxModifier(unsigned int assignment) const {
			return _maxModifiers[assignment];
		}

	protected:
		friend class UnaryFactor<FactorType>;
		double factorImpl(unsigned int assignment) const {
			return _modifiers[assignment] + factorPreMod(assignment);
		}

		double factorPreMod(unsigned int assignment) const  {
			return static_cast<const FactorType*>(this)->factorPreModImpl(assignment);
		}
	private:
		std::vector<double> _modifiers;
		std::vector<ModificationInfo<GraphType> > _maxModifiers;
};
};


#endif


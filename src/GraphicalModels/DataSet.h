#ifndef _DATASET_H_
#define _DATASET_H_

#include <vector>
#include <boost/shared_ptr.hpp>

namespace GraphicalModel {

template <typename GraphType>
class DataSet {
public:
	typedef std::vector<boost::shared_ptr<GraphType> > GraphVector;
	GraphVector graphs;
	void addGraph(const boost::shared_ptr<GraphType>& graph);
	unsigned int nFactors() const {
		unsigned int ret = 0;
		typename GraphVector::const_iterator it = graphs.begin();
		for (; it!=graphs.end(); ++it) {
			ret += (*it)->nFactors();
		}
		return ret;
	}
	unsigned int nGraphs() const { return graphs.size(); }
private:
	static unsigned int next_graph_id;
	static unsigned int next_factor_id;
};


template <typename GraphType>
unsigned int DataSet<GraphType>::next_graph_id = 0;
template <typename GraphType>
unsigned int DataSet<GraphType>::next_factor_id = 0;

template <typename GraphType>
void DataSet<GraphType>::addGraph(const boost::shared_ptr<GraphType>& graph) {
	graph->setID(next_graph_id++);
	graph->numberFactors(next_factor_id);
	graphs.push_back(graph);
}
};

#endif


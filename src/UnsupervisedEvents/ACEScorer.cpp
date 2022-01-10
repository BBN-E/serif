#include "Generic/common/leak_detection.h"
#include "ACEScorer.h"

#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include "Generic/common/foreach_pair.hpp"
#include "Generic/common/ParamReader.h"
#include "Generic/common/bsp_declare.h"
#include "Generic/common/UnicodeUtil.h"
#include "AnswerExtractor.h"
#include "ACEDecoder.h"
#include "ACEEventGoldStandard.h"
#include "ProblemDefinition.h"
#include "AnswerMatrix.h"

using std::make_pair;
using std::setw;
using std::setprecision;
using std::wcout;
using std::wofstream;
using std::stringstream;
using std::string;
using boost::filesystem::copy_file;
using boost::filesystem::copy_option;
using boost::filesystem::path;

class Counts {
	public:
		Counts() {}
		void increment(int key, double amount) {
			std::map<int, double>::iterator it = _map.find(key);

			if (it != _map.end()) {
				it->second+=amount;
			} else {
				_map.insert(make_pair(key, amount));
			}
		}

		const std::map<int, double>& entries() const {return _map;};

		double get(int key) const {
			std::map<int, double>::const_iterator it = _map.find(key);

			if (it == _map.end()) {
				return 0.0;
			} else {
				return it->second;
			}
		}
	private:
		std::map<int, double> _map;
};

BSP_DECLARE(AnswerKey)
class AnswerKey {
public:
	AnswerKey(size_t n_labels) : n_labels(n_labels) {}
	virtual AnswerMatrix answers(const GoldStandardACEInstance& inst) const = 0;
protected:
	size_t n_labels;
};

class SerifCentricAnswerKey : public AnswerKey {
public:
	SerifCentricAnswerKey(size_t n_labels) : AnswerKey(n_labels) {}
	AnswerMatrix answers(const GoldStandardACEInstance& inst) const {
		size_t n_entities = inst.serifEvent->nEntities();
		if (!n_entities) {
			return AnswerMatrix(0,0);
		} 

		AnswerMatrix ret (n_entities, n_labels);

		const GoldAnswers& goldAnswers = *inst.goldAnswers;
		for (size_t serif_entity = 0; serif_entity < n_entities; ++serif_entity) {
			const std::vector<double>& alignment = 
				inst.entityMap->similarityMatrix[serif_entity];
			double sum = 0.0;
			BOOST_FOREACH(double d, alignment) {
				sum+=d;
			}

			if (sum > 0.0) {
				for (size_t gold_entity = 0; gold_entity < goldAnswers.size(); ++gold_entity) {
					if (alignment[gold_entity] > 0) {
						int role = goldAnswers[gold_entity];
						if (role>=0) {
							if (static_cast<size_t>(role) >= ret.forEntity(serif_entity).size()) {
								std::wstringstream err;
								err << "Role " << role << " is out of bounds. Size of vector is " << ret.forEntity(serif_entity).size();
								throw UnexpectedInputException("SerifCentricAnswerKey::answers",
										err);
							} else {
								ret.forEntity(serif_entity)[role] += 
									alignment[gold_entity] / sum;
							}
						}
					}
				}
			}
		}
		
		ret.normalize();

		return ret;
	}
};
/*
class FractionalAnswerKey : public AnswerKey {
public:
	FractionalAnswerKey() : AnswerKey() {}
	AnswerMatrix answers(const GoldStandardACEInstance& inst) const {

	}
};*/

class GoldCentricAnswerKey : public AnswerKey {
public:
	GoldCentricAnswerKey(size_t n_labels) : AnswerKey(n_labels) {}
	AnswerMatrix answers(const GoldStandardACEInstance& inst) const {
		size_t n_entities = inst.serifEvent->nEntities();
		if (!n_entities) {
			return AnswerMatrix(0,0);
		} 

		AnswerMatrix ret (n_entities, n_labels);
		std::vector<double> noneWeight(n_entities, 0.0);

		const GoldAnswers& goldAnswers = *inst.goldAnswers;
		const PassageEntityMapping::SimilarityMatrix& similarityMatrix =
			inst.entityMap->similarityMatrix;
		for (size_t gold_entity = 0; gold_entity < goldAnswers.size(); ++gold_entity) {
			int role = goldAnswers[gold_entity];
			double sum = 0.0;
			for (size_t serif_entity = 0; serif_entity<n_entities; ++serif_entity) {
				sum += similarityMatrix[serif_entity][gold_entity];
			}

			if (sum > 0.0) {
				for (size_t serif_entity = 0; serif_entity < n_entities; ++serif_entity) {
					double sim = similarityMatrix[serif_entity][gold_entity];
					if (sim > 0) {
						if (role >= 0) {
							if (static_cast<size_t>(role) >= ret.forEntity(serif_entity).size()) {
								std::wstringstream err;
								err << "Role " << role << " is out of bounds. Size of vector is " << ret.forEntity(serif_entity).size();
								throw UnexpectedInputException("SerifCentricAnswerKey::answers",
										err);
							} else {
								ret.forEntity(serif_entity)[role] += sim / sum;
							}
						} else {
							noneWeight[serif_entity] += sim/sum;
						}
					}
				}
			}
		}
		
		for (size_t serif_entity = 0; serif_entity < ret.size(); ++serif_entity) {
			std::vector<double>& scores = ret.forEntity(serif_entity);
			double sum = noneWeight[serif_entity];
			BOOST_FOREACH(double d, scores) {
				sum += d;
			}
			if (sum > 0.0) {
				BOOST_FOREACH(double& d, scores) {
					d/=sum;
				}
			}
		}

		return ret;
	}
};

/*
class MentionAnswerKey : public AnswerKey {
public:
	MentionAnswerKey() : AnswerKey() {}
	AnswerMatrix answers(const GoldStandardACEInstance& inst) const {
	}
};*/

struct PRFResult {
	PRFResult(double tp, double fp, double fn) : true_positives(tp),
	false_positives(fp), false_negatives(fn) {}

	double true_positives;
	double false_positives;
	double false_negatives;

	double precision() const {
		if (true_positives + false_positives > 0.0) {
			return true_positives / (false_positives + true_positives);
		} else {
			return 0.0;
		}
	}

	double recall() const {
		if (true_positives + false_negatives > 0.0) {
			return true_positives / (true_positives + false_negatives);
		} else {
			return 0.0;
		}
	}

	double f() const {
		double p = precision();
		double r = recall();

		if (p + r > 0.0) {
			return 2*p*r/(p+r);
		} else {
			return 0.0;
		}
	}
};

BSP_DECLARE(PRFMap)
class PRFMap {
public:
	PRFMap() {}

	void updateForLabelVectors(const LabelVector& gold, const LabelVector& predicted) {
		if (gold.size() != predicted.size()) {
			throw UnexpectedInputException("PRMMap::updateForLabelVector",
					"Label vector size mismatch in number of labels");
		}

		for (size_t label = 0; label < gold.size(); ++label) {
			double g = gold[label];
			double p = predicted[label];

			double tp = (std::min)(g, p);
			double fp = (std::max)(0.0, p-g);
			double fn = (std::max)(0.0, g-p);

			if (tp > 0) {
				truePositives.increment(label, tp);
			}
			if (fp > 0) {
				falsePositives.increment(label, fp);
			}
			if (fn > 0) {
				falseNegatives.increment(label, fn);
			}
		}
	}

	void registerPredictions(const AnswerMatrix& gold, const AnswerMatrix& predicted) {
		if (gold.size() != predicted.size()) {
			throw UnexpectedInputException("PRFMap::registerPredictions",
					"Answer matrix size mismatch in number of entities");
		}

		for (size_t entity = 0; entity<gold.size(); ++entity) {
			updateForLabelVectors(gold.forEntity(entity), 
					predicted.forEntity(entity));
		}
	}

	std::map<int, PRFResult> scores() const {
		std::map<int, PRFResult> ret;
		std::set<int> labels;

		BOOST_FOREACH_PAIR(int label, double dummy, truePositives.entries()) {
			labels.insert(label);
		}

		BOOST_FOREACH_PAIR(int label, double dummy, falseNegatives.entries()) {
			labels.insert(label);
		}

		BOOST_FOREACH_PAIR(int label, double dummy, falsePositives.entries()) {
			labels.insert(label);
		}

		BOOST_FOREACH(int label, labels) {
			ret.insert(make_pair(label, 
						PRFResult(truePositives.get(label), 
							falsePositives.get(label), 
							falseNegatives.get(label))));
		}

		return ret;
	}

	Counts falsePositives;
	Counts falseNegatives;
	Counts truePositives;
};
/*
PRFMap PRF(const GraphicalModel::DataSet<ACEEvent>& data, SingleAnswerExtractor& gold,
		MultiAnswerExtractor& predicted) 
{
	PRFMap counts;

	for (unsigned int graph_idx = 0; graph_idx<data.nGraphs(); ++graph_idx) {
		ACEEvent_ptr graph = data.graphs[graph_idx];
		for (unsigned int entity =0; entity<graph->nEntities(); ++entity) {
			counts.registerPredictions(
					gold.answer(*graph, entity), predicted.answers(*graph, entity));
		}
	}

	return counts;

	//std::map<int, PRFResult> scores = counts.scores();
}*/

typedef std::vector<PRFMap_ptr> ScorersForDecoderSet;
typedef std::vector<ScorersForDecoderSet> ScorersForDecoderSets;


double jitter(double range) {
	return (range*rand())/RAND_MAX - range/2.0;
}

void ROCCurve(unsigned int label, const ScorersForDecoderSets& scorers, 
		const DecoderLists& decoders, const ProblemDefinition& problem,
		wofstream& out) 
{
	typedef std::pair<double, double> Point;
	typedef std::vector<Point>  Line;
	typedef std::map<std::wstring, Line> LabelsToLines;
	LabelsToLines labelsToLines;

	for (size_t i=0; i<scorers.size(); ++i) {
		for (size_t j=0; j<scorers[i].size(); ++j) {
			// if two points have the exact same values, one would be
			// invisible on the graph. To avoid this, we 'jitter' each point
			// slightly.
			double x_jitter = jitter(1.0);
			double y_jitter = jitter(1.0);
			std::map<int, PRFResult> scores = scorers[i][j]->scores();
			std::map<int,PRFResult>::const_iterator probe = scores.find(label);
			if (probe != scores.end()) {
				std::wstring decoderName = UnicodeUtil::toUTF16StdString(decoders[i][j]->name());
				labelsToLines[decoderName].push_back(
						std::make_pair(probe->second.precision() + x_jitter,
							probe->second.recall() + y_jitter));
			}
		}
	}

	wstring name = problem.className(label);

	out << endl;
	out << L"<div id='" << name<< L"' style='width:600px;height:600px'></div><br>" << endl;
	out << L"<center>" << name << L"</center>" << std::endl;
	out << L"<script type='text/javascript'>" << endl;
	out << L"$.plot($('#" << name << L"')," << endl;
	out << L"[" << endl;
	bool first_series = true;
	BOOST_FOREACH_PAIR(const std::wstring& decoderName, Line line, labelsToLines) {
		if (first_series) {
			first_series = false;
		} else {
			out << L", ";
		}
		out << L"{ lines: {show: true }, points: {show: true}, label:'"
			<< decoderName << L"', data: [" << endl;
		bool first = true;
		BOOST_FOREACH_PAIR(double precision, double recall, line) {
			if (first) {
				first = false;
			} else {
				out << L", ";
			}
			precision = (std::max)(0.0,precision);
			precision = (std::min)(100.0,precision);
			recall = (std::max)(0.0, recall);
			recall = (std::min)(100.0,recall);

			out << L"[" << precision << L", " << recall << L"]" << endl;
		}
		out << L"]}" << endl;
	}
	out << L"]," << endl;
	out << L"{legend: {show: true, position: 'nw'}, xaxis: {min: 0.0, max: 100.0}, yaxis: {min: 0.0, max:100.0}})" << endl;
	out << L"</script>" << endl;
}

ScorersForDecoderSets createScorersForDecoderSet(const DecoderLists& decoderSets) {
	ScorersForDecoderSets scorers;

	for (size_t i=0; i<decoderSets.size(); ++i) {
		const DecoderList& decoders = decoderSets[i];
		ScorersForDecoderSet sa;
		for (size_t j=0; j<decoders.size(); ++j) {
			sa.push_back(boost::make_shared<PRFMap>());
		}
		scorers.push_back(sa);
	}

	return scorers;
}

void writeScoresToFile(const std::string& name, const std::string& output_dir, 
		const ProblemDefinition& problem, const ScorersForDecoderSets& scorers, 
		const DecoderLists& decoderSets)
{
	// we create both pretty HTML output for human consumption
	// and tab-delimited output for script consumpation
	stringstream htmlOut, txtOut;
	htmlOut << output_dir << "/scores." << name << ".html";
	txtOut << output_dir << "/scores." << name << ".dat";
	wofstream out(htmlOut.str().c_str());
	wofstream txt(txtOut.str().c_str());

	out << L"<html><head><title>Scores</title>" << endl;
	out << L"<script src='https://ajax.googleapis.com/ajax/libs/jquery/1.7.1/jquery.min.js' type='text/javascript'></script>" << endl;
	out << L"<script src='flot.js' type='text/javascript'></script>" << endl;
	out << L"</head>" << endl;
	out << L"<body>" << endl;

	out << L"<b>X-axes are precision, y-axes are recall</b><br/>" << endl;
	for (size_t i=0; i<problem.nClasses(); ++i) {
		out << L"<h1>" << problem.className(i) << L"</h1>" << std::endl;
		ROCCurve(i, scorers, decoderSets, problem, out);
		out << L"<br/>";
		out << L"<table>";
		out << L"<tr><th>Decoder</th><th>Precision</th><th>Recall</th><th>F-measure</th></tr>";
		for (size_t j=0; j<scorers.size(); ++j) {
			for (size_t k=0; k<scorers[j].size(); ++k) {
				wstring decoderName = UnicodeUtil::toUTF16StdString(decoderSets[j][k]->name());  
				out << L"<tr><td>" << decoderName << L" " << k << L"</td>";
				std::map<int, PRFResult> scores = scorers[j][k]->scores();
				std::map<int, PRFResult>::const_iterator probe =
					scores.find(i);
				if (probe != scores.end()) {
					const PRFResult& prf = probe->second;
					out << L"<td>"<<prf.precision() << L"</td><td>" 
						<< prf.recall() << L"</td><td>" << prf.f() << L"</td>";
					txt << problem.className(i) << L"\t" 
						<< UnicodeUtil::toUTF16StdString(name) 
						<< L"\t" << decoderName << L" " << k << L"\t"
						<< prf.true_positives << L"\t" << prf.false_positives
						<< L"\t" << prf.false_negatives << L"\n";
				} else {
					out << L"<td>n/a</td><td>n/a</td><td>n/a</td>";
					SessionLogger::warn("role_does_not_appear")
						<< L"Role " << problem.className(i) 
						<< L" does not appear at all.";
				}
				out << L"</tr>";
			}
		}
		out << L"</table>";
	}

	out << "</body></html>" << endl;
}


void ACEScorer::dumpScore(const ProblemDefinition_ptr& problem,
		const DecoderLists& decoderSets, const std::string& output_dir)
{
	if (!boost::filesystem::is_directory(output_dir)) {
		boost::filesystem::create_directory(output_dir);
	}

	string flotPath = ParamReader::getRequiredParam("flot_path");
	stringstream newFlotPath;
	newFlotPath << output_dir << "/flot.js";
	path src(flotPath);
	path dest(newFlotPath.str());
	copy_file(src, dest, copy_option::overwrite_if_exists);

	ACEEventGoldStandard goldStandard(problem, 
			ParamReader::getRequiredParam("correct_answer_test_doc_table"),
			ParamReader::getRequiredParam("test_doc_table"));
	
	std::vector<AnswerKey_ptr> answerKeys;
	answerKeys.push_back(boost::make_shared<GoldCentricAnswerKey>(problem->nClasses()));
	answerKeys.push_back(boost::make_shared<SerifCentricAnswerKey>(problem->nClasses()));
	/*answerKeys.push_back(boost::make_shared<FractionalAnswerKey>());
	answerKeys.push_back(boost::make_shared<MentionAnswerKey>());*/

	std::vector<ScorersForDecoderSets> scorersByType;
	BOOST_FOREACH(const AnswerKey_ptr& dummy, answerKeys) {
		scorersByType.push_back(createScorersForDecoderSet(decoderSets));
	}

	std::vector<GoldStandardACEInstance> goldInstances;
	size_t n_gold_instances = 0;
	size_t skipped = 0;
	for (size_t doc=0; doc<goldStandard.size(); ++doc) {
		try {
		goldInstances = goldStandard.goldStandardInstances(doc);
		BOOST_FOREACH(GoldStandardACEInstance goldInst, goldInstances) {
			for (size_t answerKey = 0; answerKey < answerKeys.size(); ++answerKey) {
				AnswerMatrix goldAnswerMatrix = answerKeys[answerKey]->answers(goldInst);
				for (size_t decoderSet = 0; decoderSet<decoderSets.size(); ++decoderSet) {
					for (size_t decoder = 0; decoder < decoderSets[decoderSet].size(); ++decoder) {
						scorersByType[answerKey][decoderSet][decoder]
							->registerPredictions(goldAnswerMatrix, 
								decoderSets[decoderSet][decoder]->decode(goldInst));
					}
				}
			}
		}
		n_gold_instances += goldInstances.size();
		} catch (SkipThisDocument& doc) {
			++skipped;
		}
	}

	SessionLogger::info("num_gold_instances") << L"Read " << n_gold_instances
		<< L" gold standard instances";
	if (skipped) {
		SessionLogger::warn("skipped_docs") << L"Skipped " << skipped << L" documents.";
	}

	writeScoresToFile("traditional", output_dir, *problem, scorersByType[0], decoderSets);
	writeScoresToFile("generous", output_dir, *problem, scorersByType[1], decoderSets);
}


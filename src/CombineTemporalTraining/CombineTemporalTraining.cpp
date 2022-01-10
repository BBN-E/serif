#include "Generic/common/leak_detection.h"

#pragma warning (disable: 4996)

#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <boost/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/functional/hash.hpp>
#include <boost/archive/text_wiarchive.hpp>
#include <boost/archive/text_woarchive.hpp>
#include <boost/serialization/tracking.hpp>
#include "sqlite/SqliteDB.h"
#include "Generic/common/foreach_pair.hpp"
#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include "ActiveLearning/StringStore.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"
#include "temporal/TemporalDB.h"
#include "temporal/TemporalInstanceSerialization.h"

using namespace std;
using boost::filesystem::exists;
using boost::filesystem::directory_iterator;
using boost::filesystem::is_directory;
using boost::make_shared;
using boost::dynamic_pointer_cast;
using boost::archive::text_woarchive;
using boost::archive::text_wiarchive;
using boost::hash_combine;

BOOST_CLASS_TRACKING(TemporalInstanceData, boost::serialization::track_never);

class FeatureData {
	public:
		FeatureData(const wstring& metadata,
				const wstring& prettyprint) : 
				metadata(metadata), prettyprint(prettyprint) {}
		wstring metadata;
		wstring prettyprint;
		bool operator==(const FeatureData& rhs) const {
			return rhs.metadata == metadata
				&& rhs.prettyprint == prettyprint;
		}
};

class FeatureDataHash {
public:
	size_t operator()(const FeatureData& data) const {
		size_t ret = 0;
		boost::hash_combine(ret, data.metadata);
		boost::hash_combine(ret, data.prettyprint);
		return ret;
	};
};

typedef std::map<unsigned int, unsigned int> FeatureMapping;
typedef boost::shared_ptr<FeatureMapping> FeatureMapping_ptr;

typedef boost::unordered_map<FeatureData, unsigned int, FeatureDataHash> FeatureMapHash;
class CombinedFeatureMap {
public:
	CombinedFeatureMap() : nextFeature(0) {}
	FeatureMapHash combinedFeatures;
	int nextFeature;

	FeatureMapping_ptr createFeatureMapping(SqliteDB_ptr sliceDB) {
		FeatureMapping_ptr mapping = make_shared<FeatureMapping>();
		Table_ptr tbl = sliceDB->exec(L"select idx, metadata, prettyprint from features");

		int n_features = 0;
		int n_new_mappings = 0;
		BOOST_FOREACH(const std::vector<wstring>& row, *tbl) {
			++n_features;
			int idx = boost::lexical_cast<int>(row[0]);
			FeatureData data(row[1], row[2]);

			FeatureMapHash::const_iterator it =
				combinedFeatures.find(data);

			if (it == combinedFeatures.end()) {
				int mappedIdx = nextFeature++;
				combinedFeatures.insert(make_pair(data, mappedIdx));
				mapping->insert(make_pair(idx, mappedIdx));
				++n_new_mappings;
			} else {
				mapping->insert(make_pair(idx, it->second));
			}
		}

		SessionLogger::info("new_mappings") << "\tFrom " << n_features 
			<< " features produced " << n_new_mappings << " new feature "
			<< " mappings";

		return mapping;
	}

	void write(SqliteDB_ptr db) {
		db->beginTransaction();
		SessionLogger::warn("hack") << "Warning! Currently the "
			<< "number of temporal attributes is hard coded to 3";
		BOOST_FOREACH_PAIR(const FeatureData& data, unsigned int idx, combinedFeatures) {
			for (int i=0; i<3; ++i) {
				wstringstream sql;
				sql << L"insert into features (idx, alphabet, metadata, prettyprint, "
					<< L"name, weight, annotated, positive, expectation, regularization) values "
					<< L"(" << idx << L", " << i << L", \"" << data.metadata
					<< L"\", \"" << data.prettyprint << L"\", \"\", 0.0, 0, 0, 0.0, 1.0)";
				db->exec(sql.str());
			}
		}
		db->endTransaction();
	}
};


bool path_exists(const string& f) {
	if (!exists(f)) {
		SessionLogger::err("bad_path") << "Path does not exists: " << f;
		return false;
	}
	return true;
}

void process_directory(const string& directory, CombinedFeatureMap& bigMap,
		text_woarchive& combinedFVs, wofstream& combinedPreview,
		std::set<size_t>& instanceHashes)
{
	string db_path = directory + "/temporal.db";
	string preview_path = directory + "/temporal.preview";
	string fv_path = directory + "/temporal.fvs";

	if (path_exists(db_path) && path_exists(preview_path)
			&& path_exists(fv_path))
	{
		SqliteDB_ptr dirDB = make_shared<SqliteDB>(db_path);
		FeatureMapping_ptr mapping = bigMap.createFeatureMapping(dirDB);

		TemporalInstanceData instData;

		wifstream inpStr(fv_path.c_str(), ios_base::binary);
		text_wiarchive in(inpStr);
		int n_instances = 0;
		set<int> dupes;

		try {
			while (true) {
				in >> instData;
				size_t hsh = instData.hashValue();

				if (instanceHashes.find(hsh) == instanceHashes.end()) {
					instanceHashes.insert(hsh);
					BOOST_FOREACH(unsigned int& feat, instData.fv()) {
						feat = mapping->find(feat)->second;
					}
					combinedFVs << instData;
				} else {
					dupes.insert(n_instances);
				}
				++n_instances;
			}
		} catch (boost::archive::archive_exception const&) {}

		SessionLogger::info("num_processed") << "\tProcessed "
			<< n_instances << " feature vectors";
		SessionLogger::info("dupes") << "\tEliminated " 
			<< dupes.size() << " duplicates";

		StringStore_ptr previewStrings = dynamic_pointer_cast<StringStore>(
				SimpleInMemoryStringStore::create(preview_path, n_instances));

		for (size_t i=0; i<previewStrings->size(); ++i) {
			if (dupes.find(i) == dupes.end()) {
				combinedPreview << previewStrings->getString(i) << std::endl;
			}
		}

		SessionLogger::info("num_processed") << "\tProcessed "
			<< previewStrings->size() << " preview strings";
	}

}

int main(int argc, char** argv) {
#ifdef NDEBUG
	try {
#endif
	if (argc != 3) {
		cerr << "Expected 2 arguments, got " 
			<< (argc -1 ) << ", usage is\n"
			<< "CombineTemporalTraining param_file output_directory";
		exit(1);
	}
	string paramFile = argv[1];
	string directory = argv[2];

	ParamReader::readParamFile(paramFile);
	ParamReader::logParams();

	ConsoleSessionLogger logger(std::vector<wstring>(), L"[CombTempTrain]");
	SessionLogger::setGlobalLogger(&logger);
	SessionLoggerUnsetter unsetter;

	if (exists(directory)) {
		string outputDBPath = directory + "/temporal.db";
		string outputFVs = directory + "/temporal.fvs";
		string outputPreview = directory + "/temporal.preview";
		CombinedFeatureMap newMap;
		set<size_t> instanceHashes;

		wofstream combinedFVFile(outputFVs.c_str(), ios_base::binary);
		text_woarchive combinedFVs(combinedFVFile);
		wofstream combinedPreview(outputPreview.c_str());

		directory_iterator endIt;
		for(directory_iterator dirIt(directory); dirIt != endIt; ++dirIt) {
			if (is_directory(dirIt->status())) {
				SessionLogger::info("process_dir") << "Processing "
					<< dirIt->path().string();
				process_directory(dirIt->path().string(), newMap, combinedFVs, 
						combinedPreview, instanceHashes);
			}
		}

		{
			// perform temporal db initialziation
			TemporalDB_ptr db = make_shared<TemporalDB>(outputDBPath);
		}
		{
			// reopen to insert the data
			SqliteDB_ptr db = make_shared<SqliteDB>(outputDBPath, false);
			newMap.write(db);
		}
	} else {
		SessionLogger::err("no_directory") << "Specified directory "
			<< "does not exist";
		exit(1);
	}

#ifdef NDEBUG
	} catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		return -1;
	}
	catch (std::exception &e) {
		std::cerr << "Uncaught Exception: " << e.what() << std::endl;
		return -1;
	}
#endif
}

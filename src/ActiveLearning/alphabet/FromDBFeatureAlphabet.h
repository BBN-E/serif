#ifndef _FROM_DB_FEATURE_ALPHABET_H_
#define _FROM_DB_FEATURE_ALPHABET_H_

#include "FeatureAlphabet.h"
#include "Generic/database/DatabaseConnection.h"

BSP_DECLARE(FromDBFeatureAlphabet)
BSP_DECLARE(SqliteDBConnection)

class FromDBFeatureAlphabet : public FeatureAlphabet {
public:
	virtual int size() const;

	virtual double firstFeatureWeightByName(const std::wstring& feature_name) const;
	virtual int firstFeatureIndexByName(const std::wstring& feature_name) const;
	virtual double featureWeightByIndex(int idx) const;

	virtual std::vector<AnnotatedFeatureRecord_ptr> annotations() const;
	virtual void recordAnnotations(const std::vector<AnnotatedFeatureRecord_ptr>& annotations);

	virtual Eigen::VectorXd getWeights() const;

	virtual DatabaseConnection::Table_ptr getFeatureRows(double threshold, bool include_negatives, 
			const std::wstring& constraints = L"") const;

	virtual std::vector<int> featureIndicesOfClass(
			const std::wstring& feature_class_name, bool use_metadata_field=false) const;
	
	virtual void setFeatureWeight(int idx, double weight);
	// If you have many features to update,
	// this is much faster than calling setFeatureWeight for each one
	virtual void setFeatureWeights(const Eigen::VectorXd& feature_weights);
	virtual void featureRegularizationWeights(Eigen::VectorXd& regWeights,
			Eigen::VectorXd& negRegWeights) const;
	virtual std::pair<Eigen::VectorXd, Eigen::VectorXd>
		featureRegularizationWeights() const;
	virtual std::wstring getFeatureName(int idx) const;
    virtual ~FromDBFeatureAlphabet() {};

protected:
	FromDBFeatureAlphabet(DatabaseConnection_ptr db, unsigned int alphabetIndex);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(FromDBFeatureAlphabet, DatabaseConnection_ptr, unsigned int);
	mutable DatabaseConnection_ptr _db;
	unsigned int _alphIdx;
	//friend class LearnItDB;
};

#endif

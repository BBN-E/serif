#ifndef _FEATURE_ALPHABET_H_
#define _FEATURE_ALPHABET_H_

#include <string>
#include <vector>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "LearnIt/Eigen/Core"
#include "Generic/database/DatabaseConnection.h"

BSP_DECLARE(AnnotatedFeatureRecord)
BSP_DECLARE(FeatureAlphabet)

class FeatureAlphabet {
public:
	virtual int size() const = 0;

	virtual double firstFeatureWeightByName(const std::wstring& feature_name) const = 0;
	virtual int firstFeatureIndexByName(const std::wstring& feature_name) const = 0;
	virtual double featureWeightByIndex(int idx) const = 0;

	virtual std::vector<AnnotatedFeatureRecord_ptr> annotations() const = 0;
	virtual void recordAnnotations(const std::vector<AnnotatedFeatureRecord_ptr>& annotations) = 0;

	virtual Eigen::VectorXd getWeights() const = 0;

	virtual std::vector<int> featureIndicesOfClass(
			const std::wstring& feature_class_name, bool use_metadata_field = false) const = 0;
	
	virtual void setFeatureWeight(int idx, double weight) = 0;
	// If you have many features to update,
	// this is much faster than calling setFeatureWeight for each one
	virtual void setFeatureWeights(const Eigen::VectorXd& feature_weights) = 0;
	virtual void featureRegularizationWeights(Eigen::VectorXd& regWeights,
			Eigen::VectorXd& negRegWeights) const = 0;
	virtual std::pair<Eigen::VectorXd, Eigen::VectorXd> featureRegularizationWeights() const = 0;
	virtual std::wstring getFeatureName(int idx) const = 0;
    virtual ~FeatureAlphabet() {};

	virtual DatabaseConnection::Table_ptr getFeatureRows(double threshold, bool include_negatives, 
		const std::wstring& constraints = L"") const = 0;

};

#endif

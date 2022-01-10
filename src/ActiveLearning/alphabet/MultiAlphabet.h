#ifndef _MULTI_ALPHABET_H_
#define _MULTI_ALPHABET_H_

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "FeatureAlphabet.h"
#include "Generic/database/DatabaseConnection.h"

BSP_DECLARE(MultiAlphabet)

class MultiAlphabet : public FeatureAlphabet {
public:
	static MultiAlphabet_ptr create(FeatureAlphabet_ptr first, FeatureAlphabet_ptr second);
	static MultiAlphabet_ptr create(std::vector<FeatureAlphabet_ptr> alphabets);

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
	void featureRegularizationWeights(Eigen::VectorXd& regWeights, 
			Eigen::VectorXd& negRegWeights) const;
	virtual std::pair<Eigen::VectorXd, Eigen::VectorXd> featureRegularizationWeights() const;
	virtual std::wstring getFeatureName(int idx) const;

	unsigned int indexFromAlphabet(unsigned int idx, unsigned int alphabet) const;

    virtual ~MultiAlphabet() {};

	int dbNumForIdx(unsigned int idx) const;
private:
	MultiAlphabet(const std::vector<FeatureAlphabet_ptr>& alphabets);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(MultiAlphabet, const std::vector<FeatureAlphabet_ptr>);

	FeatureAlphabet_ptr dbForIdx(int idx);
	const FeatureAlphabet_ptr dbForIdx(int idx) const;
	int trans(unsigned int idx) const;

	std::vector<FeatureAlphabet_ptr> _alphabets;
	std::vector<unsigned int> _offsets;
};

#endif

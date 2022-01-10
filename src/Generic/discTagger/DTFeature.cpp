// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include <time.h>

#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/BlockFeatureTable.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/scoped_ptr.hpp>

void DTFeature::writeWeights(DTFeature::FeatureWeightMap &weightMap,
							UTF8OutputStream& out,
							bool write_zero_weights)
{
	for (FeatureWeightMap::iterator iter = weightMap.begin();
		iter != weightMap.end(); ++iter)
	{
		DTFeature *feature = (*iter).first;

		if (*(*iter).second != 0 || write_zero_weights) {
			out << L"((" << feature->getFeatureType()->getName().to_string()
				<< L" ";
			feature->write(out);
			out << L") " << *(*iter).second << L")\n";
		}
	}
}

void DTFeature::writeSumWeights(DTFeature::FeatureWeightMap &weightMap,
							UTF8OutputStream& out,
							bool write_zero_weights)
{
	for (FeatureWeightMap::iterator iter = weightMap.begin();
		iter != weightMap.end(); ++iter)
	{
		DTFeature *feature = (*iter).first;
		if ((*iter).second.getSum() != 0 || write_zero_weights) {
			out << L"((" << feature->getFeatureType()->getName().to_string()
				<< L" ";
			feature->write(out);
			out << L") " << (*iter).second.getSum() << L")\n";
		}
	}
}

void DTFeature::writeLazySumWeights(DTFeature::FeatureWeightMap &weightMap,
							UTF8OutputStream& out, long n_hypotheses,
							bool write_zero_weights)
{
	for (FeatureWeightMap::iterator iter = weightMap.begin();
		iter != weightMap.end(); ++iter)
	{
		DTFeature *feature = (*iter).first;
		double weight = (*iter).second.getLazySum(n_hypotheses);
		if (weight != 0 || write_zero_weights) {
			out << L"((" << feature->getFeatureType()->getName().to_string()
				<< L" ";
			feature->write(out);
			out << L") " << weight << L")\n";
		}
	}
}

void DTFeature::recordFileListForReference(Symbol key, UTF8OutputStream& out) {

	Symbol file_list = ParamReader::getParam(key);
	if (!file_list.is_null()) {
		out << key << L" " << file_list.to_string() << L"\n";		
		boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(file_list.to_string()));
		UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
		UTF8Token token;
		int num_files = 0;
		while (!tempFileListStream.eof()) {
			tempFileListStream >> token;
			if (wcscmp(token.chars(), L"") == 0)
				break;
			out << key << L" " << num_files << L" " << token.symValue() << "\n";
			num_files++;
		}
		tempFileListStream.close();
	} else {
		out << key << L" NOT-SPECIFIED\n";		
	}
}


void DTFeature::recordParamForReference(Symbol key, UTF8OutputStream& out) {
	Symbol sym = ParamReader::getParam(key);
	out << key.to_string() << L" ";
	if (sym.is_null())
		out << "NOT-SPECIFIED";
	else out << sym.to_string();
	out << L"\n";
}

void DTFeature::recordParamForConsistency(Symbol key, UTF8OutputStream& out) {
	out << L"* ";
	recordParamForReference(key, out);
}

void DTFeature::recordParamForReference(const char *key, UTF8OutputStream& out) {
	std::string buffer = ParamReader::getParam(key);
	
	out << key << " ";
	if (!buffer.empty())
		out << buffer;
	else 
		out << "NOT-SPECIFIED";

	out << L"\n";
}

void DTFeature::recordParamForConsistency(const char *key, UTF8OutputStream& out) {
	out << L"* ";
	recordParamForReference(key, out);
}

/* write the time and date into the stream
 */
void DTFeature::recordDate(UTF8OutputStream& out) {
	time_t currentTime;
	time( &currentTime );
	wchar_t tmpbuf[256];
	wcsftime( tmpbuf, 256,
		L"This model was created on %B %d, %Y at %H:%M.\n", localtime( &currentTime ));
	out << tmpbuf << L"\n";
}

void DTFeature::readWeights(FeatureWeightMap &weightMap,
							const char *model_file, Symbol modelprefix)
{
	Symbol rParen = Symbol(L")");
	Symbol lParen = Symbol(L"(");
	Symbol asterisk = Symbol(L"*");
	int nweights = 0;
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8Token token;

	in.open(model_file);
	if (in.fail()) {
		std::stringstream errMsg;
		errMsg << "Could not open perceptron weights file: " << model_file;
		throw UnexpectedInputException("DTFeature::readWeights()", errMsg.str().c_str());
	}
	while (!in.eof()) {
		in >> token; // (
		if (in.eof()) break;

		// skip over everything until the first paren
		// check any starred parameters!
		if (token.symValue() != lParen) {
			while (!in.eof()) {
				//cerr<<"in skip params:"<<token.symValue().to_debug_string()<<endl;
				if (token.symValue() == asterisk) {
					in >> token;
					Symbol paramName = token.symValue();
					in >> token;
					Symbol paramValue = token.symValue();
					checkParam(model_file, paramName, paramValue);
				}
				in >> token;
				if (token.symValue() == lParen) {
					break;
				}
			}
		} 

		if (in.eof()) break;
			
		in >> token; // (
		in >> token; // feature type
		DTFeatureType *type = DTFeatureType::getFeatureType(modelprefix, token.symValue());
		if (type == 0) {
			std::string message;
			message = "Discriminative model refers to unregistered feature: ";
			message += token.symValue().to_debug_string();
			throw UnexpectedInputException("DTFeature::readWeights()",
				message.c_str());
		}

		// the feature type knows which DTFeature subclass to instantiate
		DTFeature *feature = type->makeEmptyFeature();
		feature->read(in);
		in >> token; // )

		PWeight weight;
		char weight_str[101];
		//in >> *weight;	//this causes Serif to crash with exponents (5.97554e-311), 
		in >> token;	//don't call .symValue() on this, its a double and will quickly overflow symbols
		wcstombs(weight_str, token.chars(), 100); 
		weight_str[100] = '\0';		// make sure it's null terminated (not guaranteed by wcstombs)    
		*weight = atof(weight_str); // convert to chars first because _wtof is not ANSI compatible (boost::lexical_cast would also work)
		weightMap[feature] = weight;
		nweights++;
		
		in >> token; // )
		// sanity check:
		if (token.symValue() != rParen) {
			std::string message;
			message = "Reading perceptron model from:\n";
			message += model_file;
			message += "\nExpected ')' but got: `";
			message += token.symValue().to_debug_string();
			message += "'";
			throw UnexpectedInputException("DTFeature::readWeights()",
				message.c_str());
		}
	}

	in.close();

	if (nweights == 0) {
		char message[1000];
		_snprintf(message,
			sizeof(message)/sizeof(message[0]),
			"Reading perceptron model %s:\n  No weights found.\n",
			model_file);
		SessionLogger::warn("no_weights") << message;
	}
}

void DTFeature::readWeights(BlockFeatureTable &weightTable,
							const char *model_file, Symbol modelprefix)
{
	Symbol rParen = Symbol(L")");
	Symbol lParen = Symbol(L"(");
	Symbol asterisk = Symbol(L"*");
	int nweights = 0;
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8Token token;

	in.open(model_file);
	if (in.fail()) {
		std::stringstream errMsg;
		errMsg << "Could not open perceptron weights file: " << model_file;
		throw UnexpectedInputException("DTFeature::readWeights()", errMsg.str().c_str());
	}
	while (!in.eof()) {
		in >> token; // (
		if (in.eof()) break;

		// skip over everything until the first paren
		// check any starred parameters!
		if (token.symValue() != lParen) {
			while (!in.eof()) {
				//cerr<<"in skip params:"<<token.symValue().to_debug_string()<<endl;
				if (token.symValue() == asterisk) {
					in >> token;
					Symbol paramName = token.symValue();
					in >> token;
					Symbol paramValue = token.symValue();
					checkParam(model_file, paramName, paramValue);
				}
				in >> token;
				if (token.symValue() == lParen) {
					break;
				}
			}
		} 

		if (in.eof()) break;

		in >> token; // (
		in >> token; // feature type
		DTFeatureType *type = DTFeatureType::getFeatureType(modelprefix, token.symValue());
		if (type == 0) {
			std::string message;
			message = "Discriminative model refers to unregistered feature: ";
			message += token.symValue().to_debug_string();
			throw UnexpectedInputException("DTFeature::readWeights()",
				message.c_str());
		}

		// the feature type knows which DTFeature subclass to instantiate
		DTFeature *feature = type->makeEmptyFeature();
		feature->read(in);
		in >> token; // )

		char weight_str[101];
		//in >> *weight;	//this causes Serif to crash with exponents (5.97554e-311), 
		in >> token;	//don't call .symValue() on this, its a double and will quickly overflow symbols
		wcstombs(weight_str, token.chars(), 100); 
		weight_str[100] = '\0';		// make sure it's null terminated (not guaranteed by wcstombs)    
		double weight = atof(weight_str); // convert to chars first because _wtof is not ANSI compatible (boost::lexical_cast would also work)
		if (!weightTable.insert(feature, weight)) {
			feature->deallocate();
		}
		else {
			nweights++;
		}

		in >> token; // )
		// sanity check:
		if (token.symValue() != rParen) {
			std::string message;
			message = "Reading perceptron model from:\n";
			message += model_file;
			message += "\nExpected ')' but got: `";
			message += token.symValue().to_debug_string();
			message += "'";

			throw UnexpectedInputException("DTFeature::readWeights()",
				message.c_str());
		}
	}

	in.close();

	if (nweights == 0) {
		char message[1000];
		_snprintf(message,
			sizeof(message)/sizeof(message[0]),
			"Reading perceptron model %s:\n  No weights found.\n",
			model_file);
		SessionLogger::warn("no_weights") << message;
	}
}

void DTFeature::checkParam(const char *model_file, Symbol paramName, Symbol paramValue) {
	std::wstring paramNameStr(paramName.to_string());
	std::wstring paramValueStr(paramValue.to_string());

	// Old model files use dashes rather than underscores; so replace any 
	// dashes we see from the recorded model parameters with underscores 
	// before checking to see if parameters match.
	boost::algorithm::replace_all(paramNameStr, L"-", L"_");

	paramName = Symbol(paramNameStr);

	Symbol currentParamValue = ParamReader::getParam(paramName);

	if (currentParamValue.is_null()) {
		if (paramValueStr == L"NOT-SPECIFIED")
			return;

		// Sometimes we train models of the 'maxent' or 'p1' variety using params that have those prefixes,
		//   but at decode time we just use the base param. Deal with that here.
		if (paramNameStr.find(L"maxent_") == 0) {
			paramNameStr = paramNameStr.substr(7,std::wstring::npos);
			currentParamValue = ParamReader::getParam(paramNameStr);
		} else if (paramNameStr.find(L"p1_") == 0) {
			paramNameStr = paramNameStr.substr(3,std::wstring::npos);
			currentParamValue = ParamReader::getParam(paramNameStr);
		}

		if (currentParamValue.is_null()) {
			std::stringstream msg;
			msg << "Possible training/decode parameter mismatch for " << model_file << " / " << paramName.to_debug_string() << ":\n" 
				<< "  training: " << paramValue.to_debug_string() << "\n"
				<< "  decode: NONE\n";
			SessionLogger::warn("train_decode_mismatch") << msg.str();
			return;
		}
	}


	std::string trainStr = paramValue.to_debug_string();
	std::string decodeStr = currentParamValue.to_debug_string();

	int last_forward_slash = (int) trainStr.find_last_of("/");
	int last_backward_slash = (int) trainStr.find_last_of("\\");
	int last_slash = last_forward_slash;
	if (last_backward_slash > last_slash)
		last_slash = last_backward_slash;
	if (last_slash > 0 && last_slash + 1 < (int) trainStr.length())
		trainStr = trainStr.substr(last_slash + 1);


	last_forward_slash = (int) decodeStr.find_last_of("/");
	last_backward_slash = (int) decodeStr.find_last_of("\\");
	last_slash = last_forward_slash;
	if (last_backward_slash > last_slash)
		last_slash = last_backward_slash;
	if (last_slash > 0 && last_slash + 1 < (int) decodeStr.length())
		decodeStr = decodeStr.substr(last_slash + 1);

	if (trainStr.compare(decodeStr) != 0) {
		
		std::stringstream msg;
		msg << "Possible training/decode parameter mismatch for " << model_file << " / " << paramName.to_debug_string() << ":\n" 
			<< "  training: " << paramValue.to_debug_string() << "\n"
			<< "  decode: " << currentParamValue.to_debug_string() << "\n";
		SessionLogger::warn("train_decode_mismatch") << msg.str();
	}

}

void DTFeature::addWeightsToSum(FeatureWeightMap *weights){
	for (DTFeature::FeatureWeightMap::iterator iter = weights->begin();
			iter != weights->end(); ++iter)
	{
		(*iter).second.addToSum();
	}
}

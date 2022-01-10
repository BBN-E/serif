// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/parse/ParserTrainer/K_Estimator.h"
#include <cstddef>
#include <cstring>
#include <string> 
#include <math.h> 
#include <fcntl.h>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include <boost/scoped_ptr.hpp>

K_Estimator::K_Estimator(char *head_counts_file, char *pre_counts_file, char *post_counts_file, 
						 char *left_counts_file, char *right_counts_file,
						 char *headProbFile, char *premodProbFile, char *postmodProbFile, 
						 char *leftLexicalProbFile, char *rightLexicalProbFile, 
						 char *outfile,
						 int mhc,
						 double inc, double min, double max)
{

	_increment = static_cast<float>(inc);
	_min = static_cast<float>(min);
	_max = static_cast<float>(max);
	_outfile = outfile;

	// just creates instance, initializes nothing 
	_headProbs = _new HeadProbs();
    _premodProbs = _new ModifierProbs();
    _postmodProbs = _new ModifierProbs();
    _leftLexicalProbs = _new LexicalProbs();
    _rightLexicalProbs = _new LexicalProbs();
	
	// smoothing counts
	_input_filenames = _new string[5];
	_input_filenames[HEAD] = head_counts_file;
	_input_filenames[LEFT] = left_counts_file;
	_input_filenames[RIGHT] = right_counts_file;
	_input_filenames[PRE] = pre_counts_file;
	_input_filenames[POST] = post_counts_file;

	// main corpus counts put into Deriver classes
    boost::scoped_ptr<UTF8InputStream> headProbStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& headProbStream(*headProbStream_scoped_ptr);
    headProbStream.open(headProbFile);
    _headDeriver = _new HeadProbDeriver(headProbStream);
    headProbStream.close();
	
    boost::scoped_ptr<UTF8InputStream> leftLexicalProbStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& leftLexicalProbStream(*leftLexicalProbStream_scoped_ptr);
    leftLexicalProbStream.open(leftLexicalProbFile);
    _leftLexicalDeriver = _new LexicalProbDeriver(leftLexicalProbStream, mhc);
    leftLexicalProbStream.close();
	
    boost::scoped_ptr<UTF8InputStream> rightLexicalProbStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& rightLexicalProbStream(*rightLexicalProbStream_scoped_ptr);
    rightLexicalProbStream.open(rightLexicalProbFile);
    _rightLexicalDeriver = _new LexicalProbDeriver(rightLexicalProbStream, mhc);
    rightLexicalProbStream.close();
	
    boost::scoped_ptr<UTF8InputStream> premodProbStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& premodProbStream(*premodProbStream_scoped_ptr);
    premodProbStream.open(premodProbFile);
    _premodDeriver = _new ModifierProbDeriver(premodProbStream);
    premodProbStream.close();
	
    boost::scoped_ptr<UTF8InputStream> postmodProbStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& postmodProbStream(*postmodProbStream_scoped_ptr);
    postmodProbStream.open(postmodProbFile);
	_postmodDeriver = _new ModifierProbDeriver(postmodProbStream);
    postmodProbStream.close();


}

K_Estimator::~K_Estimator() {
	delete _headProbs;
    delete _premodProbs;
    delete _postmodProbs;
    delete _leftLexicalProbs;
    delete _rightLexicalProbs;
	delete _input_filenames;
	delete _headDeriver;
	delete _leftLexicalDeriver;
	delete _rightLexicalDeriver;
	delete _premodDeriver;
	delete _postmodDeriver;
}



void K_Estimator::estimate() {

	double likelihood = 0;
	double best_likelihood = -10000000;
	NgramScoreTable::Table::iterator iter;

	UTF8OutputStream out;
	out.open(_outfile);

	for (int model_type = HEAD; model_type <= RIGHT; model_type++) {

		int number_of_multipliers;
		
		// fill in smoothing counts
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(_input_filenames[model_type].c_str());
		
		if (model_type == HEAD) {
			_current_table = _new NgramScoreTable(4, in);
		} else _current_table = _new NgramScoreTable(7, in);

		in.close();


		if (model_type == HEAD) {
			number_of_multipliers = 2;
			float *best_mult = _new float[number_of_multipliers];
	
			_headDeriver->pre_estimation();
			_headProbs->set_table_access(_headDeriver);

			best_likelihood = -10000000;
			for (float i = _min; i <= _max; i += _increment) {
				for (float j = _min; j <= _max; j += _increment) {
					likelihood = 0;
					_headDeriver->set_unique_multiplier_pwt(i);
					_headDeriver->set_unique_multiplier_pt(j);
					_headDeriver->derive_tables_for_estimation();
					for (iter = _current_table->get_start(); iter != _current_table->get_end(); ++iter) {
						float prob = _headProbs->lookup((*iter).first[0], (*iter).first[1], 
														(*iter).first[2], (*iter).first[3]);
						if (prob != 0)
							likelihood += log(prob);
						if (likelihood < best_likelihood)
							break;
					}
					//cerr << i << " " << j << " " << likelihood << "\n";
					//cerr.flush();
					if (likelihood > best_likelihood) {
						best_mult[0] = i;
						best_mult[1] = j;
						best_likelihood = likelihood;
					}
				}
			}

			out << best_mult[0] << " " << best_mult[1] << "\n";
			out.flush();

		}
		
		else if (model_type == PRE || model_type == POST) {
			ModifierProbDeriver *mpd;
			ModifierProbs *mp;
			if (model_type == PRE) {
				mpd = _premodDeriver;
				mp = _premodProbs;
			} else {
				mpd = _postmodDeriver;
				mp = _postmodProbs;
			}
			
			mpd->pre_estimation();
			mp->set_table_access(mpd);
			number_of_multipliers = 2;
			float *best_mult = _new float[number_of_multipliers];

			best_likelihood = -10000000;
			for (float i = _min; i <= _max; i += _increment) {
				for (float j = _min; j <= _max; j += _increment) {
					likelihood = 0;
					mpd->set_unique_multiplier_PHpwt(i);
					mpd->set_unique_multiplier_PHpt(j);
					mpd->derive_tables_for_estimation();
					for (iter = _current_table->get_start(); iter != _current_table->get_end(); ++iter) {
						float prob = mp->lookup((*iter).first[0], (*iter).first[1], 
												 (*iter).first[2], (*iter).first[3],
												 (*iter).first[4], (*iter).first[5], 
												 (*iter).first[6]);
						if (prob != 0)
							likelihood += log(prob);
						if (likelihood < best_likelihood)
							break;
					}
					//cerr << i << " " << j << " " << likelihood << "\n";
					//cerr.flush();
					if (likelihood > best_likelihood) {
						best_mult[0] = i;
						best_mult[1] = j;
						best_likelihood = likelihood;
					}
				}
			}
			out << best_mult[0] << " " << best_mult[1] << "\n";
			out.flush();
			
		}

		else if (model_type == LEFT || model_type == RIGHT) {
			LexicalProbDeriver *lpd;
			LexicalProbs *lp;
			LexicalProbs *alt;
			if (model_type == LEFT) {
				lpd = _leftLexicalDeriver;
				lp = _leftLexicalProbs;
				alt = _rightLexicalProbs; 
				_rightLexicalDeriver->pre_estimation();
				alt->set_table_access(_rightLexicalDeriver);
				lpd->pre_estimation();
				lp->set_table_access(lpd);
			} else {
				lpd = _rightLexicalDeriver;
				lp = _rightLexicalProbs;
				alt = _leftLexicalProbs;
			}

			
			number_of_multipliers = 3;
			float *best_mult = _new float[number_of_multipliers];
			best_likelihood = -10000000;
			for (float i = _min; i <= _max; i += _increment) {
				for (float j = _min; j <= _max; j += _increment) {
					for (float k = _min; k <= _max; k += _increment) {
						likelihood = 0;
						lpd->set_unique_multiplier_MtPHwt(i);
						lpd->set_unique_multiplier_MtPHt(j);
						lpd->set_unique_multiplier_Mt(k);
						lpd->derive_tables_for_estimation();
						for (iter = _current_table->get_start(); iter != _current_table->get_end(); ++iter) {
							float prob = lp->lookup(alt, (*iter).first[0], (*iter).first[1], 
								(*iter).first[2], (*iter).first[3],
								(*iter).first[4], (*iter).first[5], 
								(*iter).first[6]);
							if (prob != 0)
								likelihood += log(prob);
							if (likelihood < best_likelihood)
								break;
						}
						//cerr << i << " " << j << " " << k << " " << likelihood << "\n";
						//cerr.flush();
						if (likelihood > best_likelihood) {
							best_mult[0] = i;
							best_mult[1] = j;
							best_mult[2] = k;
							best_likelihood = likelihood;
						}
					}
				}
			}

			out << best_mult[0] << " " << best_mult[1] << " " << best_mult[2] << "\n";
			out.flush();
			
			
		}


	}

	out.close();

}

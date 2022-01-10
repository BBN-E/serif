// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef GENERIC_HYPOTHESIS_H
#define GENERIC_HYPOTHESIS_H

#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/PGFactDate.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/shared_ptr.hpp"
#include <vector>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(PGFactArgument);
BSP_DECLARE(GenericHypothesis);

/*! \brief GenericHypothesis serves as the base class for the various kinds of
    hypothesis implementations.  It cannot be directly instantiated, but it 
    does provide some very basic default implementations of the common 
    hypothesis functions.
*/
class GenericHypothesis
{
public:

	typedef struct {
		std::string role;
		int actor_id;
		std::wstring value;
	} kb_arg_t;

	GenericHypothesis();
	virtual ~GenericHypothesis(void) {}

    /*! \brief Determines whether the given hypothesis is equivalent to this 
        hypothesis, such that the two could/should be merged, and in fact
		the new hypothesis should subsume the old one. The exact 
        definition of equivalence depends on the subclass of GenericHypothesis
        being used.

        \param hypoth The new hypothesis to compare to. It should be the same
        subclass of GenericHypothesis as the current object.
        \return true if they are equivalent and the new one is better, false otherwise
	*/
	virtual bool isEquivAndBetter(GenericHypothesis_ptr hypoth) { return false; }

    /*! \brief Determines whether the given hypothesis is equivalent to this 
        hypothesis, such that the two could/should be merged. The exact 
        definition of equivalence depends on the subclass of GenericHypothesis
        being used.

        \param hypoth The other hypothesis to compare to. It should be the same
        subclass of GenericHypothesis as the current object.
        \return true if they are equivalent, false otherwise
	*/
	virtual bool isEquiv(GenericHypothesis_ptr hypoth) = 0;

	/*! \brief Determines whether the given hypothesis is similar to this 
        hypothesis, such that the two should probably not both be output.
		The exact definition of equivalence depends on the subclass of 
		GenericHypothesis being used.

        \param hypoth The other hypothesis to compare to. It should be the same
        subclass of GenericHypothesis as the current object.
        \return true if they are similar, false otherwise
	*/
	virtual bool isSimilar(GenericHypothesis_ptr hypoth) { return isEquiv(hypoth); }

    /*! \brief Adds a hypothesis to this hypothesis. By default,
	    it just takes the facts from one and adds them here.

        \param hypo The hypothesis to be added
    */
	virtual void addSupportingHypothesis(GenericHypothesis_ptr hypo);

    /*! \brief Returns the string representation of the assertion of this 
        hypothesis (which will be uploaded to the profile database and 
        eventually displayed to the user as the "answer" to a particular slot).

        \return value of this hypothesis, as a string
    */
	virtual std::wstring getDisplayValue() = 0;	
	
    /*! \brief Returns the arguments to be uploaded to the KB
    */
	virtual std::vector<kb_arg_t> getKBArguments(int actor_id, ProfileSlot_ptr slot);

	/*! \brief Returns true if hypothesis should not be displayed for some reason.
    */
	virtual bool isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale) { return false; }

	/*! \brief Returns true if hypothesis is risky. 
    */
	virtual bool isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale) { return false; }

    /*! \brief Compares two hypotheses and decides which is better for output.

        \return Returns -1 if this is lower-ranking than hypo. Returns 1 if this
	    is higher-ranking than hypo. Returns 0 if their rankings are equivalent.	           
    */
	enum {BETTER, SAME, WORSE}; // to improve code readability
	virtual int rankAgainst(GenericHypothesis_ptr hypo);
	virtual void computeScore();

	/*! \brief Calls child-class-specific add function for each fact.
    */
	virtual void addHypothesisSpecificFact(PGFact_ptr fact) {}

	/*! \brief Returns whether want to check if hold and non-hold dates
	    are inconsistent */
	virtual bool excludeQuestionableHoldDates() { return false; }

	// NON VIRTUAL FUNCTIONS BELOW HERE

	/*! \brief Add a fact to the supporting fact list, and keep running
	    variables up to date. 
    */
	void addFact(PGFact_ptr fact);

	/*! \brief Set confidence.
    */
	void setConfidence(double confidence) { _confidence = confidence; }

	/*! \brief iterate over all the fact dates, and find a consistent
	    list of hold, non-hold, start and end dates to store on the
		hypothesis
	*/
	void generateHypothesisDates();
	void clearHoldDates();
	void clearNonHoldDates();

	// Accessors
	const std::vector<PGFact_ptr>& getSupportingFacts() { return _supportingFacts; }
	double getBestFactScore() { return _best_fact_score; }
	int getBestScoreGroup() { return _best_score_group; }
	int getBestDoublyReliableScoreGroup() { return _best_doubly_reliable_score_group; }
	double getOldestFactID() { return _oldest_fact_ID; }
	double getNewestFactID() { return _newest_fact_ID; }
	int nSupportingFacts() { return static_cast<int>(_supportingFacts.size()); }
	int nReliableFacts() { return _n_reliable_facts; }
	int nEnglishFacts() { return _n_english_facts; }
	int nReliableEnglishTextFacts() { return _n_reliable_english_text_facts; }
	int nDoublyReliableFacts() { return _n_doubly_reliable_facts; }
	int nDoublyReliableEnglishTextFacts() { return _n_doubly_reliable_english_text_facts; }
	bool hasExternalHighConfidenceFact() { return _has_external_high_confidence_fact; }
	boost::gregorian::date& getOldestCaptureTime() { return _oldestCaptureTime; }
	boost::gregorian::date& getNewestCaptureTime() { return _newestCaptureTime; }
	PGFactDate_ptr getStartDate();
	PGFactDate_ptr getEndDate();
	double getConfidence() { return _confidence; }
	std::vector<PGFactDate_ptr>& getHoldDates();
	std::vector<PGFactDate_ptr>& getActivityDates();
	std::vector<PGFactDate_ptr>& getNonHoldDates();
	std::vector<PGFactDate_ptr> getAllDates();
	std::vector<PGFactDate_ptr> getAllNonHoldDates();

private:

    /*! \brief A collection of fact objects which are considered supporting 
        evidence for this hypothesis
    */
	std::vector<PGFact_ptr> _supportingFacts;
	
    /*! \brief The best fact score we've seen. Kept up to date by the use of addFact().
    */
	double _best_fact_score;

	/*! \brief The lowest score group we've seen. Kept up to date by the use of addFact().
    */
	int _best_score_group;
	int _best_doubly_reliable_score_group;
	
	/*! \brief The oldest and newest fact entry IDs we've seen. Kept up to date by the use of addFact().
	    Used to keep sorting deterministic and repeatable.
    */
	double _oldest_fact_ID;
	double _newest_fact_ID;

	/*! \brief The earliest/latest date we've seen. Kept up to date by the use of addFact().
    */
	boost::gregorian::date _oldestCaptureTime;
	boost::gregorian::date _newestCaptureTime;
	
	/*! \brief Counters for English-ness and AGENT1-reliability.
    */
	int _n_reliable_english_text_facts;
	int _n_reliable_facts;
	int _n_english_facts;
	int _n_doubly_reliable_facts;
	int _n_doubly_reliable_english_text_facts;

	/*! \brief True if there is a DBPedia fact, for example
    */
	bool _has_external_high_confidence_fact;
	
	/*! \brief Placeholder for score, should we want to use one
    */
	double _score_cache;

	/*! \brief Set by ProfileConfidence late in the game
    */
	double _confidence;

	/*! \brief Date info -- A consistent set of dates taken from facts.
	*/
	const static float DATE_THRESHOLD;
	PGFactDate_ptr _startDate;
	PGFactDate_ptr _endDate;
	std::vector<PGFactDate_ptr> _holdDates;
	std::vector<PGFactDate_ptr> _nonHoldDates;
	std::vector<PGFactDate_ptr> _activityDates;
	std::vector<PGFactDateSet_ptr> breakIntoConsistentSets(std::vector<PGFactDate_ptr> dates);
	PGFactDate_ptr getBestDate(std::vector<PGFactDateSet_ptr>, int num_dates);
	void clearDates();
	void storeIfEarliestOrLatest(PGFactDate_ptr date, PGFactDate_ptr &earliest, PGFactDate_ptr &latest);
	
	bool isFactFromNewDocument(PGFact_ptr newFact);
};

#endif

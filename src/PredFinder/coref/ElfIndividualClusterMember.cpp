/**
* An ElfIndividual and the associated evidence we need
* to perform coreference clustering.
*
* @file ElfIndividualClusterMember.cpp
* @author nward@bbn.com
* @date 2010.08.23
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "boost/foreach.hpp"
#include "ElfIndividualClusterMember.h"
#include "PredFinder/elf/ElfIndividual.h"
#include "PredFinder/elf/ElfRelation.h"

/**
 * Construct a cluster member from its individual and
 * relevant relation subgraph. Determine the ontology
 * domain from the individual's type.
 *
 * @param individual The individual we're clustering.
 * @param relations A dictionary of relevant relation
 * evidence keyed by relation name.
 *
 * @author nward@bbn.com
 * @date 2010.08.24
 **/
ElfIndividualClusterMember::ElfIndividualClusterMember(const std::wstring & domain_prefix, const ElfIndividual_ptr individual, 
													   const ElfRelationMap & relations) {
	// Copy the members
	_individual = individual;
	_relations = relations;
	_domain_prefix = domain_prefix;

	if (is_nfl()) {
		// Check if the singleton overlapping role list needs to be initialized
		if (overlapping_relation_roles.empty())
			init_nfl_overlapping_relation_roles();

		// Check if the singleton must equal role list needs to be initialized
		if (must_equal_relation_roles.empty())
			init_nfl_must_equal_relation_roles();

		// Check if the singleton must not equal role list needs to be initialized
		if (must_not_equal_relation_roles.empty())
			init_nfl_must_not_equal_relation_roles();

		// Check if the singleton conditional equality role list needs to be initialized
		if (must_equal_if_equal_relation_roles.empty())
			init_nfl_must_equal_if_equal_relation_roles();
	}

	// Accumulate a unique set (by equality) of the member relations' arguments
	_n_unique_relation_args = 0;
	BOOST_FOREACH(ElfRelationMap::value_type relations_by_name, _relations) {
		BOOST_FOREACH(ElfRelation_ptr relation, relations_by_name.second) {
			BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
				if (arg->get_individual().get() != NULL && arg->get_individual()->get_best_uri() == _individual->get_best_uri())
					// Ignore args that contain this individual
					continue;

				// Make sure we haven't seen this arg (or an equivalent) before for this individual
				ElfRelationRole relation_role(relations_by_name.first, arg->get_role());
				bool seen = false;
				BOOST_FOREACH(ElfRelationRoleMap::value_type seen_relation_role, _unique_relation_args) {
					if (do_relation_roles_overlap(relation_role, seen_relation_role.first)) {
						BOOST_FOREACH(ElfRelationArg_ptr seen_arg, seen_relation_role.second) {
							if (arg->offsetless_equals(seen_arg, false)) {
								seen = true;
								break;
							}
						}
					}
				}
				if (!seen) {
					std::pair<ElfRelationRoleMap::iterator, bool> relation_role_insert = _unique_relation_args.insert(ElfRelationRoleMap::value_type(relation_role, std::vector<ElfRelationArg_ptr>()));
					relation_role_insert.first->second.push_back(arg);
					_n_unique_relation_args++;
				}
			}
		}
	}
}

/**
 * Calculate a similarity score between two cluster members
 * using a weighted average of various hard-coded features.
 * The weights are each [0.0,1.0], and the score from each
 * feature is also [0.0,1.0], so this is just a linear combination.
 *
 * This is the meat of the coreference process.
 *
 * @param other The cluster to score against
 * @return A similarity score between members [0.0,1.0]
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
double ElfIndividualClusterMember::calculate_similarity(const ElfIndividualClusterMember_ptr other) const {
	// First check if we're just comparing identical individuals for some reason
	if (_individual->get_best_uri() == other->_individual->get_best_uri())
		return 1.0;

	// Score is computed additively as we examine each feature
	double weighted_similarity = 0.0;

	// Check if any conditional relation role constraints would be violated
	//   This generally assumes that the constraints haven't already been violated,
	//   resulting in 
	if (ParamReader::isParamTrue("use_conditional_cluster_equality")) {
		// Collect any applicable consequents of an equality condition
		std::set<ElfRelationRole> consequent_set;
		BOOST_FOREACH(ElfRelationMap::value_type relations, _relations) {
			ElfRelationMap::iterator other_relations_i = other->_relations.find(relations.first);
			if (other_relations_i != other->_relations.end()) {
				BOOST_FOREACH(ElfRelation_ptr relation, relations.second) {
					BOOST_FOREACH(ElfRelation_ptr other_relation, other_relations_i->second) {
						// See if there is an equality constraint on these relations, or based on these relations
						std::set<ElfRelationRole> relation_consequent_set = get_consequents_if_relations_equal(relation, other_relation);

						// Check each constraint consequent to see if it's violated by this relation pair
						BOOST_FOREACH(ElfRelationRole consequent, relation_consequent_set) {
							// Only check constraints relevant to this relation pair
							if (consequent.first != relations.first) {
								consequent_set.insert(consequent);
								continue;
							}

							// Make sure these consequent roles are equal in this relation pair, if present
							bool consequent_checked = false;
							BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args_with_role(consequent.second)) {
								BOOST_FOREACH(ElfRelationArg_ptr other_arg, other_relation->get_args_with_role(consequent.second)) {
									if (arg->offsetless_equals(other_arg)) {
										consequent_checked = true;
										break;
									} else {
										// 
										return -1.0;
									}
								}
							}

							// Don't recheck the constraint if we already passed it
							if (!consequent_checked)
								consequent_set.insert(consequent);
						}
					}
				}
			}
		}

		// Check all relations against the list of consequents we need to check, if any
		bool consequent_equals = false;
		BOOST_FOREACH(ElfRelationRole consequent, consequent_set) {
			ElfRelationMap::const_iterator relations_i = _relations.find(consequent.first);
			ElfRelationMap::const_iterator other_relations_i = other->_relations.find(consequent.first);
			if (relations_i != _relations.end() && other_relations_i != other->_relations.end()) {
				BOOST_FOREACH(ElfRelation_ptr relation, relations_i->second) {
					BOOST_FOREACH(ElfRelation_ptr other_relation, other_relations_i->second) {
						BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args_with_role(consequent.second)) {
							BOOST_FOREACH(ElfRelationArg_ptr other_arg, other_relation->get_args_with_role(consequent.second)) {
								if (arg->offsetless_equals(other_arg)) {
									consequent_equals = true;
									break;
								} else {
									// Condition was equal, but consequent wasn't
									return -1.0;
								}
							}
							if (consequent_equals)
								break;
						}
						if (consequent_equals)
							break;
					}
					if (consequent_equals)
						break;
				}
			}
		}
	}

	// Determine how many relation arguments match between this member and the other member
	// and calculate an "f-score"
	static const double relation_feature_weight = 1.0;
	int found = 0;
	int other_found = 0;
	std::set<ElfRelationArg_ptr> seen_args;
	//SessionLogger::info("LEARNIT") << _individual->get_id() << " ?= " << other->_individual->get_id() << std::endl;
	BOOST_FOREACH(ElfRelationRoleMap::value_type args_by_relation, _unique_relation_args) {
		BOOST_FOREACH(ElfRelationRoleMap::value_type other_args_by_relation, other->_unique_relation_args) {
			// Check if these relation roles can count as equal
			//SessionLogger::info("LEARNIT") << args_by_relation.first.first << " " << args_by_relation.first.second 
				//<< " ?= " << other_args_by_relation.first.first << " " << other_args_by_relation.first.second << std::endl;
			if (!do_relation_roles_overlap(args_by_relation.first, other_args_by_relation.first)) {
				continue;
			}

			// Get equality constraints for these relation roles
			bool should_equal = should_relation_roles_be_equal(args_by_relation.first, other_args_by_relation.first);
			bool should_not_equal = should_relation_roles_be_not_equal(args_by_relation.first, other_args_by_relation.first);
			//SessionLogger::info("LEARNIT") << "  constraints: == " << should_equal << ", != " << should_not_equal << std::endl;

			// Track which of these overlapping relation args actually match by value/individual
			//SessionLogger::info("LEARNIT") << " l: " << args_by_relation.second.size() << " (" << found << ") " 
				//<< r: " << other_args_by_relation.second.size() << " (" << other_found << ")";
			BOOST_FOREACH(ElfRelationArg_ptr arg, args_by_relation.second) {
				// Check if we've seen this argument in this cluster member already, so we don't score multiple times
				bool seen = false;
				if (seen_args.find(arg) != seen_args.end())
					seen = true;

				BOOST_FOREACH(ElfRelationArg_ptr other_arg, other_args_by_relation.second) {
					// Check if we've seen this argument in the other cluster member already, so we don't score multiple times
					bool seen_other = false;
					if (seen_args.find(other_arg) != seen_args.end())
						seen_other = true;

					// Check if these two arguments overlap (ignoring offsets) and enforce equality constraints
					if (arg->offsetless_equals(other_arg, false)) {
						// Enforce inequality constraints
						if (should_not_equal) {
							// Overlapping roles that must be not equal are, cluster members can't match
							//SessionLogger::info("LEARNIT") << "Roles should have been inequal!" << std::endl;
							return -1.0;
						}

						// If neither of this pair has been seen before, increment the score counts
						if (!seen && !seen_other) {
							found++;
							other_found++;
						}

						// Keep track of what we've seen
						if (!seen) {
							seen_args.insert(arg);
							seen = true;
						}
						if (!seen_other)
							seen_args.insert(other_arg);
					} else if (should_equal) {
						// Overlapping roles that must be equal aren't, cluster members can't match
						//SessionLogger::info("LEARNIT") << "Roles should have been equal!" << std::endl;
						return -1.0;
					}
				}
			}
			//SessionLogger::info("LEARNIT") << " -> " << found << ", " << other_found << std::endl;
		}
	}
	double relation_recall = 0.0;
	if (_n_unique_relation_args > 0)
		relation_recall = ((double) found)/_n_unique_relation_args;
	double relation_precision = 0.0;
	if (other->_n_unique_relation_args > 0)
		relation_precision = ((double) other_found)/other->_n_unique_relation_args;
	double relation_feature = 0.0;
	if (relation_recall > 0 && relation_precision > 0)
		relation_feature = 2*relation_recall*relation_precision/(relation_recall + relation_precision);
	weighted_similarity += relation_feature_weight*relation_feature;
	//SessionLogger::info("LEARNIT") << _individual->get_id() << " ?= " << other->_individual->get_id() << ": " << weighted_similarity << std::endl;

	// Done
	return weighted_similarity;
}

/**
 * Stores relation/role pairs in sets that are allowed to overlap.
 * Initialized once with the first cluster member.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
std::vector<std::set<std::pair<std::wstring, std::wstring> > > ElfIndividualClusterMember::overlapping_relation_roles;

/**
 * Loads the static overlapping_relation_roles table
 * using hard-coded logic based on the macros. Gross.
 *
 * Conflicts are given precedence, so there may be some
 * conflicting relation roles loaded here (so we can compare
 * across non-conflicting relations).
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
void ElfIndividualClusterMember::init_nfl_overlapping_relation_roles(void) {
	// nfl:NFLTeam
	const std::wstring relations_with_loser_role[] = {
		L"eru:NFLGameLoserDate",
		L"eru:gameLoserDate",
		L"eru:NFLLoserDate",
		L"eru:NFLGameLoserDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLGameWinnerOrLoser",
	};
	const std::wstring relations_with_winner_role[] = {
		L"eru:NFLGameWinnerDate",
		L"eru:gameWinnerDate",
		L"eru:NFLWinnerDate",
		L"eru:NFLGameWinnerDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLGameWinnerOrLoser",
	};
	const std::wstring relations_with_team_role[] = {
		L"eru:pointsScoredForPlayerInGame",
		L"eru:touchdownCompleteCountForPlayerInGame",
		L"eru:fieldGoalCompleteCountForPlayerInGame",
		L"eru:onePointConversionCompleteCountForPlayerInGame",
		L"eru:twoPointConversionCompleteCountForPlayerInGame",
		L"eru:safetyCompleteCountForPlayerInGame",
		L"eru:touchdownPartialCountForPlayerInGame",
		L"eru:fieldGoalPartialCountForPlayerInGame",
		L"eru:onePointConversionPartialCountForPlayerInGame",
		L"eru:twoPointConversionPartialCountForPlayerInGame",
		L"eru:safetyPartialCountForPlayerInGame",
		L"eru:pointsScoredForTeamInGame",
		L"eru:touchdownCompleteCountForTeamInGame",
		L"eru:fieldGoalCompleteCountForTeamInGame",
		L"eru:onePointConversionCompleteCountForTeamInGame",
		L"eru:twoPointConversionCompleteCountForTeamInGame",
		L"eru:safetyCompleteCountForTeamInGame",
		L"eru:touchdownPartialCountForTeamInGame",
		L"eru:fieldGoalPartialCountForTeamInGame",
		L"eru:onePointConversionPartialCountForTeamInGame",
		L"eru:twoPointConversionPartialCountForTeamInGame",
		L"eru:safetyPartialCountForTeamInGame",
		L"eru:teamInGame",
		L"eru:homeTeamInGame",
		L"eru:awayTeamInGame",
		L"eru:gameLoser",
		L"eru:gameWinner",
		L"eru:teamScoringAll",
		L"eru:scoreSummary",
	};
	const std::wstring relations_with_two_team_roles[] = {
		L"eru:NFLGameTeamsDate",
		L"eru:NFLGameTeamsDate_opt",
	};
	std::set<ElfRelationRole> overlapping_team_roles;
	size_t i;
	for (i = 0; i < sizeof(relations_with_loser_role)/sizeof(std::wstring); i++) {
		overlapping_team_roles.insert(ElfRelationRole(std::wstring(relations_with_loser_role[i]), std::wstring(L"eru:gameLoser")));
	}
	for (i = 0; i < sizeof(relations_with_winner_role)/sizeof(std::wstring); i++) {
		overlapping_team_roles.insert(ElfRelationRole(std::wstring(relations_with_winner_role[i]), std::wstring(L"eru:gameWinner")));
	}
	for (i = 0; i < sizeof(relations_with_team_role)/sizeof(std::wstring); i++) {
		overlapping_team_roles.insert(ElfRelationRole(std::wstring(relations_with_team_role[i]), std::wstring(L"eru:NFLTeam")));
	}
	for (i = 0; i < sizeof(relations_with_two_team_roles)/sizeof(std::wstring); i++) {
		overlapping_team_roles.insert(ElfRelationRole(std::wstring(relations_with_two_team_roles[i]), std::wstring(L"eru:teamInGame1")));
		overlapping_team_roles.insert(ElfRelationRole(std::wstring(relations_with_two_team_roles[i]), std::wstring(L"eru:teamInGame2")));
	}
	overlapping_team_roles.insert(ElfRelationRole(std::wstring(L"eru:NFLScoringTeam"), std::wstring(L"eru:teamScoring")));
	overlapping_relation_roles.push_back(overlapping_team_roles);

	// eru:NFLPlayer
	const std::wstring relations_with_player_role[] = {
		L"eru:pointsScoredForPlayerInGame",
		L"eru:touchdownCompleteCountForPlayerInGame",
		L"eru:fieldGoalCompleteCountForPlayerInGame",
		L"eru:onePointConversionCompleteCountForPlayerInGame",
		L"eru:twoPointConversionCompleteCountForPlayerInGame",
		L"eru:safetyCompleteCountForPlayerInGame",
		L"eru:touchdownPartialCountForPlayerInGame",
		L"eru:fieldGoalPartialCountForPlayerInGame",
		L"eru:onePointConversionPartialCountForPlayerInGame",
		L"eru:twoPointConversionPartialCountForPlayerInGame",
		L"eru:safetyPartialCountForPlayerInGame",
	};
	std::set<ElfRelationRole> overlapping_player_roles;
	for (i = 0; i < sizeof(relations_with_player_role)/sizeof(std::wstring); i++) {
		overlapping_player_roles.insert(ElfRelationRole(std::wstring(relations_with_player_role[i]), std::wstring(L"eru:NFLPlayer")));
	}
	overlapping_relation_roles.push_back(overlapping_player_roles);

	// eru:gameDate
	const std::wstring relations_with_date_role[] = {
		L"eru:NFLGameLoserDate",
		L"eru:gameLoserDate",
		L"eru:NFLLoserDate",
		L"eru:NFLGameLoserDateScore",
		L"eru:NFLGameWinnerDate",
		L"eru:gameWinnerDate",
		L"eru:NFLWinnerDate",
		L"eru:NFLGameWinnerDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLScoringTeam",
		L"eru:gameDate",
	};
	std::set<ElfRelationRole> overlapping_date_roles;
	for (i = 0; i < sizeof(relations_with_date_role)/sizeof(std::wstring); i++) {
		overlapping_date_roles.insert(ElfRelationRole(std::wstring(relations_with_date_role[i]), std::wstring(L"eru:gameDate")));
	}
	overlapping_relation_roles.push_back(overlapping_date_roles);
}

/**
 * Checks the contents of overlapping_relation_roles to
 * see if the specified pair of relations and roles can
 * be considered to be equal even if their relation/role
 * don't match.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
bool ElfIndividualClusterMember::do_relation_roles_overlap(const ElfRelationRole & left_relation_role, 
														   const ElfRelationRole & right_relation_role) 
{
	// Equal trivially overlaps
	if (left_relation_role.first == right_relation_role.first && left_relation_role.second == right_relation_role.second)
		return true;

	// Iterate over each overlap set
	BOOST_FOREACH(std::set<ElfRelationRole> overlapping_roles, overlapping_relation_roles) {
		if (overlapping_roles.find(left_relation_role) != overlapping_roles.end() && overlapping_roles.find(right_relation_role) != overlapping_roles.end())
			return true;
	}

	// No match
	return false;
}

/**
 * Stores relation/role pairs that must be equal by relation/role pair.
 * Initialized once with the first cluster member.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
std::map<ElfRelationRole, std::set<ElfRelationRole> > ElfIndividualClusterMember::must_equal_relation_roles;

/**
 * Loads the static must_equal_relation_roles table
 * using hard-coded logic based on the macros. Gross.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
void ElfIndividualClusterMember::init_nfl_must_equal_relation_roles(void) {
	// eru:gameDate
	const std::wstring relations_with_date_role[] = {
		L"eru:NFLGameLoserDate",
		L"eru:gameLoserDate",
		L"eru:NFLLoserDate",
		L"eru:NFLGameLoserDateScore",
		L"eru:NFLGameWinnerDate",
		L"eru:gameWinnerDate",
		L"eru:NFLWinnerDate",
		L"eru:NFLGameWinnerDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLScoringTeam",
		L"eru:gameDate",
	};
	size_t i, j;
	for (i = 0; i < sizeof(relations_with_date_role)/sizeof(std::wstring); i++) {
		ElfRelationRole left_date(std::wstring(relations_with_date_role[i]), std::wstring(L"eru:gameDate"));
		for (j = 0; j < sizeof(relations_with_date_role)/sizeof(std::wstring); j++) {
			ElfRelationRole right_date(std::wstring(relations_with_date_role[j]), std::wstring(L"eru:gameDate"));
			std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> date_role_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(left_date, std::set<ElfRelationRole>()));
			date_role_insert.first->second.insert(right_date);
		}
	}

	// eru:NFLTeam in home contexts (there can be only one!)
	ElfRelationRole home_team_role(L"eru:homeTeamInGame", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> home_team_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(home_team_role, std::set<ElfRelationRole>()));
	home_team_insert.first->second.insert(home_team_role);

	// eru:NFLTeam in away contexts (there can be only one!)
	ElfRelationRole away_team_role(L"eru:awayTeamInGame", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> away_team_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(away_team_role, std::set<ElfRelationRole>()));
	away_team_insert.first->second.insert(away_team_role);

	// eru:gameWinner (there can be only one!)
	const std::wstring relations_with_winner_role[] = {
		L"eru:NFLGameWinnerDate",
		L"eru:gameWinnerDate",
		L"eru:NFLWinnerDate",
		L"eru:NFLGameWinnerDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLGameWinnerOrLoser",
	};
	ElfRelationRole winner_team_role(L"eru:gameWinner", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> winner_team_role_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(winner_team_role, std::set<ElfRelationRole>()));
	winner_team_role_insert.first->second.insert(winner_team_role);
	for (i = 0; i < sizeof(relations_with_winner_role)/sizeof(std::wstring); i++) {
		ElfRelationRole left_winner_role(std::wstring(relations_with_winner_role[i]), std::wstring(L"eru:gameWinner"));
		std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> winner_role_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(left_winner_role, std::set<ElfRelationRole>()));
		winner_role_insert.first->second.insert(winner_team_role);
		winner_team_role_insert.first->second.insert(left_winner_role);
		for (j = 0; j < sizeof(relations_with_winner_role)/sizeof(std::wstring); j++) {
			ElfRelationRole right_winner_role(std::wstring(relations_with_winner_role[j]), std::wstring(L"eru:gameWinner"));
			winner_role_insert.first->second.insert(right_winner_role);
		}
	}

	// eru:gameLoser (there can be only one!)
	const std::wstring relations_with_loser_role[] = {
		L"eru:NFLGameLoserDate",
		L"eru:gameLoserDate",
		L"eru:NFLLoserDate",
		L"eru:NFLGameLoserDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLGameWinnerOrLoser",
	};
	ElfRelationRole loser_team_role(L"eru:gameLoser", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> loser_team_role_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(loser_team_role, std::set<ElfRelationRole>()));
	loser_team_role_insert.first->second.insert(loser_team_role);
	for (i = 0; i < sizeof(relations_with_loser_role)/sizeof(std::wstring); i++) {
		ElfRelationRole left_loser_role(std::wstring(relations_with_loser_role[i]), std::wstring(L"eru:gameLoser"));
		std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> loser_role_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(left_loser_role, std::set<ElfRelationRole>()));
		loser_role_insert.first->second.insert(loser_team_role);
		loser_team_role_insert.first->second.insert(left_loser_role);
		for (j = 0; j < sizeof(relations_with_loser_role)/sizeof(std::wstring); j++) {
			ElfRelationRole right_loser_role(std::wstring(relations_with_loser_role[j]), std::wstring(L"eru:gameLoser"));
			loser_role_insert.first->second.insert(right_loser_role);
		}
	}

	// eru:NFLTeam in eru:scoreSummary contexts (there can be only one!)
	ElfRelationRole summary_team_role(L"eru:scoreSummary", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> summary_team_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(summary_team_role, std::set<ElfRelationRole>()));
	summary_team_insert.first->second.insert(summary_team_role);

	// eru:NFLTeam in eru:teamScoringAll contexts (there can be only one!)
	ElfRelationRole team_scoring_all(L"eru:teamScoringAll", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> team_scoring_all_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(team_scoring_all, std::set<ElfRelationRole>()));
	team_scoring_all_insert.first->second.insert(team_scoring_all);

	// Various partial counts for different scoring types (one per GTSS)
	const std::wstring partial_count_scoring_types[] = {
		L"touchdown",
		L"fieldGoal",
		L"onePointConversion",
		L"twoPointConversion",
		L"safety",
	};
	for (i = 0; i < sizeof(partial_count_scoring_types)/sizeof(std::wstring); i++) {
		std::wstring scoring_predicate = L"eru:" + partial_count_scoring_types[i] + L"PartialCount";
		std::wstringstream scoring_role;
        scoring_role << L"eru:team" << towupper(partial_count_scoring_types[i][0]) << partial_count_scoring_types[i].substr(1) << L"Count";
		ElfRelationRole partial_count_role(scoring_predicate, scoring_role.str());
		std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> partial_count_insert = must_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(partial_count_role, std::set<ElfRelationRole>()));
		partial_count_insert.first->second.insert(partial_count_role);
	}
}

/**
 * Checks the contents of must_equal_relation_roles to
 * see if the specified pair of relations and roles 
 * must have equal values.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
bool ElfIndividualClusterMember::should_relation_roles_be_equal(const ElfRelationRole & left_relation_role, 
																const ElfRelationRole & right_relation_role) 
{
	// Check for each role in the other's equality set
	std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator left_must_equal = must_equal_relation_roles.find(left_relation_role);
	if (left_must_equal != must_equal_relation_roles.end()) {
		if (left_must_equal->second.find(right_relation_role) != left_must_equal->second.end())
			return true;
	}
	std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator right_must_equal = must_equal_relation_roles.find(right_relation_role);
	if (right_must_equal != must_equal_relation_roles.end()) {
		if (right_must_equal->second.find(left_relation_role) != right_must_equal->second.end())
			return true;
	}

	// No equality constraint
	return false;
}

/**
 * Stores relation/role pairs that must NOT be equal by relation/role pair.
 * Initialized once with the first cluster member.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
std::map<ElfRelationRole, std::set<ElfRelationRole> > ElfIndividualClusterMember::must_not_equal_relation_roles;

/**
 * Loads the static must_not_equal_relation_roles table
 * using hard-coded logic based on the macros. Gross.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
void ElfIndividualClusterMember::init_nfl_must_not_equal_relation_roles(void) {
	// eru:gameLoser/eru:gameWinner
	const std::wstring relations_with_loser_role[] = {
		L"eru:NFLGameLoserDate",
		L"eru:gameLoserDate",
		L"eru:NFLLoserDate",
		L"eru:NFLGameLoserDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLGameWinnerOrLoser",
	};
	const std::wstring relations_with_winner_role[] = {
		L"eru:NFLGameWinnerDate",
		L"eru:gameWinnerDate",
		L"eru:NFLWinnerDate",
		L"eru:NFLGameWinnerDateScore",
		L"eru:NFLGameNameOrWinnerOrLoserDate",
		L"eru:NFLGameWinnerLoserDate",
		L"eru:NFLGameWinnerOrLoserDate",
		L"eru:NFLGameWinnerOrLoser",
	};
	size_t i, j;
	ElfRelationRole winner_team_role(L"eru:gameWinner", L"eru:NFLTeam");
	ElfRelationRole loser_team_role(L"eru:gameLoser", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> winner_team_role_insert = must_not_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(winner_team_role, std::set<ElfRelationRole>()));
	winner_team_role_insert.first->second.insert(loser_team_role);
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> loser_team_role_insert = must_not_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(loser_team_role, std::set<ElfRelationRole>()));
	loser_team_role_insert.first->second.insert(winner_team_role);
	for (i = 0; i < sizeof(relations_with_loser_role)/sizeof(std::wstring); i++) {
		ElfRelationRole loser_role(std::wstring(relations_with_loser_role[i]), std::wstring(L"eru:gameLoser"));
		std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> loser_role_insert = must_not_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(loser_role, std::set<ElfRelationRole>()));
		loser_role_insert.first->second.insert(winner_team_role);
		winner_team_role_insert.first->second.insert(loser_role);
		for (j = 0; j < sizeof(relations_with_winner_role)/sizeof(std::wstring); j++) {
			ElfRelationRole winner_role(std::wstring(relations_with_winner_role[j]), std::wstring(L"eru:gameWinner"));
			loser_role_insert.first->second.insert(winner_role);
			loser_team_role_insert.first->second.insert(winner_role);
			std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> winner_role_insert = must_not_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(winner_role, std::set<ElfRelationRole>()));
			winner_role_insert.first->second.insert(loser_role);
			winner_role_insert.first->second.insert(loser_team_role);
		}
	}

	// eru:NFLTeam in home/away contexts (bidirectionally exclusive)
	ElfRelationRole home_team_role(L"eru:homeTeamInGame", L"eru:NFLTeam");
	ElfRelationRole away_team_role(L"eru:awayTeamInGame", L"eru:NFLTeam");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> home_team_insert = must_not_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(home_team_role, std::set<ElfRelationRole>()));
	home_team_insert.first->second.insert(away_team_role);
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> away_team_insert = must_not_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(away_team_role, std::set<ElfRelationRole>()));
	away_team_insert.first->second.insert(home_team_role);
}

/**
 * Checks the contents of must_not_equal_relation_roles to
 * see if the specified pair of relations and roles 
 * must have inequal values.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
bool ElfIndividualClusterMember::should_relation_roles_be_not_equal(const ElfRelationRole & left_relation_role, 
																	const ElfRelationRole & right_relation_role) {
	// Check for the each role in the other's inequality set
	std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator left_must_not_equal = must_not_equal_relation_roles.find(left_relation_role);
	if (left_must_not_equal != must_not_equal_relation_roles.end()) {
		if (left_must_not_equal->second.find(right_relation_role) != left_must_not_equal->second.end())
			return true;
	}
	std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator right_must_not_equal = must_not_equal_relation_roles.find(right_relation_role);
	if (right_must_not_equal != must_not_equal_relation_roles.end()) {
		if (right_must_not_equal->second.find(left_relation_role) != right_must_not_equal->second.end())
			return true;
	}

	// No inequality constraint
	return false;
}

/**
 * Stores relation/role pairs that must be conditionally equal
 * by relation/role pair (that is, if the first is equal, then
 * the second must be equal).
 * Initialized once with the first cluster member.
 *
 * @author nward@bbn.com
 * @date 2011.01.10
 **/
std::map<ElfRelationRole, std::set<ElfRelationRole> > ElfIndividualClusterMember::must_equal_if_equal_relation_roles;

/**
 * Loads the static must_equal_if_equal_relation_roles table
 * using hard-coded logic based on the macros. Gross.
 *
 * @author nward@bbn.com
 * @date 2011.01.10
 **/
void ElfIndividualClusterMember::init_nfl_must_equal_if_equal_relation_roles(void) {
	// if eru:NFLTeam in eru:pointsScoredForTeamInGame for a given nfl:NFLGame is equal, then eru:teamPointsScored in the same eru:pointsScoredForTeamInGame should be equal
	ElfRelationRole points_scored_team_role(L"eru:pointsScoredForTeamInGame", L"eru:NFLTeam");
	ElfRelationRole points_scored_points_role(L"eru:pointsScoredForTeamInGame", L"eru:teamPointsScored");
	std::pair<std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator, bool> points_scored_insert = must_equal_if_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(points_scored_team_role, std::set<ElfRelationRole>()));
	points_scored_insert.first->second.insert(points_scored_points_role);

	// if eru:NFLTeam in eru:teamScoringAll for a given nfl:NFLGameTeamSummaryScore is equal, then eru:teamPointsScored in eru:pointsScored for the same nfl:NFLGameTeamSummaryScore should be equal
	ElfRelationRole scoring_all_team_role(L"eru:teamScoringAll", L"eru:NFLTeam");
	ElfRelationRole score_summary_team_role(L"eru:scoreSummary", L"eru:NFLTeam");
	ElfRelationRole scored_points_role(L"eru:pointsScored", L"eru:teamPointsScored");
	points_scored_insert = must_equal_if_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(scoring_all_team_role, std::set<ElfRelationRole>()));
	points_scored_insert.first->second.insert(scored_points_role);
	points_scored_insert = must_equal_if_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(score_summary_team_role, std::set<ElfRelationRole>()));
	points_scored_insert.first->second.insert(scored_points_role);

	// Also reverse the constraint, if the points are equal than the team must be; this may block tie games, but will block a lot of extraneous clusters
	points_scored_insert = must_equal_if_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(points_scored_points_role, std::set<ElfRelationRole>()));
	points_scored_insert.first->second.insert(points_scored_team_role);
	points_scored_insert = must_equal_if_equal_relation_roles.insert(std::pair<ElfRelationRole, std::set<ElfRelationRole> >(scored_points_role, std::set<ElfRelationRole>()));
	points_scored_insert.first->second.insert(scoring_all_team_role);
	points_scored_insert.first->second.insert(score_summary_team_role);
}

/**
 * Checks the contents of must_equal_if_equal_relation_roles to
 * see if the specified pair of relations have equal roles that
 * are the condition of a constraint.
 *
 * @author nward@bbn.com
 * @date 2011.01.10
 **/
std::set<ElfRelationRole> ElfIndividualClusterMember::get_consequents_if_relations_equal(const ElfRelation_ptr left_relation, 
																						 const ElfRelation_ptr right_relation) 
{
	// Ignore relations that aren't the same predicate
	if (left_relation->get_name() != right_relation->get_name())
		return std::set<ElfRelationRole>();

	// Check if these relations share a role that is the condition of an equality
	BOOST_FOREACH(ElfRelationArg_ptr left_arg, left_relation->get_args()) {
		ElfRelationRole left_relation_role(left_relation->get_name(), left_arg->get_role());
		std::map<ElfRelationRole, std::set<ElfRelationRole> >::iterator left_must_equal_i = must_equal_if_equal_relation_roles.find(left_relation_role);
		if (left_must_equal_i != must_equal_if_equal_relation_roles.end()) {
			BOOST_FOREACH(ElfRelationArg_ptr right_arg, right_relation->get_args_with_role(left_arg->get_role())) {
				if (left_arg->offsetless_equals(right_arg))
					return left_must_equal_i->second;
			}
		}
	}

	// No equality constraint, or the constraint wasn't met
	return std::set<ElfRelationRole>();
}

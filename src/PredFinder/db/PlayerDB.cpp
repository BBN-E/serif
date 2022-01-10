/**
 * Functionality for reading NFL world knowledge.
 *
 * @file PlayerDB.cpp
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/sqlite/SqliteDB.h"
#include "PlayerDB.h"
#include "boost/foreach.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string/replace.hpp"
#include <iostream>

/**
 * Read the sqlite database from the specified
 * location into cached tables.
 *
 * @param db_location Path to the sqlite .db file. If string is empty, db will not be loaded.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
PlayerDB::PlayerDB(const std::string db_location) {
	// Ignore unspecified parameter
	if (db_location == "")
		return;

	// Open the database
	SessionLogger::info("LEARNIT") << "Reading NFL player database " << db_location << "..." << std::endl;
	SqliteDB db(db_location);

	// Populate the player lookup table
	Table_ptr player_roster = db.exec(
		L"SELECT roster.season, team.ontology_uri, player.name, player.uri "
		L"FROM roster, team, player "
		L"WHERE roster.player = player.id AND roster.team = team.id AND team.ontology_uri IS NOT NULL"
		);
	BOOST_FOREACH(std::vector<std::wstring> roster_row, *player_roster) {
		// Get the values from the database
		int season = boost::lexical_cast<int>(roster_row[0]);
		std::wstring team_uri = roster_row[1];
		std::wstring player_name = roster_row[2];
		std::wstring player_uri = boost::replace_first_copy(roster_row[3], L"http://www.footballdb.com/players/", L"fdb:");

		// Make sure this season has an entry
		std::pair<SeasonMap::iterator, bool> season_insert = _players_by_team_and_season.insert(SeasonMap::value_type(season, StringVectorMap()));

		// Make sure this team has an entry
		std::pair<StringVectorMap::iterator, bool> team_insert = season_insert.first->second.insert(StringVectorMap::value_type(team_uri, std::vector<std::pair<std::wstring,std::wstring> >()));

		// Append this player
		team_insert.first->second.push_back(std::pair<std::wstring, std::wstring>(player_name, player_uri));
	}
	SessionLogger::info("LEARNIT") << "  Found " << player_roster->size() << " roster entries over " << _players_by_team_and_season.size() << " seasons" << std::endl;
}

/**
 * Finds all of the teams this player could be a member of,
 * optionally restricted by season. Multiple returns are allowed,
 * since some players trade mid-season, and some players share the same name.
 *
 * @param player The string name of the player to check for.
 * @param season The optional NFL season year to restrict by.
 * @return A map from player URIs to a set of team URIs; for most situations,
 * with a unique player name and season restriction, this will be one pair.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
PlayerResultMap PlayerDB::get_teams_for_player(const std::wstring & player, int season) const {
	// Result map
	PlayerResultMap result;

	// Check if a season was specified
	if (season != -1) {
		// Get the season's team map
		SeasonMap::const_iterator season_i = _players_by_team_and_season.find(season);
		if (season_i != _players_by_team_and_season.end()) {
			// Loop through this season's team rosters looking for this player
			BOOST_FOREACH(StringVectorMap::value_type team_roster, season_i->second) {
				// Loop through this team roster looking for this player
				for (std::vector<std::pair<std::wstring, std::wstring> >::iterator roster_i = team_roster.second.begin(); roster_i != team_roster.second.end(); roster_i++) {
					// Check if this is a matching player
					if (roster_i->first == player) {
						// Associate this player URI and team URI
						std::pair<PlayerResultMap::iterator, bool> team_insert = result.insert(PlayerResultMap::value_type(roster_i->second, std::set<std::wstring>()));
						team_insert.first->second.insert(team_roster.first);
					}
				}
			}
		}
	} else {
		BOOST_FOREACH(SeasonMap::value_type season_teams, _players_by_team_and_season) {
			// Loop through this season's team rosters looking for this player
			BOOST_FOREACH(StringVectorMap::value_type team_roster, season_teams.second) {
				// Loop through this team roster looking for this player
				for (std::vector<std::pair<std::wstring, std::wstring> >::iterator roster_i = team_roster.second.begin(); roster_i != team_roster.second.end(); roster_i++) {
					// Check if this is a matching player
					if (roster_i->first == player) {
						// Associate this player URI and team URI
						std::pair<PlayerResultMap::iterator, bool> team_insert = result.insert(PlayerResultMap::value_type(roster_i->second, std::set<std::wstring>()));
						team_insert.first->second.insert(team_roster.first);
					}
				}
			}
		}
	}

	// Done
	return result;
}

/**
 * Finds all of the unique player URIs that could match the
 * specified player name assuming that play for a particular team.
 * Multiple returns are allowed since some players share the
 * same name, even on the same team in the same season, although
 * it's unlikely.
 *
 * @param player The string name of the player to check for.
 * @param team The optional NFL team URI to restrict by.
 * @param season The optional NFL season year to restrict by.
 * @return A vector of player URIs; for most situations, its
 * length will be 1.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
std::vector<std::wstring> PlayerDB::get_uris_for_player(const std::wstring & player, const std::wstring & team, int season) const {
	// Result vector
	std::vector<std::wstring> player_uris;

	// Find all URI/team combinations for this player name
	PlayerResultMap player_team_results = get_teams_for_player(player, season);

	// If a team was specified, restrict the URI search to that
	if (team != L"") {
		BOOST_FOREACH(PlayerResultMap::value_type player_team, player_team_results) {
			// See if this player played for this team
			std::set<std::wstring>::iterator team_i = player_team.second.find(team);
			if (team_i != player_team.second.end()) {
				player_uris.push_back(player_team.first);
			}
		}
	} else {
		// Just use all of the URIs found
		BOOST_FOREACH(PlayerResultMap::value_type player_team, player_team_results) {
			player_uris.push_back(player_team.first);
		}
	}

	// Done
	return player_uris;
}

/**
 * Checks if a player played for a particular team at some point.
 *
 * @param player The string name of the player to check for.
 * @param team The NFL team URI to check for.
 * @param season The optional NFL season year to restrict by.
 * @return True if the player played for that team ever, or in
 * the specified season.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
bool PlayerDB::is_player_on_team(const std::wstring & player, const std::wstring & team, int season) const {
	// Just check for emptiness of the URI table
	return get_uris_for_player(player, team, season).size() != 0;
}

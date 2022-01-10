/**
 * Functionality for reading NFL world knowledge.
 *
 * @file PlayerDB.h
 * @author nward@bbn.com
 * @date 2010.08.17
 **/

#pragma once

#include "boost/shared_ptr.hpp"
#include <map>
#include <set>
#include <vector>
/**
 * Stores a list of strings by string. Intended for use
 * as a dictionary of player names organized by team URI.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
typedef std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring> > > StringVectorMap;

/**
 * Defines a hierarchy of teams by season.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
typedef std::map<int, StringVectorMap> SeasonMap;

/**
 * Defines a collection of player/team search results.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
typedef std::map<std::wstring, std::set<std::wstring> > PlayerResultMap;

/**
 * Wrapper class that reads a sqlite database containing
 * NFL world knowledge and exposes it with lookup tables
 * and application-specific convenience methods.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
class PlayerDB {
public:
	PlayerDB(const std::string db_location);

	PlayerResultMap get_teams_for_player(const std::wstring & player, int season = -1) const;
	std::vector<std::wstring> get_uris_for_player(const std::wstring & player, const std::wstring & team = L"", int season = -1) const;
	bool is_player_on_team(const std::wstring & player, const std::wstring & team, int season = -1) const;

private:
	/**
	 * Stores all of the player names in the database
	 * by team URI and by season.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.17
	 **/
	SeasonMap _players_by_team_and_season;
};


/**
 * Shared pointer for use in containers.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
typedef boost::shared_ptr<PlayerDB> PlayerDB_ptr;

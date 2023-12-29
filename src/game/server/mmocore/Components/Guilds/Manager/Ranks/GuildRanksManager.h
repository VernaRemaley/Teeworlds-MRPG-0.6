/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_GUILD_RANKS_MANAGER_H
#define GAME_SERVER_COMPONENT_GUILD_RANKS_MANAGER_H

#include "GuildRankData.h"

// Forward declaration and alias
class CGS;
class CGuildData;
using GuildRankContainer = std::deque<class CGuildRankData*>;

// This class is a controller for managing guild ranks
class CGuildRanksController
{
	// Pointer to the game server
	CGS* GS() const;

	GuildRankContainer m_aRanks;
	CGuildData* m_pGuild;

public:
	// Constructor 
	CGuildRanksController() = delete;
	CGuildRanksController(CGuildData* pGuild);

	// Destructor
	~CGuildRanksController();

	// Function to get the container of guild ranks
	GuildRankContainer& GetContainer() { return m_aRanks; }

	// Function to add a new guild rank
	bool Add(std::string Rank);

	// Function to remove an existing guild rank
	bool Remove(std::string Rank);

	// Function to get a guild rank by its name
	CGuildRankData* Get(std::string Rank) const;

private:
	// Function to initialize the guild ranks
	void Init();
};


#endif
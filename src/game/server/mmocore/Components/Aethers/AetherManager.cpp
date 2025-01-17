/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "AetherManager.h"

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

#include <game/server/mmocore/Components/Guilds/GuildManager.h>

void CAetherManager::OnInit()
{
	const auto InitAethers = Database->Prepare<DB::SELECT>("*", "tw_aethers");
	InitAethers->AtExecute([this](ResultPtr pRes)
	{
		while (pRes->next())
		{
			vec2 Pos = vec2(pRes->getInt("TeleX"), pRes->getInt("TeleY"));
			int WorldID = pRes->getInt("WorldID");

			AetherIdentifier ID = pRes->getInt("ID");
			CAether(ID).Init(pRes->getString("Name").c_str(), Pos, WorldID);
		}

		Job()->ShowLoadingProgress("Aethers", CAether::Data().size());
	});
}

void CAetherManager::OnInitAccount(CPlayer *pPlayer)
{
	ResultPtr pRes = Database->Execute<DB::SELECT>("*", "tw_accounts_aethers", "WHERE UserID = '%d'", pPlayer->Account()->GetID());
	while(pRes->next())
	{
		const int TeleportID = pRes->getInt("AetherID");
		pPlayer->Account()->m_aAetherLocation[TeleportID] = true;
	}
}

bool CAetherManager::OnHandleVoteCommands(CPlayer *pPlayer, const char *CMD, const int VoteID, const int VoteID2, int Get, const char *GetText)
{
	const int ClientID = pPlayer->GetCID();

	// teleport
	if(PPSTR(CMD, "TELEPORT") == 0)
	{
		AetherIdentifier AetherID = VoteID;
		const int Price = VoteID2;
		if(Price > 0 && !pPlayer->Account()->SpendCurrency(Price))
			return true;

		CAether* pAether = &CAether::Data()[AetherID];
		vec2 Position = pAether->GetPosition();
		if(!GS()->IsPlayerEqualWorld(ClientID, pAether->GetWorldID()))
		{
			pPlayer->GetTempData().SetTeleportPosition(Position);
			pPlayer->ChangeWorld(pAether->GetWorldID());
			return true;
		}

		pPlayer->GetCharacter()->ChangePosition(Position);
		GS()->UpdateVotes(ClientID, pPlayer->m_CurrentVoteMenu);
		return true;
	}

	return false;
}

bool CAetherManager::OnHandleTile(CCharacter* pChr, int IndexCollision)
{
	CPlayer* pPlayer = pChr->GetPlayer();
	const int ClientID = pPlayer->GetCID();

	if (pChr->GetHelper()->TileEnter(IndexCollision, TILE_AETHER_TELEPORT))
	{
		_DEF_TILE_ENTER_ZONE_SEND_MSG_INFO(pPlayer);
		UnlockLocation(pChr->GetPlayer(), pChr->m_Core.m_Pos);
		GS()->StrongUpdateVotes(ClientID, pPlayer->m_CurrentVoteMenu);
		return true;
	}
	else if (pChr->GetHelper()->TileExit(IndexCollision, TILE_AETHER_TELEPORT))
	{
		_DEF_TILE_EXIT_ZONE_SEND_MSG_INFO(pPlayer);
		GS()->StrongUpdateVotes(ClientID, pPlayer->m_CurrentVoteMenu);
		return true;
	}

	return false;
}

bool CAetherManager::OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu)
{
	if (ReplaceMenu)
	{
		CCharacter* pChr = pPlayer->GetCharacter();
		if (!pChr || !pChr->IsAlive())
			return false;

		if (pChr->GetHelper()->BoolIndex(TILE_AETHER_TELEPORT))
		{
			ShowList(pChr);
			return true;
		}
		return false;
	}

	return false;
}

void CAetherManager::UnlockLocation(CPlayer *pPlayer, vec2 Pos)
{
	const int ClientID = pPlayer->GetCID();
	for (const auto& [ID, Aether] : CAether::Data())
	{
		if (distance(Aether.GetPosition(), Pos) > 100 || pPlayer->Account()->m_aAetherLocation.find(ID) != pPlayer->Account()->m_aAetherLocation.end())
			continue;

		pPlayer->Account()->m_aAetherLocation[ID] = true;
		Database->Execute<DB::INSERT>("tw_accounts_aethers", "(UserID, AetherID) VALUES ('%d', '%d')", pPlayer->Account()->GetID(), ID);

		GS()->Chat(ClientID, "You now have Aethernet access to the {STR}.", Aether.GetName());
		GS()->ChatDiscord(DC_SERVER_INFO, Server()->ClientName(ClientID), "Now have Aethernet access to the {STR}.", Aether.GetName());
		return;
	}
}

void CAetherManager::ShowList(CCharacter* pChar) const
{
	CPlayer* pPlayer = pChar->GetPlayer();
	const int ClientID = pPlayer->GetCID();
	GS()->AddVoteItemValue(ClientID);
	GS()->AV(ClientID, "null");

	GS()->AVH(ClientID, TAB_AETHER, "Available aethers");
	if(pPlayer->Account()->HasGuild() && pPlayer->Account()->GetGuild()->HasHouse())
	{
		GS()->AVM(ClientID, "MSPAWN", NOPE, TAB_AETHER, "Move to Guild House - free");
	}

	if(pPlayer->Account()->HasHouse())
	{
		GS()->AVM(ClientID, "HOUSE_SPAWN", NOPE, TAB_AETHER, "Move to Your House - free");
	}

	for (const auto& [ID, Aether] : CAether::Data())
	{
		if (pPlayer->Account()->m_aAetherLocation.find(ID) == pPlayer->Account()->m_aAetherLocation.end())
			continue;

		const bool LocalTeleport = (GS()->IsPlayerEqualWorld(ClientID, Aether.GetWorldID()) && distance(pPlayer->GetCharacter()->m_Core.m_Pos, Aether.GetPosition()) < 120);
		if (LocalTeleport)
			continue;

		const int Price = g_Config.m_SvPriceTeleport * (Aether.GetWorldID() + 1);
		GS()->AVD(ClientID, "TELEPORT", ID, Price, TAB_AETHER, "{STR} : {STR} - {VAL}gold", Aether.GetName(), Server()->GetWorldName(Aether.GetWorldID()), Price);
	}

	GS()->AV(ClientID, "null");
}

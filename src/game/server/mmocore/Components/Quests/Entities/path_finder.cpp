/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/mmocore/Components/Bots/BotData.h>
#include "path_finder.h"

#include <game/server/mmocore/Components/Worlds/WorldManager.h>
#include <game/server/gamecontext.h>

CEntityPathFinder::CEntityPathFinder(CGameWorld* pGameWorld, vec2 Pos, int WorldID, int ClientID, bool* pComplete)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FINDQUEST, Pos)
{
	vec2 GetterPos{0,0};
	GS()->Mmo()->WorldSwap()->FindPosition(WorldID, Pos, &GetterPos);

	m_PosTo = GetterPos;
	m_ClientID = ClientID;
	m_WorldID = WorldID;
	m_pPlayer = GS()->GetPlayer(m_ClientID, true, true);
	m_pComplete = pComplete;

	GameWorld()->InsertEntity(this);
}

void CEntityPathFinder::Tick()
{
	if(!m_pPlayer || !m_pPlayer->GetCharacter() || !total_size_vec2(m_PosTo))
	{
		GS()->m_World.DestroyEntity(this);
		return;
	}
	
	if (!m_pComplete || (*m_pComplete) == true)
	{
		GS()->CreatePlayerSpawn(m_Pos, CmaskOne(m_ClientID));
		GS()->m_World.DestroyEntity(this);
	}
}

void CEntityPathFinder::Snap(int SnappingClient)
{
	if(m_ClientID != SnappingClient || !m_pPlayer || !m_pPlayer->GetCharacter())
		return;

	CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(pPickup)
	{
		vec2 CorePos = m_pPlayer->GetCharacter()->m_Core.m_Pos;
		m_Pos = CorePos;
		m_Pos -= normalize(CorePos - m_PosTo) * clamp(distance(m_Pos, m_PosTo), 32.0f, 90.0f);

		pPickup->m_X = (int)m_Pos.x;
		pPickup->m_Y = (int)m_Pos.y;
		pPickup->m_Type = POWERUP_ARMOR;
		pPickup->m_Subtype = 0;
	}
}
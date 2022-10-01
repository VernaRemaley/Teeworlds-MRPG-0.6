/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "SkillData.h"

#include <game/server/gamecontext.h>

#include "Entities/HealthTurret/healer-health.h"
#include "Entities/AttackTeleport/attack-teleport.h"
#include "Entities/SleepyGravity/sleepy-gravity.h"

std::map < int, std::map < int, CSkillData > > CSkillData::ms_aSkills;

void CSkillData::SetSkillOwner(CPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
	m_pGS = pPlayer->GS();
}

void CSkillData::SelectNextControlEmote()
{
	if(!m_pPlayer || !m_pPlayer->IsAuthed())
		return;

	m_SelectedEmoticion++;
	if(m_SelectedEmoticion >= NUM_EMOTICONS)
		m_SelectedEmoticion = -1;

	Sqlpool.Execute<DB::UPDATE>("tw_accounts_skills", "UsedByEmoticon = '%d' WHERE SkillID = '%d' AND UserID = '%d'", m_SelectedEmoticion, m_SkillID, m_pPlayer->Acc().m_UserID);
}

bool CSkillData::Use()
{
	if(!m_pPlayer || !m_pPlayer->IsAuthed() || !m_pPlayer->GetCharacter() || m_Level <= 0)
		return false;

	// mana check
	CCharacter* pChr = m_pPlayer->GetCharacter();
	const int PriceMana = translate_to_percent_rest(m_pPlayer->GetStartMana(), Info()->GetManaPercentageCost());
	if(PriceMana > 0 && pChr->CheckFailMana(PriceMana))
		return false;

	const vec2 PlayerPosition = pChr->GetPos();
	const int ClientID = m_pPlayer->GetCID();
	if(m_SkillID == Skill::SkillHeartTurret)
	{
		for(CHealthHealer* pHh = (CHealthHealer*)GS()->m_World.FindFirst(CGameWorld::ENTYPE_SKILLTURRETHEART); pHh; pHh = (CHealthHealer*)pHh->TypeNext())
		{
			if(pHh->m_pPlayer->GetCID() != ClientID)
				continue;

			pHh->Reset();
			break;
		}
		const int PowerLevel = PriceMana;
		new CHealthHealer(&GS()->m_World, m_pPlayer, GetBonus(), PowerLevel, PlayerPosition);
		return true;
	}

	if(m_SkillID == Skill::SkillSleepyGravity)
	{
		for(CSleepyGravity* pHh = (CSleepyGravity*)GS()->m_World.FindFirst(CGameWorld::ENTYPE_SLEEPYGRAVITY); pHh; pHh = (CSleepyGravity*)pHh->TypeNext())
		{
			if(pHh->m_pPlayer->GetCID() != ClientID)
				continue;

			pHh->Reset();
			break;
		}
		const int PowerLevel = PriceMana;
		new CSleepyGravity(&GS()->m_World, m_pPlayer, GetBonus(), PowerLevel, PlayerPosition);
		return true;
	}

	if(m_SkillID == Skill::SkillAttackTeleport)
	{
		new CAttackTeleport(&GS()->m_World, PlayerPosition, pChr, GetBonus());
		return true;
	}

	if(m_SkillID == Skill::SkillBlessingGodWar)
	{
		for(int i = 0; i < MAX_PLAYERS; i++)
		{
			CPlayer* pPlayer = GS()->GetPlayer(i, true, true);
			if(!pPlayer || !GS()->IsPlayerEqualWorldID(i) || distance(PlayerPosition, pPlayer->GetCharacter()->GetPos()) > 800
				|| (pPlayer->GetCharacter()->IsAllowedPVP(ClientID) && i != ClientID))
				continue;

			const int RealAmmo = 10 + pPlayer->GetAttributeSize(Attribute::Ammo);
			const int RestoreAmmo = translate_to_percent_rest(RealAmmo, min(GetBonus(), 100));
			for(int j = WEAPON_GUN; j <= WEAPON_LASER; j++)
			{
				pPlayer->GetCharacter()->GiveWeapon(j, RestoreAmmo);
				GS()->CreateDeath(PlayerPosition, i);
			}
			GS()->CreateSound(PlayerPosition, SOUND_CTF_GRAB_PL);
		}

		GS()->CreateText(NULL, false, vec2(PlayerPosition.x, PlayerPosition.y - 96.0f), vec2(0, 0), 40, "RECOVERY AMMO");
		return true;
	}
	return false;
}

bool CSkillData::Upgrade()
{
	if(!m_pPlayer || !m_pPlayer->IsAuthed() || m_Level >= Info()->GetMaxLevel())
		return false;

	if(!m_pPlayer->SpendCurrency(Info()->GetPriceSP(), itSkillPoint))
		return false;

	const int ClientID = m_pPlayer->GetCID();
	ResultPtr pRes = Sqlpool.Execute<DB::SELECT>("*", "tw_accounts_skills", "WHERE SkillID = '%d' AND UserID = '%d'", m_SkillID, m_pPlayer->Acc().m_UserID);
	if(pRes->next())
	{
		m_Level++;
		Sqlpool.Execute<DB::UPDATE>("tw_accounts_skills", "Level = '%d' WHERE SkillID = '%d' AND UserID = '%d'", m_Level, m_SkillID, m_pPlayer->Acc().m_UserID);
		GS()->Chat(ClientID, "Increased the skill [{STR} level to {INT}]", Info()->GetName(), m_Level);
		return true;
	}

	m_Level = 1;
	m_SelectedEmoticion = -1;
	Sqlpool.Execute<DB::INSERT>("tw_accounts_skills", "(SkillID, UserID, Level) VALUES ('%d', '%d', '1');", m_SkillID, m_pPlayer->Acc().m_UserID);
	GS()->Chat(ClientID, "Learned a new skill [{STR}]", Info()->GetName());
	return true;
}
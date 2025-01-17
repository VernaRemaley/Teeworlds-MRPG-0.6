/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

#include "mmocore/Components/Accounts/AccountData.h"
#include "mmocore/Components/Inventory/ItemData.h"
#include "mmocore/Components/Quests/QuestData.h"
#include "mmocore/Components/Skills/SkillData.h"

#include "entities/character.h"

#include "vote_event_optional.h"
#include "mmocore/Cooldown.h"

enum
{
	WEAPON_SELF = -2, // self die
	WEAPON_WORLD = -1, // swap world etc
};

class CPlayer
{
	MACRO_ALLOC_POOL_ID()

	struct StructLatency
	{
		int m_AccumMin;
		int m_AccumMax;
		int m_Min;
		int m_Max;
	};

	struct StructLastAction
	{
		int m_TargetX;
		int m_TargetY;
	};

	int m_SnapHealthTick;
	ska::unordered_map<int, bool> m_aHiddenMenu;

protected:
	CCharacter* m_pCharacter;
	CGS* m_pGS;

	IServer* Server() const;
	int m_ClientID;

	// lastest afk state
	bool m_Afk;
	bool m_LastInputInit;
	int64_t m_LastPlaytime;
	std::function<void()> m_PostVotes;

public:
	CGS* GS() const { return m_pGS; }
	vec2 m_ViewPos;
	int m_PlayerFlags;
	int m_aPlayerTick[NUM_TICK];
	char m_aClanString[128];
	Mood m_MoodState;
	CCooldown m_Cooldown {};

	char m_aLastMsg[256];
	int m_TutorialStep;

	StructLatency m_Latency;
	StructLastAction m_LatestActivity;

	/* ==========================================================
		VAR AND OBJECTS PLAYER MMO
	========================================================== */
	CTuningParams m_PrevTuningParams;
	CTuningParams m_NextTuningParams;
	CPlayerDialog m_Dialog;

	bool m_WantSpawn;
	short m_aSortTabs[NUM_SORT_TAB];
	int m_TempMenuValue;
	short m_CurrentVoteMenu;
	short m_LastVoteMenu;
	bool m_ZoneInvertMenu;
	bool m_RequestChangeNickname;
	int m_EidolonCID;
	bool m_ActivedGroupColors;
	int m_TickActivedGroupColors;

	/* ==========================================================
		FUNCTIONS PLAYER ENGINE
	========================================================== */
public:
	CNetObj_PlayerInput* m_pLastInput;

	CPlayer(CGS* pGS, int ClientID);
	virtual ~CPlayer();

	virtual int GetTeam();
	virtual bool IsBot() const { return false; }
	virtual int GetBotID() const { return -1; }
	virtual int GetBotType() const { return -1; }
	virtual int GetBotMobID() const { return -1; }
	virtual	int GetPlayerWorldID() const;
	virtual CTeeInfo& GetTeeInfo() const;

	virtual int GetStartHealth();
	int GetStartMana();
	virtual	int GetHealth() { return GetTempData().m_TempHealth; }
	virtual	int GetMana() { return GetTempData().m_TempMana; }
	bool IsAfk() const { return m_Afk; }
	int64_t GetAfkTime() const;

	void FormatBroadcastBasicStats(char* pBuffer, int Size, const char* pAppendStr = "\0");

	virtual void HandleTuningParams();
	virtual int64_t GetMaskVisibleForClients() const { return -1; }
	virtual int IsActiveForClient(int ClientID) const { return 2; }
	virtual int GetEquippedItemID(ItemFunctional EquipID, int SkipItemID = -1) const;
	virtual int GetAttributeSize(AttributeIdentifier ID);
	float GetAttributePercent(AttributeIdentifier ID);
	virtual void UpdateTempData(int Health, int Mana);

	virtual void GiveEffect(const char* Potion, int Sec, float Chance = 100.0f);
	virtual bool IsActiveEffect(const char* Potion) const;
	virtual void ClearEffects();

	virtual void Tick();
	virtual void PostTick();
	virtual void Snap(int SnappingClient);
	virtual void FakeSnap();

	void RefreshClanString();

	void SetPostVoteListCallback(const std::function<void()> pFunc) { m_PostVotes = pFunc; }
	bool IsActivePostVoteList() const { return m_PostVotes != nullptr; }
	void PostVoteList();

	virtual bool IsActive() const { return true; }
	class CPlayerBot* GetEidolon() const;
	void TryCreateEidolon();
	void TryRemoveEidolon();

private:
	virtual void HandleEffects();
	virtual void TryRespawn();
	void HandleScoreboardColors();
	void HandlePrison();

public:
	CCharacter* GetCharacter() const;

	void KillCharacter(int Weapon = WEAPON_WORLD);
	void OnDisconnect();
	void OnDirectInput(CNetObj_PlayerInput* pNewInput);
	void OnPredictedInput(CNetObj_PlayerInput* pNewInput) const;

	int GetCID() const { return m_ClientID; }
	/* ==========================================================
		FUNCTIONS PLAYER HELPER
	========================================================== */
	void ProgressBar(const char* Name, int MyLevel, int MyExp, int ExpNeed, int GivedExp) const;
	bool Upgrade(int Value, int* Upgrade, int* Useless, int Price, int MaximalUpgrade) const;

	/* ==========================================================
		FUNCTIONS PLAYER ACCOUNT
	========================================================== */
	const char* GetLanguage() const;

	bool GetHiddenMenu(int HideID) const;
	bool IsAuthed() const;
	int GetStartTeam() const;

	/* ==========================================================
		FUNCTIONS PLAYER PARSING
	========================================================== */
	bool ParseVoteOptionResult(int Vote);
	bool ParseVoteUpgrades(const char* CMD, int VoteID, int VoteID2, int Get);
	bool IsClickedKey(int KeyID) const;

	/* ==========================================================
		FUNCTIONS PLAYER ITEMS
	========================================================== */
	class CPlayerItem* GetItem(const CItem& Item) { return GetItem(Item.GetID()); }
	virtual class CPlayerItem* GetItem(ItemIdentifier ID);
	class CSkill* GetSkill(SkillIdentifier ID);
	class CPlayerQuest* GetQuest(QuestIdentifier ID);
	CAccountTempData& GetTempData() const { return CAccountTempData::ms_aPlayerTempData[m_ClientID]; }
	CAccountData* Account() const { return &CAccountData::ms_aData[m_ClientID]; }

	int GetTypeAttributesSize(AttributeType Type);
	int GetAttributesSize();

	void SetSnapHealthTick(int Sec);

	virtual Mood GetMoodState() const { return Mood::NORMAL; }
	void ChangeWorld(int WorldID);

	/* ==========================================================
	   VOTING OPTIONAL EVENT
	========================================================== */
	// Function: CreateVoteOptional
	// Parameters:
	//    - OptionID: an integer value representing the vote option
	//    - OptionID2: an integer value representing the vote option
	//    - Sec: an integer value representing the duration of the vote event
	//    - pInformation: a pointer to a character array containing optional information for the vote
	//    - ...: additional optional arguments for the information text format
	// Return:
	//    - a pointer to a CVoteEventOptional object representing the created vote event
	CVoteEventOptional* CreateVoteOptional(int OptionID, int OptionID2, int Sec, const char* pInformation, ...);

private:
	// Function: RunEventOptional
	// Parameters:
	//    - Option: an integer value representing the selected option for the vote event
	//    - pOptional: a pointer to a CVoteEventOptional object representing the vote event
	// Description:
	//    - Runs the selected optional vote event
	void RunEventOptional(int Option, CVoteEventOptional* pOptional);

	// Function: HandleVoteOptionals
	// Description:
	//    - Handles all the optional vote events
	void HandleVoteOptionals();
};

#endif

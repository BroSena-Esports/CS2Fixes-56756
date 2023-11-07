/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023 Source2ZE
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "KeyValues.h"
#include "commands.h"
#include "ctimer.h"
#include "eventlistener.h"
#include "entity/cbaseplayercontroller.h"
#include "entity/ccsplayercontroller.h"
#include "adminsystem.h"

#include "tier0/memdbgon.h"
#include "playermanager.h"

extern IGameEventManager2 *g_gameEventManager;
extern IServerGameClients *g_pSource2GameClients;
extern CEntitySystem *g_pEntitySystem;
extern CGlobalVars *gpGlobals;

CUtlVector<CGameEventListener *> g_vecEventListeners;

void RegisterEventListeners()
{
	static bool bRegistered = false;

	if (bRegistered || !g_gameEventManager)
		return;

	FOR_EACH_VEC(g_vecEventListeners, i)
	{
		g_gameEventManager->AddListener(g_vecEventListeners[i], g_vecEventListeners[i]->GetEventName(), true);
	}

	bRegistered = true;
}

void UnregisterEventListeners()
{
	if (!g_gameEventManager)
		return;

	FOR_EACH_VEC(g_vecEventListeners, i)
	{
		g_gameEventManager->RemoveListener(g_vecEventListeners[i]);
	}

	g_vecEventListeners.Purge();
}

// CONVAR_TODO
bool g_bForceCT = false; //edited from true

CON_COMMAND_F(c_force_ct, "toggle forcing CTs on every round", FCVAR_SPONLY | FCVAR_LINKED_CONCOMMAND)
{
	if (args.ArgC() > 1)
		g_bForceCT = V_StringToBool(args[1], true);

	Message("Forcing CTs on every round is now %s.\n", g_bForceCT ? "ON" : "OFF");
}

GAME_EVENT_F(round_prestart)
{
	if (!g_bForceCT)
		return;

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CCSPlayerController* pController = CCSPlayerController::FromSlot(i);

		// Only do this for Ts, ignore CTs and specs
		if (!pController || pController->m_iTeamNum() != CS_TEAM_T)
			continue;

		pController->SwitchTeam(CS_TEAM_CT);
	}
}

bool g_bBlockTeamMessages = true;

CON_COMMAND_F(c_block_team_messages, "toggle team messages", FCVAR_SPONLY | FCVAR_LINKED_CONCOMMAND)
{
	if (args.ArgC() > 1)
		g_bBlockTeamMessages = V_StringToBool(args[1], true);
}

GAME_EVENT_F(player_team)
{
	// Remove chat message for team changes
	if (g_bBlockTeamMessages)
		pEvent->SetBool("silent", true);
}

// CONVAR_TODO: have a convar for forcing debris collision

GAME_EVENT_F(player_spawn)
{
	CCSPlayerController *pController = (CCSPlayerController *)pEvent->GetPlayerController("userid");

	if (!pController)
		return;
//*******************************Medic****************************
	int iPlayer = pController->GetPlayerSlot();
		ZEPlayer* pZEPlayer = g_playerManager->GetPlayer(iPlayer);

		if (pZEPlayer)
		{
			pZEPlayer->SetUsedMedkit(false);
		}
//*******************************Medic****************************
		CBasePlayerPawn *pPawn = pController->GetPawn();
	CHandle<CCSPlayerController> hController = pController->GetHandle();

	// Gotta do this on the next frame...
	new CTimer(0.0f, false, [hController]()
	{
		CCSPlayerController *pController = hController.Get();

		if (!pController || !pController->m_bPawnIsAlive())
			return -1.0f;
			
		int iPlayer = pController->GetPlayerSlot();					
		ZEPlayer* pZEPlayer = g_playerManager->GetPlayer(iPlayer);
		//clan tag
		if (pZEPlayer->IsAdminFlagSet(ADMFLAG_ROOT))				
        	{
            		pController->m_szClan("[OWNER]");     				
        	} 
		else if (pZEPlayer->IsAdminFlagSet(ADMFLAG_CUSTOM6))				
        	{
            		pController->m_szClan("[CO-OWNER]");     				
        	} 
		else if (pZEPlayer->IsAdminFlagSet(ADMFLAG_CUSTOM2))				
        	{
            		pController->m_szClan("[ADMIN]");     				
        	} 
		else if (pZEPlayer->IsAdminFlagSet(ADMFLAG_CUSTOM3))				
        	{
            		pController->m_szClan("[MOD]");     				
        	} 
		else if (pZEPlayer->IsAdminFlagSet(ADMFLAG_CUSTOM1))				
        	{
            		pController->m_szClan("[HELPER]");     				
        	} 		
		else if (pZEPlayer->IsAdminFlagSet(ADMFLAG_RESERVATION))				
        	{
            		pController->m_szClan("[VIP]");     				
        	} 
		else if (pZEPlayer->IsAdminFlagSet(ADMFLAG_RESERVATION))				
        	{
            		pController->m_szClan("[VIP]");     				
        	} 
		else 			
        	{
            		pController->m_szClan("[BroSena]");     				
        	}

		CBasePlayerPawn *pPawn = pController->GetPawn();

		// Just in case somehow there's health but the player is, say, an observer
		if (!pPawn || !pPawn->IsAlive())
			return -1.0f;

		pPawn->m_pCollision->m_collisionAttribute().m_nCollisionGroup = COLLISION_GROUP_DEBRIS;
		pPawn->m_pCollision->m_CollisionGroup = COLLISION_GROUP_DEBRIS;
		pPawn->CollisionRulesChanged();

		return -1.0f;
	});
}

GAME_EVENT_F(player_hurt)
{
	CCSPlayerController *pAttacker = (CCSPlayerController*)pEvent->GetPlayerController("attacker");
	CCSPlayerController *pVictim = (CCSPlayerController*)pEvent->GetPlayerController("userid");

	// Ignore Ts/zombies and CTs hurting themselves
	if (!pAttacker || pAttacker->m_iTeamNum() != CS_TEAM_CT || pAttacker == pVictim)
		return;

	ZEPlayer* pPlayer = pAttacker->GetZEPlayer();

	if (!pPlayer)
		return;

	pPlayer->SetTotalDamage(pPlayer->GetTotalDamage() + pEvent->GetInt("dmg_health"));
}

int g_iBombTimerCounter = 0;

GAME_EVENT_F(round_start)
{
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		ZEPlayer* pPlayer = g_playerManager->GetPlayer(i);

		if (!pPlayer)
			continue;

		pPlayer->SetTotalDamage(0);
	}
	
	g_iBombTimerCounter = 0;
}

GAME_EVENT_F(bomb_planted)
{
    ConVar* cvar = g_pCVar->GetConVar(g_pCVar->FindConVar("mp_c4timer"));

    int iC4;
    memcpy(&iC4, &cvar->values, sizeof(iC4));

    g_iBombTimerCounter = iC4;

    new CTimer(1.0f, false, []()
    {
        if (g_iBombTimerCounter <= 0)
            return -1.0f;

        g_iBombTimerCounter--;

        ClientPrintAll(HUD_PRINTCENTER, "C4: %d", g_iBombTimerCounter);
        return 1.0f;
    });
}

GAME_EVENT_F(bomb_defused)
{
    g_iBombTimerCounter = 0;
}

GAME_EVENT_F(round_end)
{
    g_iBombTimerCounter = 0;
}

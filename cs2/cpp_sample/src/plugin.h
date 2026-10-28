/**
* vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * Source2Mod
 * Copyright (C) 2025-2026 Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl.
 * All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
 * gives you permission to link the code of this program
 * (as well as its derivative works) to "Counter-Strike 2," "Source 2,"
 * "Steam," and any Game MODs or server software running on software by
 * Valve Corporation. You must obey the GNU General Public License in all
 * respects for all other code used.
 *
 * Additionally, this exception applies to all derivative works unless
 * otherwise stated in LICENSE.txt.
 *
 * Authors:
 *   - Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
 *
 * Project: Source2Mod
 */

/*
 * One plugin that walks through Source2Mod, a section at a time. It started
 * as a port of Metamod:Source's own s2_sample_mm -- the same hooks, the same
 * convars, the same command, the same level callbacks -- and grew a section
 * for every subsystem a plugin usually reaches for first:
 *
 *   1. Lifecycle        Load / Unload / ISource2ModListener
 *   2. ConVars          CConVar, CConVarRef, change hook
 *   3. Commands         console + chat commands, arguments, replies, listeners
 *   4. Game events      pre / post hooks, dontBroadcast, firing an event
 *   5. Core events      entity listener, entity outputs, client callbacks,
 *                       GameFrame, a client command
 *   6. Net messages     building + sending, outgoing and incoming hooks
 *   7. Sounds           one-call emit, sound objects, stopping, the sound hook
 *   8. Game functions   typed points (TakeDamage, PostThink), any function by
 *                       gamedata / address / signature, calling one
 *   9. Entities         schema fields, teleport, items, inputs
 *  10. Timers           next frame, delayed, repeating
 *  11. Transmit         hiding players and entities per viewer, CheckTransmit hook
 *  12. Script natives   natives, a library and a handle type for .sp plugins
 *  13. Paths, permissions  the folders, permission checks, groups, a gated command
 *  14. Traces           a ray from the eyes, what it hit, contents at a point
 *  15. Game systems, addresses  finding a game system, calling a resolved function
 *  16. JSON, HTTP       parsing a document, a GET with headers and a JSON reply
 *  17. MySQL            serverless query with parameters, connection, transaction
 *  18. Menus, custom HUD  a center HTML menu, clicks on a custom HUD layout
 *  19. Scripts          cs_script source run from the plugin, messages both ways
 *  20. Configs          a JSON config declared with its defaults, reloaded live,
 *                       a value changed and saved
 *  21. Clients          the engine client of a slot, asking a client for a ConVar,
 *                       disconnecting from inside its own command
 *
 * ../sp_sample is the same plugin in SourcePawn: the same sections, numbered
 * the same, doing the same things through the script API.
 *
 * plugin.cpp carries the same numbers in its section banners. Each section is
 * a SetupXxx() called from Load() followed by the handlers it registers, so a
 * section can be lifted out into a plugin of its own as it stands.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * This sample plugin is public domain.
 */

#ifndef _INCLUDE_SOURCE2MOD_SAMPLE_PLUGIN_H_
#define _INCLUDE_SOURCE2MOD_SAMPLE_PLUGIN_H_

#include "source2mod/ISource2ModPlugin.h"
#include "source2mod/ISource2ModApi.h"

// SOURCE2MOD_SAVEVARS() fills one global per subsystem, so every subsystem header
// has to be in scope for its interface name.
#include "source2mod/ISource2ModAddresses.h"
#include "source2mod/ISource2ModCommands.h"
#include "source2mod/ISource2ModConfigs.h"
#include "source2mod/ISource2ModConVars.h"
#include "source2mod/ISource2ModCustomHud.h"
#include "source2mod/ISource2ModEntities.h"
#include "source2mod/ISource2ModEvents.h"
#include "source2mod/ISource2ModGameConfig.h"
#include "source2mod/ISource2ModFunctions.h"
#include "source2mod/ISource2ModHooks.h"
#include "source2mod/ISource2ModGameSystems.h"
#include "source2mod/ISource2ModHTTP.h"
#include "source2mod/ISource2ModJSON.h"
#include "source2mod/ISource2ModMemory.h"
#include "source2mod/ISource2ModMenus.h"
#include "source2mod/ISource2ModModule.h"
#include "source2mod/ISource2ModSQL.h"
#include "source2mod/ISource2ModNetworkMessages.h"
#include "source2mod/ISource2ModScheduler.h"
#include "source2mod/ISource2ModSounds.h"
#include "source2mod/ISource2ModTrace.h"
#include "source2mod/ISource2ModPaths.h"
#include "source2mod/ISource2ModPermissions.h"
#include "source2mod/ISource2ModScripts.h"
#include "source2mod/ISource2ModTransmit.h"

#include "source2mod/sdk/schema/classes/CBaseEntity.h"
#include "source2mod/sdk/schema/classes/CCSPlayerPawn.h"
#include "source2mod/sdk/game/server/entityio.h"
#include "source2mod/sdk/game/shared/takedamageinfo.h"
#include "source2mod/sdk/game/shared/takedamageresult.h"
#include "entity2/entitysystem.h"
#include "igameevents.h"
#include "engine2/isource2server.h"
#include "engine2/ivengineserver2.h"

// Generated from plugin-metadata.json by tools/version_gen.py -- the metadata
// below is not written twice.
#include "version_gen.h"

SOURCE2MOD_GLOBALVARS();

class SamplePlugin final : public ISource2ModPlugin,
                           public ISource2ModListener,
                           public IEntityListener,
                           public IEntityOutputListener,
                           public ISource2ModConfigListener
{
public:
	bool Load(PluginId id, ISource2ModAPI *api, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;

public: // 1. lifecycle -- ISource2ModListener
	void OnAllSource2ModPluginsLoaded() override;
	void OnLevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background) override;
	void OnLevelShutdown() override;

public: // 5. core events -- ISource2ModEntities listeners
	void OnEntitySpawned(CEntityInstance *pEntity) override;
	Action OnEntityOutput(CEntityInstance *pCaller, const char *pszOutput, CEntityInstance *pActivator, float flDelay, bool post) override;

public: // 5. core events -- ISource2ModHooks handlers: engine callbacks, a client command
	HookReturn<bool> OnClientConnect(ClientConnectContext &ctx, bool post);
	HookReturn<void> OnClientPutInServer(ClientPutInServerContext &ctx, bool post);
	HookReturn<void> OnClientSettingsChanged(ClientSlotContext &ctx, bool post);
	HookReturn<void> OnClientDisconnect(ClientDisconnectContext &ctx, bool post);
	HookReturn<void> OnGameFrame(GameFrameContext &ctx, bool post);
	HookReturn<void> OnClientCommand(FunctionContext &ctx, bool post);

public: // 20. configs -- ISource2ModConfigListener: configs/C++ Sample/config.json
	void OnDeclareConfigs(ISource2ModConfigDeclarations *configs) override;
	bool OnConfigValidate(ISource2ModConfig *config, const ISource2ModJSONValue *root, char *error, size_t maxlen) override;
	void OnConfigChanged(ISource2ModConfig *config, ConfigChange reason) override;

public: // 8. game functions -- a typed point, and any function by gamedata, address or signature
	HookReturn<TakeDamageContext::Return> OnTakeDamage(TakeDamageContext &ctx, bool post);
	HookReturn<void> OnGiveNamedItem(FunctionContext &ctx, bool post);
	HookReturn<void> OnSwitchTeam(FunctionContext &ctx, bool post);
	HookReturn<void> OnUtilRemove(FunctionContext &ctx, bool post);
	HookReturn<void> OnSnapViewAngles(FunctionContext &ctx, bool post);

private: // one per section of plugin.cpp, called from Load() in this order
	void SetupConVars();
	void SetupCommands();
	void SetupGameEvents();
	void SetupCoreEvents();
	void SetupNetMessages();
	void SetupSounds();
	void SetupGameFunctions();
	void SetupEntityCommands();
	void SetupTimers();
	void SetupTransmit();
	void SetupScriptNatives();
	void SetupPathsAndPermissions();
	void SetupTraces();
	void SetupGameSystems();
	void SetupJsonAndHttp();
	void SetupMySQL();
	void SetupMenus();
	void SetupScripts();
	void SetupConfigs();
	void SetupClients();

private:
	// 6. net messages -- one bit per player slot, the same layout the hooks get
	// their recipients in.
	uint64_t m_NoShakeMask = 0;     // players who turned screen shakes off (!noshake)
	uint64_t m_VoiceMutedMask = 0;  // players whose voice is dropped (sample_mute)

	// 10. timers -- a Timer* is only good while the timer lives; see SetupTimers().
	Timer *m_pCountdownTimer = nullptr;
	int m_nCountdown = 0;

	// 20. configs -- copied out of config.json in OnConfigChanged(); the JSON
	// values themselves are only good until the next reload.
	std::string m_sConfigPrefix;
	bool m_bFeatureEnabled = false;
	float m_flFeatureStrength = 0.0f;
	int m_nConfigAdmins = 0;

	// 17. MySQL -- made on first use, gone with the plugin.
	ISource2ModSQLServerless *m_pServerless = nullptr;
	ISource2ModSQLConnection *m_pConnection = nullptr;

public:
	const char *GetAuthor() override { return PLUGIN_AUTHOR; }
	const char *GetName() override { return PLUGIN_DISPLAY_NAME; }
	const char *GetDescription() override { return PLUGIN_DESCRIPTION; }
	const char *GetVersion() override { return PLUGIN_FULL_VERSION; }
};

extern SamplePlugin g_Plugin;

#endif //_INCLUDE_SOURCE2MOD_SAMPLE_PLUGIN_H_

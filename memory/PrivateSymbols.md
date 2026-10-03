---
title: PrivateSymbols
type: reference
permalink: bulletphysics/private-symbols
tags:
- bulletphysics
- private-vars
- private-funcs
- private-globals
- gamedata
- reference
---

# Game-private symbols used by `BulletPhysics`

Every engine/client private symbol resolves through the host gamedata contract:
`g_pMetaHookAPI->ResolveGameSymbol(moduleBase, name, kind, &address)` for
`FUNCTION` / `GLOBAL` / `VIRTUAL_FUNCTION`. There is no signature scan, no Capstone
dependency, no mirror module image and no scan fallback. The authoritative consumer
contract is `scripts/manifests/bulletphysics.json`; this note lists only the symbols
that are resolved today, together with the code that consumes them.

## Resolution contract

- `GamedataResolvePtr` (`src/privatehook.h`) wraps the host call. The `required` argument
  mirrors the manifest exactly: a required symbol that is missing raises
  `Sys_Error("Could not resolve gamedata symbol: <name> (module <module>, <status>)")`, an
  optional one returns `nullptr` and the caller guards for null.
- Resolution is driven from four entry points, all passing the real module base:
  `Engine_FillAddress(g_EngineDLLInfo.ImageBase)` and
  `Client_FillAddress(g_ClientDLLInfo.ImageBase)` from `LoadEngine` / `LoadClient`, plus
  `EngineStudio_FillAddress` / `ClientStudio_FillAddress` from `HUD_GetStudioModelInterface`.
  MetaHook derives the module CRC-64/XZ from the on-disk file and returns
  `moduleBase + rva`; no RVA remapping is involved.
- `src/privatehook.cpp` asserts `METAHOOK_API_VERSION >= 112`: the gamedata resolution
  surface (FUNCTION / GLOBAL / VIRTUAL_FUNCTION) is a hard host requirement.
- Missing symbols surface either as a gamedata validator failure at build/publish time
  (`scripts/validate-gamedata.py` BulletPhysics consumer gate) or as the `Sys_Error` above
  at load — never as a silent skip.
- Engine-only builds without a client module (`hl-3248/3266/3329/3647/4554/6153`) never
  publish client symbols; the manifest declares them under `symbolExemptions` for
  `g_iUser1` / `g_iUser2` rather than gating them per build in code.

## Engine module symbols (`ResolveGameSymbol` with the engine image base)

| Symbol | Kind | Plugin field | Requirement | Consumer |
| --- | --- | --- | --- | --- |
| `R_NewMap` | function | `gPrivateFuncs.R_NewMap` | required | hooked from `Engine_InstallHook`; wrapper runs the original, then `ClientPhysicManager()->NewMap()`, `ClientEntityManager()->NewMap()`, `g_pViewPort->NewMap()` |
| `R_RenderView` | function | `gPrivateFuncs.R_RenderView` or `R_RenderView_SvEngine` | required | hooked from `Engine_InstallHook`; the SvEngine variant takes `int viewIdx` and is selected by `g_iEngineType`, only one of the two fields is populated |
| `V_RenderView` | function | `gPrivateFuncs.V_RenderView` | required | forced view refresh in `HUD_TempEntUpdate` (`CAM_Think()` + `V_RenderView()`) |
| `R_CullBox` | function | `gPrivateFuncs.R_CullBox` | required | `BasePhysicManager.cpp` (`StudioCheckBBox` visibility) via the `R_CullBox` wrapper in `exportfuncs.cpp` |
| `R_StudioDrawModel` / `R_StudioDrawPlayer` / `R_StudioSetupBones` | function | `gPrivateFuncs.R_Studio*` | required | hooked from `ClientStudio_InstallHooks`; engine-side Studio path |
| `cl_max_edicts` | global | `int* cl_max_edicts` (`*ptr`) | required | `EngineGetMaxClientEdicts()` -> `ClientEntityManager.cpp`, `exportfuncs.cpp` |
| `cl_entities` | global | `cl_entity_t** cl_entities` (`*ptr`) | required | `EngineGetClientEntitiesBase()` -> `ClientEntityManager.cpp` |
| `gTempEnts` | global | `TEMPENTITY* gTempEnts` (array base) | required | `EngineGetTempTentsBase()` / `EngineGetTempTentByIndex()` -> `ClientEntityManager.cpp` |
| `cl_viewentity` | global | `int* cl_viewentity` (`*ptr`) | required | `CL_IsFirstPersonMode()` in `exportfuncs.cpp` |
| `mod_known` | global | `void* mod_known` (array base) | required | `EngineGetModelIndex()` / `EngineGetModelByIndex()` / `EngineFindWorldModelBySubModel()` -> `BasePhysicManager.cpp`, `ClientEntityManager.cpp`, `Base*Object.cpp`, `PhysicUTIL.cpp`, `PhysicDebugGUI.cpp`, config-edit dialogs |
| `mod_numknown` | global | `int* mod_numknown` (`*ptr`) | required | `EngineGetNumKnownModel()` / `EngineGetMaxKnownModel()` -> `BasePhysicManager.cpp` |
| `cl_parsecount` | global | `int* cl_parsecount` (`*ptr`) | required | `ClientEntityManager.cpp` (PVS delay check), `exportfuncs.cpp` (messagenum checks) |
| `cl_numvisedicts` / `cl_visedicts` | global | `int*` / `cl_entity_t**` | required | `ClientEntityManager.cpp` visible-entity list membership |
| `r_worldentity` | global | `cl_entity_t* r_worldentity` (object base) | required | `BaseDynamicObject.cpp`, `BaseStaticObject.cpp` (skip the world as dynamic/static), `BasePhysicManager.cpp` (world brush model), `BulletStaticRigidBody.cpp` |
| `cl_worldmodel` | global | `model_t** cl_worldmodel` (`*ptr`) | required | `BasePhysicManager.cpp` (world surface/model lookup, `BULLET_WORLD` debug level, world-node index arrays) |
| `currententity` | global | `cl_entity_t** currententity` (`*ptr`) | required | save/set/restore around engine Studio calls: `BasePhysicManager.cpp`, `ClientEntityManager.cpp`, `exportfuncs.cpp` |
| `pstudiohdr` | global | `studiohdr_t** pstudiohdr` (`*ptr`) | required | `exportfuncs.cpp` (`StudioSetupBones_Template`, `studioapi_StudioCheckBBox`) |
| `r_origin` | global | `float* r_origin` | required | `EngineGetRendererViewOrigin()` -> `PhysicDebugGUI.cpp` (debug picking/tracing) |
| `allow_cheats` | global | `int* allow_cheats` (`*ptr`) | required, **SvEngine only** | `AllowCheats()` -> `exportfuncs.cpp`, `PhysicDebugGUI.cpp`; other engines leave the pointer null and use the `sv_cheats` cvar |

`allow_cheats` is the only engine symbol resolved behind a conditional: it is requested
only when `g_iEngineType == ENGINE_SVENGINE`, matching the manifest's Sven-only group.

## Client module symbols (`ResolveGameSymbol` with the client image base)

| Symbol | Kind | Plugin field | Requirement | Consumer |
| --- | --- | --- | --- | --- |
| `g_iUser1` / `g_iUser2` | global | `int*` (`*ptr`) | required | `exportfuncs.cpp` `V_CalcRefdef` spectator resolution; `g_iUser1` also in `BaseRagdollObject.cpp` |
| `g_bRenderingPortals_SCClient` | global | `bool*` (`*ptr`) | required, Sven Co-op only | `R_IsRenderingPortals()` -> `exportfuncs.cpp` skips view sync during portal rendering |
| `g_pitchdrift` | global | `pitchdrift_t*` | required, Sven Co-op only | `exportfuncs.cpp` saves/restores pitch drift around the forced `CAM_Think()` + `V_RenderView()` |
| `g_ViewEntityIndex_SCClient` | global | `int*` (`*ptr`) | **optional** (`required=false`) | `BasePhysicManager.cpp`: zero/restore around `StudioDrawPlayer`/`StudioDrawModel` in the ragdoll bone-setup paths |
| `g_PlayerExtraInfo` | global | `extra_player_info_t (*)[65]` | required for `cstrike` / `czero` | `CounterStrike.cpp` `CounterStrike_IsVIP()` / `CounterStrike_GetTeamNumber()` |
| `g_PlayerExtraInfo_CZDS` | global | `extra_player_info_czds_t (*)[65]` | required for `czeror` | `CounterStrike.cpp` (the `_CZDS` layout) |
| `GameStudioRenderer_StudioDrawModel` | virtualFunction | `__fastcall` fn ptr | **optional** | hooked when present from `ClientStudio_InstallHooks` |
| `GameStudioRenderer_StudioDrawPlayer` | virtualFunction | `__fastcall` fn ptr | **optional** | hooked when present from `ClientStudio_InstallHooks` |
| `GameStudioRenderer_StudioSetupBones` | virtualFunction | `__fastcall` fn ptr | **optional** | hooked when present from `ClientStudio_InstallHooks` |

The three `GameStudioRenderer_*` virtuals are resolved directly by symbol name
(`src/exportfuncs.cpp`, `ClientStudio_FillAddress`) — the host performs the vtable lookup,
so the plugin no longer derives vtable indices. Clients that do not publish them keep the
engine-side `R_Studio*` hooks only, which is the intended path for SvEngine client installs.

### Sven Co-op client view-entity slot

`g_ViewEntityIndex_SCClient` is the only optional client global: `svencoop-10257` publishes
it and the manifest requires it there; `svencoop-8948` never contains it. Resolving it with
`required=false` plus the existing null guards in `BasePhysicManager.cpp` is the intended
path for 8948 — do not reintroduce a build-number heuristic or a scan fallback. Both Sven
versions still require portal state (`g_bRenderingPortals_SCClient`) and pitchdrift.

## Interface-derived pointers (not game-private)

Stored in `gPrivateFuncs` and declared in `src/privatehook.h`, but sourced from public
interfaces rather than gamedata:

| Local symbol | Source | Use |
| --- | --- | --- |
| `studioapi_StudioCheckBBox` | `pstudio->StudioCheckBBox` (public `engine_studio_api_t`) | inline-hooked from `EngineStudio_InstallHooks`; the handler consults physics bbox visibility then calls the original |
| `efxapi_R_TempModel` | `gEngfuncs.pEfxAPI->R_TempModel` (public `cl_enginefunc_t`) | inline-hooked from `HUD_Init` when the `ClCorpse` message is hooked; the handler tags corpse temp-entities with `PhyCorpseFlag` |
| `pbonetransform` / `plighttransform` | `pstudio->StudioGetBoneTransform()` / `StudioGetLightTransform()` in `HUD_GetStudioModelInterface` | bone/light matrices read and written by the ragdoll paths in `BulletDynamicObject.cpp`, `BulletDynamicRigidBody.cpp`, `BulletRagdollObject.cpp`, `BulletRagdollRigidBody.cpp`, `BulletPhysicManager.cpp` |

`IEngineStudio.SetupPlayerModel`, `StudioSetRemapColors`, `Mod_ForName`,
`StudioGetBoneTransform/LightTransform` and `GetPlayerState` are likewise public API used
as anchors or direct calls, not private symbols.

## Hooks installed

| Target | Installed from | Kind |
| --- | --- | --- |
| `R_NewMap` | `Engine_InstallHook` | inline |
| `R_RenderView` (or `R_RenderView_SvEngine` on SvEngine) | `Engine_InstallHook` | inline |
| `GameStudioRenderer_StudioSetupBones` / `_StudioDrawPlayer` / `_StudioDrawModel` | `ClientStudio_InstallHooks` | inline, only when the optional virtual resolved |
| `R_StudioSetupBones` / `R_StudioDrawPlayer` / `R_StudioDrawModel` | `ClientStudio_InstallHooks` | inline |
| `studioapi_StudioCheckBBox` | `EngineStudio_InstallHooks` (from `HUD_GetStudioModelInterface`) | inline |
| `efxapi_R_TempModel` | `HUD_Init` | inline |

`Engine_UninstallHook` restores the two engine render hooks; `ClientStudio_UninstallHooks`
and `EngineStudio_UninstallHooks` restore the Studio hooks.

## Gating and layout assumptions

- Sven Co-op-only: `allow_cheats` (engine), `g_bRenderingPortals_SCClient`,
  `g_pitchdrift`, `g_ViewEntityIndex_SCClient`. Selected by the `SCClientDLL001` client
  factory in `Client_FillAddress`; the corresponding plugin paths are gated by
  `g_bIsSvenCoop` / engine type.
- Counter-Strike-only: `g_PlayerExtraInfo` / `g_PlayerExtraInfo_CZDS`, selected by the
  game directory (`cstrike` / `czero` / `czeror`). `dod` only sets `g_bIsDayOfDefeat`.
- Layout assumptions are compile-time asserted in `src/privatehook.h`:
  `sizeof(extra_player_info_t) == 0x74` and `sizeof(extra_player_info_czds_t) == 0x1C`.
- The engine globals are addressed as raw addresses or single-dereference pointers, not as
  gamedata slots: `cl_entities` / `cl_max_edicts` / `cl_parsecount` / `mod_numknown` /
  `cl_numvisedicts` / `cl_worldmodel` are pointer slots (`*ptr`), while `mod_known`,
  `gTempEnts` and `r_worldentity` are array/object bases used directly.
- `R_RenderView` and the `GameStudioRenderer_*` Studio entry points come from **two
  different spaces**: the engine render hook targets the engine function, while the client
  hook targets the client DLL's `CGameStudioRenderer` virtuals. Both are installed so
  client- and engine-side renders are covered.
- The `bv_*` cvars/commands, the Bullet3 world and the VGUI2Extension UI are plugin-owned
  and out of scope for this note.

## Callers

- `IPluginsV4::LoadEngine` (`plugins.cpp`) calls `Engine_FillAddress` then `Engine_InstallHook`.
- `IPluginsV4::LoadClient` (`plugins.cpp`) calls `Client_FillAddress` then `Client_InstallHooks`.
- `HUD_GetStudioModelInterface` (`exportfuncs.cpp`) calls `EngineStudio_FillAddress` + `EngineStudio_InstallHooks`, then `ClientStudio_FillAddress` + `ClientStudio_InstallHooks`.
- `HUD_Init` installs the `efxapi_R_TempModel` hook; `Engine_UninstallHook` / `ClientStudio_UninstallHooks` / `EngineStudio_UninstallHooks` restore them.

Related: [[project_overview]] [[build_and_verification]].

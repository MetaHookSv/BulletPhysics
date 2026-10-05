---
title: project_overview
type: note
permalink: bulletphysics/project-overview
---

# BulletPhysics Standalone Project

## Scope and Provenance

Migrated from `Plugins/BulletPhysics`, `Build/svencoop/bulletphysics` and the downloads
physics-data subset of MetaHookSv `fe80b6d60bfb487b52aed7ea7ec0492e7b27a5d2`; project
conventions follow the standalone Renderer. 127 source/header files, 17 UI/localization
files, 13 physics configurations and 2 OBJ files were kept as-is. The only source behavior
change is the optional view-entity slot resolution for Sven 8948/10257.

## Overview

Source directory: `src/`.

BulletPhysics is MetaHookSv's client-side physics plugin. It provides client-side physics object/ragdoll simulation and debug UI for GoldSrc/SvEngine based on Bullet3, integrating through the MetaHookSv plugin interface and applying the required hooks to engine/client rendering and animation flows. It maps GoldSrc/SvEngine entities, Studio models, and the BSP world into a Bullet3 physics world, providing construction, updates, render synchronization, debug drawing, and configuration editing for static objects, dynamic objects, and ragdolls.

## Responsibilities
- Initializes/destroys the physics manager during the IPluginsV4 lifecycle, retaining engine, client, filesystem, and MetaHook contexts.
- Enumerates players, network entities, and temporary entities active in the current frame, creating or removing physics objects according to model configuration.
- Maintains physics configurations indexed by modelindex and runtime objects, rigid bodies, constraints, and behaviors indexed by entity/component ID.
- Manages the Bullet dynamics world, collision filtering, gravity, fixed-timestep simulation, ray testing, and debug drawing.
- Builds physics configurations as StaticObject, DynamicObject, or RagdollObject, assembling components in rigid body → constraint → behavior order.
- Performs bidirectional synchronization between animated bones and Bullet rigid bodies through StudioSetupBones, StudioDrawModel/Player, and StudioCheckBBox hooks.
- Provides bv_* console commands, a VGUI2Extension debug window, object/component inspect-and-select functionality, and configuration reload/save capabilities.

## Architecture and Entry Points
- `src/plugins.cpp`: IPluginsV4 lifecycle, host API and dependency wiring.
- `src/privatehook.cpp`, `src/exportfuncs.cpp`: engine/client gamedata, hooks, frame and Studio bone synchronization.
- `src/ClientPhysicManager.h`, `src/BasePhysicManager.*`: physics system abstraction layer; `BasePhysicManager` carries most of the configuration/object-management logic.
- `src/BulletPhysicManager.*`: Bullet3 implementation; world creation, stepping, collision shape/constraint creation.
- Objects: `Base{Static,Dynamic,Ragdoll}Object.*` paired with `Bullet{Static,Dynamic,Ragdoll}Object.*`.
- Rigid bodies/constraints: `BasePhysicRigidBody.*` / `BasePhysicConstraint.*` paired with the `Bullet*RigidBody.*` / `Bullet*Constraint.*` implementations.
- Behaviors: `BasePhysicBehavior.*` / `BasePhysicComponentBehavior.*` / `BulletPhysicComponentBehavior.*`, plus Barnacle, Gargantua, buoyancy and camera behaviors (`Bullet*Behavior.*`).
- `src/Viewport.*`, `src/Physic*`, `src/AnimControl*`: VGUI2Extension debug/editing UI (`*Page`/`*Panel`/`*Dialog` files).
- `src/VGUI2ExtensionImport.*`, `src/BaseUI.cpp`, `src/GameUI.cpp`, `src/ClientVGUI.cpp`: VGUI2Extension dependency integration.
- `src/ClientEntityManager.*`, `src/PhysicUTIL.*`, `src/util.*`, `src/mathlib2.*`: utilities.
- `assets/svencoop/bulletphysics`, `assets/svencoop_downloads`: runtime resources.

BulletPhysics uses a layered structure: “MetaHook integration layer → common physics-management layer → Bullet backend → object/component layer.” Configurations and runtime objects are linked through the manager, while rendering hooks and physics stepping converge in the client main loop:

~~~mermaid
flowchart TD
    Loader["MetaHook loader / IPluginsV4"] --> Plugin["src/plugins.cpp"]
    Plugin --> EngineHooks["Engine hooks / privatehook.cpp"]
    Plugin --> ExportHooks["Client exports / exportfuncs.cpp"]
    Plugin --> Manager["IClientPhysicManager"]
    Manager --> BaseManager["CBasePhysicManager"]
    BaseManager --> BulletManager["CBulletPhysicManager"]
    BulletManager --> BulletWorld["btDiscreteDynamicsWorld"]
    ExportHooks --> EntityFrame["HUD_CreateEntities / HUD_TempEntUpdate"]
    EntityFrame --> ObjectFactory["CreatePhysicObjectFromConfig"]
    ObjectFactory --> ModelConfig["modelindex based config"]
    ModelConfig --> PhysicObject["Static / Dynamic / Ragdoll object"]
    PhysicObject --> Components["RigidBody + Constraint + Behavior"]
    Components --> BulletWorld
    BulletWorld --> BoneSync["StudioSetupBones / BoneMatrix sync"]
    BoneSync --> Renderer["GoldSrc Studio renderer"]
    VGUI["VGUI2Extension / PhysicDebugGUI"] --> Manager
~~~

### 1. Plugin Integration and Lifecycle
- IPluginsV4::Init only retains metahook_api_t, mh_interface_t, and mh_enginesave_t.
- LoadEngine acquires FileSystem, engine type/build number and the real engine base; registers the DllLoadNotification callback (used to initialize KeyValuesSystem once `vgui2.dll` is discovered); executes Engine_FillAddress/Engine_InstallHook; initializes VGUI2Extension and BaseUI/GameUI/ClientVGUI hooks; creates g_pClientPhysicManager = BulletPhysicManager_CreateInstance; and finally initializes GLEW.
- LoadClient preserves the original gExportfuncs, replaces HUD_Init, HUD_GetStudioModelInterface, HUD_CreateEntities, HUD_TempEntUpdate, HUD_AddEntity, HUD_DrawTransparentTriangles, HUD_Frame, HUD_Shutdown, V_CalcRefdef, and HUD_PostRunCmd, then performs client address resolution and hook installation.
- HUD_Init calls ClientPhysicManager()->Init after the original initialization, registers cvars such as bv_debug_draw*, bv_simrate, bv_syncview, and bv_force_updatebones, plus bv_open_debug_ui, bv_reload_*, and bv_save_configs commands, installs the ClCorpse message hook (if present) and an inline hook for efxapi_R_TempModel, and initializes g_pViewPort (if present).
- HUD_Shutdown first calls the original function, then shuts down the physics manager, Studio hooks, and temporary-model hooks; Shutdown unregisters the DLL notification callback; ExitGame destroys the manager, unloads UI/engine hooks, and shuts down VGUI2Extension.
- R_NewMap calls NewMap on the physics manager, entity manager, and debug viewport after the original map change completes.
- Engine_FillAddress locates engine functions/globals by category (rendering, view, temporary entities, visible entity list, and more). Engine_InstallHook installs inline hooks for R_NewMap and R_RenderView (SvEngine uses the R_RenderView_SvEngine branch). Client_FillAddress identifies dod/cstrike/czero from the game directory, sets g_bIsDayOfDefeat/g_bIsCounterStrike, and resolves additional addresses.

### 2. Common Manager and Data Model
- IClientPhysicManager defines internal contracts for lifecycle, configuration management, physics-object management, component management, world operations, bone bridging, Debug/Inspect/Select, and resource caches.
- CBasePhysicManager holds m_physicObjects, m_physicComponents, m_physicConfigs, m_physicObjectConfigs, external OBJ/BSP index caches, and debug-selection state. Object IDs use PACK_PHYSIC_OBJECT_ID(entindex, modelindex) to avoid retrieving old objects when entity numbers are reused.
- NewMap clears old physics objects, BSP-generated configurations, and BSP index caches; resets component/inspect selection IDs; regenerates brush vertex/index caches; loads known model configurations; and creates the static physics object for world brushes.
- Configurations are lazily loaded by modelindex: Studio models first read <model>_physics.txt and then support <model>_ragdoll.txt; brush models generate triangle-mesh static configurations from BSP. Configuration objects include rigid bodies, constraints, and behaviors; ragdolls additionally include animation control.
- CreatePhysicObjectForEntity dispatches Studio/brush creation based on model and entity type. Players, dead players, CS/CS:CZ corpses, and temporary entities use ClientEntityManager to resolve their actual model, player index, and scale, transferring old-object ownership when necessary.
- CreatePhysicObjectFromConfig creates the corresponding backend object, loads collision-shape external resources, and inserts it into m_physicObjects after Build succeeds; it destroys the temporary object if construction fails.
- UpdateAllPhysicObjects creates CPhysicObjectUpdateContext for every object and passes current gravity. Entities not emitted this frame are marked for release, while the others execute Update. StepSimulation then advances Bullet.

### 3. Object, Component, and Backend Layers
- CBaseStaticObject, CBaseDynamicObject, and CBaseRagdollObject handle common object state, configuration copying, Build/Rebuild, component containers, lifecycle updates, world addition/removal, and queries.
- DispatchBuildPhysicComponents proceeds in this order: create all rigid bodies; create non-DeferredCreate constraints; create behaviors; finally create DeferredCreate constraints. NonNative components are retained only for upper-level behavior/editing logic and do not directly create Bullet native objects.
- BulletStaticObject, BulletDynamicObject, and BulletRagdollObject implement backend factories, translating configurations into Bullet rigid bodies, constraints, and behaviors. Static objects primarily provide zero-mass colliders; dynamic/ragdoll objects support constraint solving, external forces, motion state, and behavior.
- The three main IPhysicComponent implementations are IPhysicRigidBody, IPhysicConstraint, and IPhysicBehavior. Components link both configurations and runtime instances via configId and a separate physicComponentId; the manager handles global registration and reclamation.
- CBulletPhysicManager::Init creates btDefaultCollisionConfiguration, dispatcher, DBVT broadphase, sequential impulse solver, and a custom CBulletDiscreteDynamicsWorld, sets the debug drawer and overlap filter callback, and initializes gravity to 0. Objects/components connect back to MetaHook objects through Bullet collision groups, user index/pointer, and motion state.

### 4. Frame Timing, Physics Stepping, and Queries
- HUD_CreateEntities enumerates active players and client edicts after original client entity creation (validating messagenum/EF_NODRAW/modelindex), marks entities emitted, and calls CreatePhysicObjectForEntity.
- HUD_TempEntUpdate processes temporary entities and creates any missing physics objects, updates gravity, temporarily drives view updates in Sven Co-op third person (wrapping a forced CAM_Think() + V_RenderView() with g_bIsUpdatingRefdef), then calls UpdateAllPhysicObjects and StepSimulation.
- CBulletPhysicManager::SetGravity converts GoldSrc gravity to the Bullet coordinate system; StepSimulation uses stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate()). GetSimulationTickRate is supplied by bv_simrate, with the common layer clamping it to 32–128.
- TraceLine converts GoldSrc rays to Bullet, uses filter groups to hit world, static/dynamic/ragdoll, constraints, or behaviors, then converts the hit entity index and component ID back to CPhysicTraceLineHitResult.
- HUD_DrawTransparentTriangles calls DebugDraw when AllowCheats and a debug cvar are enabled; the debug context controls visualization by object/component level, color, and inspect/selected state.

### 5. Studio Bone and Physics Synchronization
- HUD_GetStudioModelInterface caches engine_studio_api_s, gpStudioInterface, and pbonetransform/plighttransform, then resolves and installs EngineStudio/ClientStudio hooks.
- StudioDrawModel/Player use g_iRagdollRenderEntIndex and g_iRagdollRenderFlags to mark the entity owning the current SetupBones. STUDIO_RAGDOLL_SETUP_BONES lets the engine calculate animated bones only; STUDIO_RAGDOLL_UPDATE_BONES sends animation results to the physics side.
- StudioSetupBones_Template proceeds by first attempting physics-object SetupBones (skipping the original SetupBones when physics takes over), then calling the original SetupBones, and finally attempting SetupJiggleBones (kinematic/jiggle-bone synchronization).
- When building a ragdoll, SetupBonesForRagdoll samples animated bones and BulletCreateMotionState generates CBulletBoneMotionState. Dynamic rigid bodies update the bone matrix through setWorldTransform, and render hooks write it back to pbonetransform/plighttransform. The kinematic state instead writes the latest animated bones back into motion state.
- V_CalcRefdef delegates spectator/local-player camera synchronization to the physics object's CalcRefDef when not paused, in intermission, portal view, and other excluded states; bv_syncview determines the synchronization level.

### 6. Debug UI and Configuration Editing
- bv_open_debug_ui enters CViewport::OpenPhysicDebugGUI. CViewport creates CPhysicDebugGUI through VGUI2Extension and forwards state during NewMap, per-frame inspect updates, and UI lifecycle events.
- PhysicDebugGUI provides five inspect modes for entities/physics objects/rigid bodies/constraints/behaviors, selection colors, editing dialogs, configuration reload, and saving. Edits normally rebuild objects through RebuildPhysicObjectEx2 while retaining reusable component IDs.
- BaseUI/GameUI/ClientVGUI register callbacks through IVGUI2Extension_*Callbacks to cover the UI lifecycle, input, and window procedure.
- VGUI2ExtensionImport obtains IVGUI2Extension, DPI, Surface, Scheme, and Input interfaces from VGUI2Extension.dll, and initializes IKeyValuesSystem through DLL load notifications for configuration KeyValues and UI usage.
- A missing VGUI2Extension.dll skips UI callback registration; a present DLL with an incompatible interface factory or a missing required interface is fatal (Sys_Error).

## Involved Files & Symbols
- src/plugins.cpp - IPluginsV4::Init, LoadEngine, LoadClient, Shutdown, ExitGame; plugin lifecycle and hook installation order.
- src/exportfuncs.cpp - HUD_Init, HUD_GetStudioModelInterface, HUD_CreateEntities, HUD_TempEntUpdate, V_CalcRefdef, HUD_DrawTransparentTriangles, HUD_Shutdown; also message.cpp/parsemsg.cpp for entity-message parsing.
- src/privatehook.cpp/.h - engine/client address resolution; inline/vtable hooks for R_NewMap, R_RenderView (R_RenderView_SvEngine on SvEngine), Studio rendering, and StudioSetupBones.
- src/ClientPhysicManager.h - IClientPhysicManager, IPhysicObject, IPhysicComponent, rigid-body/constraint/behavior interfaces, and frame context.
- src/BasePhysicManager.h/.cpp - CBasePhysicManager; objects, components, configurations, resource caches, map switching, and common build/update flow.
- src/ClientPhysicConfig.h, src/ClientPhysicCommon.h - CClientPhysicObjectConfig, rigid-body/constraint/behavior/animation-control/collision-shape configurations, and modelindex storage.
- src/BulletPhysicManager.h/.cpp - CBulletPhysicManager; Bullet world, motion state, collision filtering, stepping, ray testing, and backend object factories.
- src/BaseStaticObject.*, BaseDynamicObject.*, BaseRagdollObject.* - backend-agnostic object lifecycles, component containers, animation activity state, and camera synchronization.
- src/BulletStaticObject.*, BulletDynamicObject.*, BulletRagdollObject.* - Bullet backend object implementations; create Bullet rigid bodies, constraints, and behaviors.
- src/BasePhysicRigidBody.*, BasePhysicConstraint.*, BasePhysicBehavior.*, BasePhysicComponentBehavior.* - backend-agnostic component base classes.
- src/BulletPhysicRigidBody.*, Bullet{Static,Dynamic,Ragdoll}RigidBody.* - rigid-body implementations; wrap btRigidBody and motion state.
- src/BulletPhysicConstraint.*, Bullet{Static,Dynamic,Ragdoll}Constraint.* - constraint implementations; wrap btTypedConstraint.
- src/BulletPhysicComponentBehavior.*, Bullet*Behavior.* - behavior implementations (Barnacle, Gargantua, buoyancy, relocation, first/third-person camera).
- src/ClientEntityManager.*, src/CounterStrike.* - entity types/indices, player-death state, CS/CS:CZ specifics, model scaling, current-frame emitted/visible state, and entity-model mapping.
- src/VGUI2ExtensionImport.*, Viewport.*, PhysicDebugGUI.*, PhysicEditorDialog.*, Physic*{Page,Panel,Dialog}.*, AnimControl*.*, BaseUI.cpp, GameUI.cpp, ClientVGUI.cpp - VGUI2Extension interfaces, debug viewport, inspect/select, and configuration editing UI.
- src/PhysicUTIL.*, src/util.*, src/mathlib2.* - configuration serialization, model-integrity validation, type/factor conversion, and other physics/math utilities.
- tests/ - configuration, shipped-asset and Bullet backend regression tests, enabled by BULLETPHYSICS_BUILD_TESTS; see [[build_and_verification]].
- CMakeLists.txt, cmake/Sources.cmake, cmake/Dependencies.cmake, cmake/VCLTL.cmake - Windows x86 C++20 DLL build, explicit SDK/plugin compile list and pinned dependencies.
- docs/en/features.md, docs/zh-CN/features.md - functionality and legacy ragdoll configuration; assets/svencoop/bulletphysics/* and assets/svencoop_downloads/* - UI/localization and physics runtime resources.

## Console Commands and cvars
- Commands: `bv_open_debug_ui`, `bv_reload_all`, `bv_reload_objects`, `bv_reload_configs`, `bv_save_configs`.
- cvars: `bv_debug_draw`, `bv_debug_draw_wallhack`, `bv_debug_draw_level_*`, `bv_debug_draw_*_color`, `bv_simrate`, `bv_syncview`, `bv_force_updatebones`.

## Dependencies and Boundaries
- MetaHook API and public interfaces: include/metahook.h, include/Interface/; used for plugin ABI, address resolution, inline/vtable hooks, engine type, and filesystem access.
- GoldSrc/SvEngine client and Studio interfaces: HLSDK cl_dll, engine, studio, and r_efx structures, plus engine_studio_api_s and cl_exportfuncs_t.
- Bullet3: btBulletDynamicsCommon and collision/ghost components; built statically from BULLET3_SOURCE_PATH or the pinned fork.
- VGUI2Extension: runtime VGUI2Extension.dll; provides IVGUI2Extension, DPI, Surface, Scheme, Input, and KeyValuesSystem.
- SourceSDK/KeyValues/FileSystem: configuration I/O, model-file access, and resource paths; uses IFileSystem_HL25 for HL25 compatibility.
- GLEW: static OpenGL extension loader for debug drawing; glewInit runs in LoadEngine.
- gamedata: all private symbols resolve through the host MetaHook API; the plugin ships a pruned nested catalog. No Capstone scanning dependency remains.
- tinyobjloader, ScopeExit, Chocobo1Hash: OBJ collision resources, scope cleanup, and hashing/integrity helpers; pinned submodules.
- Runtime resources: *_physics.txt/*_ragdoll.txt, BSP brush data, external .obj meshes, assets/svencoop/bulletphysics/*.res and localized text.
- The plugin only consumes MetaHook public API/HLSDK/SourceSDK/VGUI sources; it does not build the host. VGUI2Extension consumes public interface headers only, with the UI supplied at runtime. When no source path is provided, FetchContent is used — MetaHook from the latest `main`, the other dependencies at fixed commits.
- CMakeLists.txt builds Windows MSVC x86 Debug/Release with C++20, static CRT and VC-LTL. cmake/Sources.cmake preserves the 167-item compile list from the source vcxproj. VC-LTL uses the verified 5.3.1 package. SDL, Capstone, and FreeImage are not dependencies of this plugin.
- The plugin keeps IPluginsV4/CreateInterface and the original calling conventions; the physics interfaces are internal headers with no separate public SDK.
- Private symbols resolve through the host ResolveGameSymbol; consumption changes must be synced to the manifest. gamedata is pruned to `metahook/gamedata/bulletphysics` and merged by the host with the other catalogs.
- `g_ViewEntityIndex_SCClient` is optional at runtime (resolved with required=false); the manifest requires it only for 10257, and 8948 never had it. The existing three bone-sync usages all guard against null, which is the intended path for 8948. Both Sven versions still require the portal state and pitchdrift.

## Notes
- This plugin is a runtime system based on global pointers, global hooks, and one client physics manager. The source provides no cross-thread synchronization; physics objects, configurations, and render matrices should be accessed according to the client-hook main-thread ordering.
- Private symbols resolve against the matched binary identity through ResolveGameSymbol. Update the manifest with consumption changes; preserve calling conventions. The client view-entity slot is optional at runtime, published/required by the manifest only for 10257; 8948 leaves it null.
- m_physicObjects uses entity index as its primary key, while an entity can change models due to model switching, corpse transfer, or temporary-entity reuse. Cross-frame access should prioritize the packed physics object ID to validate modelindex.
- The current-frame emitted state determines whether an object survives. If entity enumeration/hook order changes, UpdateAllPhysicObjects can mistakenly consider an object expired and release it.
- NewMap clears BSP-origin objects, configurations, and index caches before rebuilding world collision; map switching and configuration reload must not interleave with active object updates.
- Excessively low or high bv_simrate is forcibly clamped to 32–128. Bullet fixed stepping uses at most 4 substeps; excessive frametime can still cause simulation error or CPU spikes.
- AllowCheats() uses the `allow_cheats` pointer under SvEngine and the `sv_cheats` cvar elsewhere; it gates debug commands, debug drawing, and configuration saving. When diagnosing configuration issues, confirm the sv_cheats/SvEngine allow_cheats path.
- If VGUI2Extension.dll is missing, the initialization function returns directly; if the DLL exists but its interface factory or a required interface is missing, it calls Sys_Error. The debug UI is not the sole dependency of core physics operation, but configuration editing depends on it.
- ClientPhysicManager.h still declares PhysXPhysicManager_CreateInstance, but no implementation was found within `src/`; LoadEngine currently always creates the Bullet backend, so do not infer that a switchable PhysX implementation exists.
- Configuration load/save and BoneMatrix synchronization have separate topics: [[physics_config]] and [[bonematrix_physics_sync]].

## Knowledge Entry Points
- [[physics_config]], [[bonematrix_physics_sync]]: configuration and bone synchronization.
- [[PrivateSymbols]]: current symbol inventory and retained historical scan records.
- [[build_and_verification]]: build entry points, verification evidence and unverified scope.
- [[CodeStyles]]: source conventions; when editing, follow the style of the specific file.

The MCP configuration uses the `bulletphysics` project name. Writes through MCP are only
possible when the project is actually bound to this directory's `memory/`; without such a
binding, read and write the local notes directly. The source project `metahooksv` is only
consulted as provenance.

## Callers (optional)
- IPluginsV4::LoadEngine -> BulletPhysicManager_CreateInstance
- HUD_Init -> IClientPhysicManager::Init
- R_NewMap -> IClientPhysicManager::NewMap
- HUD_CreateEntities -> CreatePhysicObjectForEntity
- HUD_TempEntUpdate -> UpdateAllPhysicObjects -> StepSimulation
- HUD_GetStudioModelInterface -> Studio/ClientStudio Hook installation and bone matrix capture
- StudioSetupBones Hook -> SetupBones / SetupJiggleBones
- V_CalcRefdef -> IPhysicObject::CalcRefDef
- bv_open_debug_ui -> CViewport::OpenPhysicDebugGUI
- bv_reload_configs / bv_save_configs -> configuration lifecycle methods

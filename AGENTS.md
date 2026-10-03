# AGENTS.md

## Knowledge and source entry points

- Start with `memory/project_overview.md`,
  `memory/build_and_verification.md`, `memory/PrivateSymbols.md` and `memory/CodeStyles.md`.
- Basic Memory is configured for project `bulletphysics`. Use MCP writes only when
  that project resolves to this repository's `memory/`; otherwise use local notes.
- Migrated notes use the `bulletphysics/` permalink prefix. PrivateSymbols lists only the
  gamedata symbols resolved today, mirroring `scripts/manifests/bulletphysics.json`.
- Build: `CMakeLists.txt`, `cmake/Sources.cmake`, `cmake/Dependencies.cmake`,
  `cmake/VCLTL.cmake`, `scripts/build-BulletPhysics-x86-{Debug,Release}.bat`.
- Plugin: `src/plugins.cpp`, `src/privatehook.cpp`, `src/exportfuncs.cpp`,
  `src/BasePhysicManager.*`, `src/BulletPhysicManager.*` and the VGUI editor classes.
- Assets: `assets/svencoop/bulletphysics` and the physics-only `assets/svencoop_downloads`.
- gamedata: `scripts/manifests/bulletphysics.json`, `scripts/sync-gamedata.py`,
  `scripts/validate-gamedata.py`. Update the manifest whenever consumption changes.
- Tests: `src/tests/client_gamedata_tests.cpp`, enabled by `BULLETPHYSICS_BUILD_TESTS`.
- Docs: bilingual README plus `docs/en` and `docs/zh-CN`. Project skills live in `.claude/skills`.

## Repository rules

- Preserve MetaHook plugin exports, ABI, calling conventions, physics behavior and resource formats.
  Follow the naming, indentation, encoding and comment style of each touched file.
- Resolve engine/client private symbols through the host gamedata contract. Required symbols
  stay fatal; intentionally optional virtuals and the Sven view-entity slot remain optional.
  8948 never publishes `g_ViewEntityIndex_SCClient`; its existing null guards are the intended path.
- External sources and third-party sources are read-only. MetaHook provides SDK sources,
  VGUI2Extension provides interface headers; this repository does not build those plugins.
- Use the normalized `BULLETPHYSICS_*_INCLUDE_DIRS` from dependency preparation for vendor headers.
- Preserve explicit compile lists and pinned dependency commits. Build output is under `build/`,
  install output under `install/`; neither is tracked or automatically deployed to a game.
- Tests keep assertions enabled in Release. Configuration/documentation text is not an assertion target.
- Report build, simulated test and real-game evidence separately. Documentation changes require
  content/path checks, not a DLL rebuild. Update build_and_verification with actual results.

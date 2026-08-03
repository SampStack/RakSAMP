# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: local implementation and consumer validation complete; remote build/release pending
- Outcome: release protocol-generic RakSAMP 0.10.0, prove it against both supported protocols in the
  consuming roleplay gamemode, then publish native/container artifacts
- Next action: commit, fast-forward `master`, push, and run the five-target build-only workflow
- Scope: client state convergence, checked protocol/config parsing, generic automation and load probes,
  development-server formatting safety, sanitizer coverage, documentation, and release gates
- Do not touch: gamemode-specific behavior in RakSAMP or immutable publication before consumer proof

## State

- Branch: `codex/reset-weapon-inventory-sync` from current `origin/master`
- Reproduction: after `/gun`, hospital recovery clears the authoritative loadout, but RakSAMP's empty
  `ID_WEAPONS_UPDATE` omits all slots and open.mp retains the prior weapon in `GetPlayerWeaponData`
- Decision: normal inventory updates remain sparse; reset updates explicitly serialize all 13 slots as zero
- Consumer evidence: roleplay 0.3DL reports `WeaponManipulation` about two seconds after discharge;
  command ordering is valid and the retained slot is the pre-recovery server-granted weapon
- Release decision: user selected immutable 0.10.0 and explicitly authorized direct push to `master`
- Hardening: checked readers now reject truncated/non-finite key RPC and sync fields; configuration parsing
  uses bounded values and atomic settings/rate commit; structured JSONL automation is opt-in and versioned
- Customization: load probes expose independent protocol fields with generic names; old anti-cheat/password
  option spellings remain compatibility aliases
- Safety: reachable unbounded formatting was replaced, TinyXML byte-color parsing was corrected, and
  invalid post-release RakNet page writes were removed

## Verification

- Passed before this slice: immutable `0.9.5` five-target release and prior consumer acceptance
- Passed: final 0.10.0 Debug client/server build, all 10 native tests, both sample configuration checks
- Passed: AddressSanitizer + UndefinedBehaviorSanitizer build, all 10 tests, and both config checks
- Passed: consuming roleplay hospital weapon-clear scenario on 0.3.7 and 0.3DL with anti-cheat enabled;
  each held a four-second negative window with no `WeaponManipulation` correction
- Passed with diagnostic fixture relaxations only: corrected local client completed the 0.3DL hospital recovery
  without a weapon correction; the fixture relaxations were reverted immediately
- Observed on 0.3.7 before a fixture-only assertion failed: discharge emitted no weapon correction during
  the four-second observation window; a clean final consumer rerun remains pending
- Failed then fixed: first client build exposed the C-array-to-`std::array` reset conversion;
  reset now uses `fill`, and the focused serializer test had already built successfully
- Not run: remote five-target native matrix, containers, immutable release verification

## Resume

- Files: completed uncommitted 0.10.0 hardening slice across client/server/common/tests/docs/workflows
- Commands: local sanitized build is `build-sanitized`; normal build is `build`
- Risks: legacy RakNet remains permissive internally; supported entry points are now checked and sanitizer
  gated, but the remote OS/architecture matrix and real consuming server remain the release authorities

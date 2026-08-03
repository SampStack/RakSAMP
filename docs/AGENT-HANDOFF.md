# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: immutable RakSAMP 0.11.0 is published from the exact commit that passed native, sanitizer,
  configuration, and two-protocol Roleplay consumer gates
- Outcome: publish RakSAMP 0.11.0 with instant `!driveposition` abuse probes plus bounded smooth
  `!driveto`, `!drivestatus`, and `!drivecancel` automation so consumers can distinguish gradual
  route evidence from instant checkpoint teleporting
- Next action: update the Roleplay dependency pin to 0.11.0 and rerun the focused consumer scenarios
  from the immutable release artifact
- Scope: generic finite/bounded driver position parsing, 100–60,000 ms interpolation, motion status
  and cancellation, assigned-driver enforcement, tests, docs, and the explicit 0.11.0 release
- Do not add: gamemode names, objectives, routes, fixture coordinates, or server-side bypasses

## Decisions

- Ordinary actions use the 16-bit sync mask; Yes, No, and CtrlBack use the protocol's two-bit
  additional-key field packed with the six-bit weapon id
- Held state is shared across on-foot, driver, and passenger sync and reset on disconnect
- Key commands force an immediate sync while normal periodic sync preserves held state
- Named controls and raw decimal/hex masks are supported; mutually exclusive additional keys are rejected
- The user explicitly authorized committing and pushing this completed slice directly to `master` and
  publishing immutable 0.10.1 after the Roleplay consumer gate
- The user explicitly requested immutable 0.11.0 after the new motion primitive passes the Roleplay
  consumer gate; no other version should be published

## Verification

- Passed 0.11.0: Release build and all 12 native tests; focused `drive-position` parser/interpolation
  coverage; both sample configuration checks; sanitizer build and all 12 tests
- Passed 0.11.0 consumer gate: Roleplay gradual two-player Forklift completion and exact payouts on 0.3.7
  and 0.3DL; instant checkpoint teleport rejection with unchanged balances on both protocols; rapid
  exit/re-entry retained one session; passenger competition did not acquire the leased job vehicle
- Passed: Release client/server build, all 11 native tests, and both sample configuration checks
- Passed: AddressSanitizer/UndefinedBehaviorSanitizer build and all 11 tests
- Passed: focused `key-state` test with named actions, aliases, raw masks, invalid combinations, and exact
  on-foot/driver/passenger additional-key packing, including legacy follow-state preservation
- Passed: build-only run 30837493479 on Linux x64/arm64, Windows x64, macOS x64/arm64, and sanitizers
- Passed: local Roleplay ATM interaction, withdrawal, deposit, and durable ledger consumer proof on
  both 0.3.7 and 0.3DL using the 0.10.1 native artifact
- Published: workflow 30837685687 reused the proven artifacts; tag `0.10.1` targets `32f592a`, all
  11 native archives plus `SHA256SUMS` are present, and the manifest hash is
  `2128b2529650afd79f4d62f01c56e608010e0638c8f3550ef525aefa50b6e97f`
- Published 0.11.0: PR #1 merged as `12f2a0b`; build-only workflow 30856578161 passed all five native
  platforms and sanitizers; publication workflow 30856761460 reused those exact artifacts; tag
  `0.11.0` targets `12f2a0b`; all 10 native archives plus `SHA256SUMS` are present; manifest hash is
  `df7e020559507fe7e697b7f3a6e2b2f4ac1c910a9c158b43713bf4ae0df60b7e`

## State

- RakSAMP `master` contains the 0.11.0 motion probes and immutable 0.11.0 is published
- Roleplay hardening changes remain local until its 0.11.0 pin and released-artifact gates pass

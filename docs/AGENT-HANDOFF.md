# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: generic key-state automation is published as immutable 0.10.1 and proven by Roleplay ATM flows
- Outcome: publish RakSAMP 0.10.1 with protocol-correct automation for every server-visible key action,
  then prove and pin it in the Roleplay ATM workflow
- Next action: none in RakSAMP; Roleplay is pinning the immutable release and completing stack acceptance
- Scope: `!key down|up`, on-foot/driver/passenger packing, aliases/raw masks, tests, docs, and 0.10.1 release
- Do not add: gamemode-specific commands or arbitrary OS scancodes

## Decisions

- Ordinary actions use the 16-bit sync mask; Yes, No, and CtrlBack use the protocol's two-bit
  additional-key field packed with the six-bit weapon id
- Held state is shared across on-foot, driver, and passenger sync and reset on disconnect
- Key commands force an immediate sync while normal periodic sync preserves held state
- Named controls and raw decimal/hex masks are supported; mutually exclusive additional keys are rejected
- The user explicitly authorized committing and pushing this completed slice directly to `master` and
  publishing immutable 0.10.1 after the Roleplay consumer gate

## Verification

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

## State

- Feature and portability fixes are pushed through `32f592a`; immutable 0.10.1 is published
- Roleplay implementation is complete and its dependency pin/full acceptance are in progress

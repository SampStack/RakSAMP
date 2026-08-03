# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: generic key-state automation implemented and locally validated; ready to commit and push
- Outcome: publish RakSAMP 0.10.1 with protocol-correct automation for every server-visible key action,
  then prove and pin it in the Roleplay ATM workflow
- Next action: commit and push `master`, then run the five-target build-only workflow before local
  Roleplay consumer validation
- Scope: `!key down|up`, on-foot/driver/passenger packing, aliases/raw masks, tests, docs, and 0.10.1 release
- Do not add: gamemode-specific commands, arbitrary OS scancodes, immutable publication before consumer proof

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
- Pending: remote five-target build-only workflow, Roleplay 0.3.7/0.3DL consumer proof, immutable release,
  and remote-artifact revalidation

## State

- Repository started clean on `master` aligned with `origin/master` at `a986602` / immutable 0.10.0
- Roleplay remains clean on `main`; its implementation has not started

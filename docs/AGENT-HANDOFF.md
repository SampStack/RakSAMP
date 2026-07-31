# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: exact-head build and both native gamemode consumer gates are green
- Outcome: five native targets and one linux/amd64 manifest per container
- Next action: refresh mutable `dev` without retired Windows arm64 assets
- Scope: release workflow and delivery documentation
- Do not touch: product behavior, native target matrix, releases, packages, or remote tags

## State

- Done: generic consumer and proportional validation rules are documented
- Done: removed arm64 container/QEMU work and `unknown/unknown` attestation manifests
- Done: exact-head five-target run 30661085491 passed at `2502706`
- Done: native macOS arm64 client passed focused acceptance against both gamemodes
- In progress: checksum-complete five-target `dev` refresh and artifact inspection
- Blocked: none
- Decisions: containers are linux/amd64 only; native targets are Linux x64/arm64, Windows x64, and
  macOS x64/arm64; no publication until consuming gamemode validation passes

## Verification

- Passed: prior v0.9.4 native matrix and product tests
- Passed: workflow YAML, skill validation, and whitespace checks for the x64-only policy
- Failed: none
- Passed: five native jobs, product tests, package staging, and archive upload
- Not run: refreshed `dev` publication after retired-asset removal

## Resume

- Files: `AGENTS.md`, this file, `.codex/hooks.json`, then files named by the active request
- Commands: use the `maintain-raksamp` skill to select focused validation
- Risks: consumer evidence can accidentally shape public APIs too narrowly

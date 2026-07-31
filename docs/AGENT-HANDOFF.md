# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: applying the five-target support boundary before a fresh GitHub build
- Outcome: five native targets and one linux/amd64 manifest per container
- Next action: push and require the exact-head five-target workflow to pass; do not publish
- Scope: release workflow and delivery documentation
- Do not touch: product behavior, native target matrix, releases, packages, or remote tags

## State

- Done: generic consumer and proportional validation rules are documented
- Done: removed arm64 container/QEMU work and `unknown/unknown` attestation manifests
- In progress: fresh five-target build and native gamemode consumer validation
- Blocked: none
- Decisions: containers are linux/amd64 only; native targets are Linux x64/arm64, Windows x64, and
  macOS x64/arm64; no publication until consuming gamemode validation passes

## Verification

- Passed: prior v0.9.4 native matrix and product tests
- Passed: workflow YAML, skill validation, and whitespace checks for the x64-only policy
- Failed: none
- Not run: fresh five-target workflow or publication; this workflow-only change does not alter runtime behavior

## Resume

- Files: `AGENTS.md`, this file, `.codex/hooks.json`, then files named by the active request
- Commands: use the `maintain-raksamp` skill to select focused validation
- Risks: consumer evidence can accidentally shape public APIs too narrowly

# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: setup ready
- Outcome: maintain generic RakSAMP capabilities without leaking consumer-specific behavior
- Next action: use a gamemode task or dedicated RakSAMP project to exercise the maintenance skill
- Scope: project guidance, recovery hook, and maintenance/release skill
- Do not touch: product behavior or remote state without a current task boundary

## State

- Done: generic consumer, proportional validation, and release-authority rules are documented
- In progress: trust and exercise the project hook when RakSAMP is primary
- Blocked: none
- Decisions: `dev` may publish when required; immutable versions require an explicit version

## Verification

- Passed: hook JSON and shell syntax, TOML parsing, skill validation, and whitespace checks
- Failed: none
- Not run: product tests; this setup does not change runtime behavior

## Resume

- Files: `AGENTS.md`, this file, `.codex/hooks.json`, then files named by the active request
- Commands: use the `maintain-raksamp` skill to select focused validation
- Risks: consumer evidence can accidentally shape public APIs too narrowly

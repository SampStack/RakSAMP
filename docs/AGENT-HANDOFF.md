# Agent handoff

> Replace stale entries; never append a diary. Keep this under 100 lines.
> Compaction is unannounced. Refresh at deterministic checkpoints; verify repository state on resume.

## Active slice

- Status: immutable `0.9.5` native set and both linux/amd64 images are published
- Outcome: five native targets and one linux/amd64 manifest per container
- Next action: confirm final immutable consumer re-pin
- Scope: release workflow and delivery documentation
- Do not touch: product behavior or native target matrix

## State

- Done: generic consumer and proportional validation rules are documented
- Done: removed arm64 container/QEMU work and `unknown/unknown` attestation manifests
- Done: exact-head five-target run 30661085491 passed at `2502706`
- Done: native macOS arm64 client passed focused acceptance against both gamemodes
- Done: run 30664811130 published ten archives without Windows arm64
- Done: client/server GHCR tags each contain one linux/amd64 image manifest
- Done: both gamemodes downloaded and ran the public macOS arm64 client from empty caches
- Done: relevant master pushes automatically replace checksum-complete `dev`
- Done: immutable run 30667483205 published ten native archives and both images
- Done: macOS packages include checksum-gated authorization helpers and ad-hoc signatures
- In progress: final immutable consumer re-pin
- Blocked: none
- Decisions: containers are linux/amd64 only; native targets are Linux x64/arm64, Windows x64, and
  macOS x64/arm64; no publication until consuming gamemode validation passes

## Verification

- Passed: prior v0.9.4 native matrix and product tests
- Passed: workflow YAML, skill validation, and whitespace checks for the x64-only policy
- Failed: none
- Passed: five native jobs, product tests, package staging, and archive upload
- Passed: immutable manifest `5c65fe5f...`; both GHCR tags are one linux/amd64 image
- Not run: post-release re-pin

## Resume

- Files: `AGENTS.md`, this file, `.codex/hooks.json`, then files named by the active request
- Commands: use the `maintain-raksamp` skill to select focused validation
- Risks: consumer evidence can accidentally shape public APIs too narrowly

# RakSAMP agent guidance

## Working style

- Make the smallest coherent change that satisfies a protocol-generic acceptance boundary.
  Preserve confirmed behavior and avoid speculative cleanup.
- RakSAMP is a public SA-MP/open.mp client and development-server tool. Never encode a gamemode's
  names, database schema, fixtures, commands, or flow. Add reusable protocol behavior or
  configurable primitives instead.
- Keep durable rules here and live task state in `docs/AGENT-HANDOFF.md`. Treat compaction as
  unannounced; refresh the handoff after decisions, before noisy work, and after material tests.

## Work loop

1. Read this file, `docs/AGENT-HANDOFF.md`, and the `maintain-raksamp` skill; inspect the worktree.
2. Trace only the consumer failure and the owning client, server, common, configuration, or test code.
3. Implement the root capability with fixed-width, protocol-correct wire behavior and bounded input.
4. Run the smallest focused native test, then the consuming gamemode scenario when applicable.
5. Report passed, failed, and not-run checks separately. Use a branch and pull request for future
   work; verify the merged revision on `origin/master`.

## Consumer repositories

- Gamemodes may expose genuine emulation gaps. Treat their behavior as acceptance evidence,
  not as an excuse for consumer-specific implementation.
- When invoked from a gamemode project, use explicit `git -C` commands and keep commits, status,
  build outputs, and handoffs separate for each repository.
- After a RakSAMP fix, validate locally before running the narrowest consumer scenario. Do not hide
  missing rendering, clicks, spectator transitions, or protocol messages with sleeps or weaker tests.

## Validation and release

- Follow `docs/development.md` and the `maintain-raksamp` skill. Use focused `ctest -R` first;
  reserve `make test`, image builds, and multi-platform publication for their actual boundary.
- Platform or delivery changes require the five-target build-only workflow for the exact revision.
- Relevant merges to `master` replace `dev`; pull requests never publish. Immutable versions require
  an explicit three-part version and manual dispatch.
- Native archives cover Linux x64/arm64, Windows x64, and macOS x64/arm64. GHCR client/server images are linux/amd64 only and disable
  provenance/SBOM attestations so package metadata contains no `unknown/unknown` platform.
- After container-heavy work, inspect `docker system df`. Use the safe `docker-clean` skill only
  when it shows meaningful reclaimable residue. Deep cleanup and volume deletion require explicit
  authorization.

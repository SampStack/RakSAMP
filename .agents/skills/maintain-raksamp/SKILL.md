---
name: maintain-raksamp
description: Implement, validate, and release generic RakSAMP client, server, protocol, configuration, load, or probe capabilities. Use for direct RakSAMP maintenance and when an attached gamemode reveals a fake-client emulation gap that belongs in the public tool; also use for validated master pushes, mutable dev publication, or explicitly versioned releases.
---

# Maintain RakSAMP

Keep RakSAMP a reusable public SA-MP/open.mp protocol tool while satisfying real consumer needs.

## Workflow

1. Define one protocol-generic acceptance boundary. Inspect the consumer failure and the owning
   RakSAMP client, server, common protocol, configuration, or test code.
2. Implement the smallest reusable capability. Prefer configurable primitives and protocol-correct
   behavior; never add gamemode names, schemas, command scripts, or hard-coded workflows.
3. Validate proportionally:
   - Focused native logic: configure once, build, then run
     `ctest --test-dir build -C Release -R <pattern> --output-on-failure`.
   - Broad protocol or release change: run `make test` and both sample `--check-config` commands
     from `docs/development.md`.
   - Gamemode-driven gap: run its narrowest consuming scenario after RakSAMP passes locally.
   - Container delivery: use `make image` only when the image boundary changed or publication is
     being accepted.
   - Platform/delivery: require all five build-only CI jobs for the exact pushed revision.
4. Update `docs/AGENT-HANDOFF.md`; commit the completed slice on a branch using the established
   conventional style and merge it through a pull request.
5. Do not publish an immutable version until consuming gamemodes pass their applicable native
   validation; then watch the requested workflow and verify its artifacts.

## Release commands

A relevant merge to `master` automatically replaces the mutable `dev` release. Use an immutable
version only when the user supplies it and consumer validation is green:

```bash
gh workflow run publish-containers.yml \
  --ref master \
  -f source_ref=master \
  -f release_version=X.Y.Z \
  -f push_dev=false \
  -f native_consumers_green=true
```

Resolve the run with `gh run list --workflow publish-containers.yml --limit 1`, then use
`gh run watch <run-id> --exit-status`. Protected-environment approval may still require the user.

## Safety

- Confirm `master` is current, tests passed, and the intended commit is pushed before publishing.
- Published client/server containers are linux/amd64 only; keep the five native targets separate.
- `dev` is mutable. Versioned tags and releases are immutable; never invent a version.
- Inspect `docker system df` after container-heavy work. Use the safe `docker-clean` workflow only
  for meaningful reclaimable residue; never delete volumes or deep-clean without authorization.

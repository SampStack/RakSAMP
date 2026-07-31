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
4. Update `docs/AGENT-HANDOFF.md`; commit the completed slice on `master` using the established
   conventional style.
5. Push `master` and publish `dev` when the current task requires consumers to receive the validated
   change. Watch the workflow and verify the requested artifact.

## Release commands

Publish the mutable development release from reviewed `master`:

```bash
gh workflow run publish-containers.yml \
  --ref master \
  -f source_ref=master \
  -f release_version= \
  -f push_dev=true
```

Use an immutable version only when the user supplies it:

```bash
gh workflow run publish-containers.yml \
  --ref master \
  -f source_ref=master \
  -f release_version=vX.Y.Z \
  -f push_dev=true
```

Resolve the run with `gh run list --workflow publish-containers.yml --limit 1`, then use
`gh run watch <run-id> --exit-status`. Protected-environment approval may still require the user.

## Safety

- Confirm `master` is current, tests passed, and the intended commit is pushed before publishing.
- `dev` is mutable. Versioned tags and releases are immutable; never invent a version.
- Inspect `docker system df` after container-heavy work. Use the safe `docker-clean` workflow only
  for meaningful reclaimable residue; never delete volumes or deep-clean without authorization.

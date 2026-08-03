# Releasing

Pull requests build and test without publishing. Pushes and merges do not run release jobs. The
single manual workflow builds once and refreshes mutable `dev` by default; immutable versions are
optional and explicit.

For an immutable release, first accept the current `dev` assets on their supported platforms.

1. Open **Actions → Build and publish releases → Run workflow**.
2. Set `source_ref` to the reviewed branch, tag, or commit. It defaults to `master`.
3. Leave `push_dev` enabled to refresh the rolling build, or clear it if only an immutable release is wanted.
4. Optionally enter an intentional immutable three-part version such as `0.10.0`.
5. Select `build_only` to retain artifacts without publishing anything.
6. Confirm consuming gamemodes passed native validation for an immutable release.
7. Review the validation and five native build jobs, then approve `ghcr-publish` when publishing.

To publish a successful build-only run later, repeat the dispatch with the same `source_ref` and set
`artifact_run_id` to its run ID. Native compilation is skipped and the retained artifacts are
published. Set the approved `release_version` and validation confirmation when making it immutable.

## Development builds

`dev` is deliberately mutable. A successful manual publication:

- moves the `dev` tag to the selected revision;
- replaces the assets on the public `dev` GitHub prerelease;
- replaces the public `dev` client and server container manifests.

It does not publish `latest`. Pull requests never publish.

## Versioned builds

Any non-empty `release_version` creates a new, non-prerelease GitHub Release and matching container
tags. Versioned tags are immutable: the workflow fails if the tag already exists.

Use an intentional version such as `0.10.0` only after accepting the `dev` build on the supported platforms.

## Published artifacts

Each release provides separate client and server native archives for:

| Platform | Architecture | Format |
|---|---|---|
| Linux | x64, ARM64 | `.tar.gz` |
| Windows | x64 | `.zip` |
| macOS | Intel x64, Apple Silicon ARM64 | `.tar.gz` |

The archives include the executable, sample configuration, README, and third-party notices. Server archives also include the example `scripts/` directory. `SHA256SUMS` covers every archive. Linux builds target Ubuntu 22.04-era system libraries, macOS builds target macOS 13+, and Windows builds use the static MSVC runtime.

The workflow also publishes:

- `ghcr.io/sampstack/raksamp-client:<image_tag>`
- `ghcr.io/sampstack/raksamp-server:<image_tag>`

Each container tag has one `linux/amd64` manifest. Provenance and SBOM attestations are disabled so
GHCR does not display a synthetic `unknown/unknown` platform. Both packages are public and can be
pulled without authentication; publishing still requires the workflow's scoped GitHub token and
protected environment approval.

Before applying a durable version, test the native archives and pull both development images on
Linux x64. Apple Silicon uses Docker's amd64 emulation only when a container test is necessary:

```bash
docker pull --platform linux/amd64 ghcr.io/sampstack/raksamp-client:dev
docker pull --platform linux/amd64 ghcr.io/sampstack/raksamp-server:dev
```

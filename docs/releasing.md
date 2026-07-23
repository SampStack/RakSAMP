# Releasing

Publication is intentionally manual.

1. Open **Actions → Build and publish releases → Run workflow**.
2. Set `source_ref` to the reviewed branch, tag, or commit. It defaults to `master`.
3. Leave `image_tag=dev` for the rolling development release, or enter an intentional version.
4. Review the validation and six native build jobs.
5. Approve the protected `ghcr-publish` environment.

## Development builds

`dev` is deliberately mutable. A successful manual run:

- moves the `dev` tag to the selected revision;
- replaces the assets on the public `dev` GitHub prerelease;
- replaces the public `dev` client and server container manifests.

It does not publish `latest` and is never triggered by a push or pull request.

## Versioned builds

Any `image_tag` other than `dev` creates a new, non-prerelease GitHub Release and matching container tags. Versioned tags are immutable: the workflow fails if the tag already exists.

Use an intentional version such as `v0.9.0` only after accepting the `dev` build on the supported platforms.

## Published artifacts

Each release provides separate client and server native archives for:

| Platform | Architecture | Format |
|---|---|---|
| Linux | x64, ARM64 | `.tar.gz` |
| Windows | x64, ARM64 | `.zip` |
| macOS | Intel x64, Apple Silicon ARM64 | `.tar.gz` |

The archives include the executable, sample configuration, README, and third-party notices. Server archives also include the example `scripts/` directory. `SHA256SUMS` covers every archive. Linux builds target Ubuntu 22.04-era system libraries, macOS builds target macOS 13+, and Windows builds use the static MSVC runtime.

The workflow also publishes:

- `ghcr.io/sampstack/raksamp-client:<image_tag>`
- `ghcr.io/sampstack/raksamp-server:<image_tag>`

Each container tag is a `linux/amd64` and `linux/arm64` manifest. Both packages are public and can be pulled without authentication; publishing still requires the workflow's scoped GitHub token and protected environment approval.

Before applying a durable version, test the native archives and pull both development images on Linux x64 and Apple Silicon:

```bash
docker pull --platform linux/amd64 ghcr.io/sampstack/raksamp-client:dev
docker pull --platform linux/arm64 ghcr.io/sampstack/raksamp-server:dev
```

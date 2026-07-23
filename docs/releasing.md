# Releasing

Container publication is intentionally manual.

1. Open **Actions → Build and publish containers → Run workflow**.
2. Set `source_ref` to the reviewed branch, tag, or commit. It defaults to `master`.
3. Leave `image_tag=dev` for a mutable development build, or enter an intentional version.
4. Review the validation job.
5. Approve the protected `ghcr-publish` environment.

The workflow publishes:

- `ghcr.io/sampstack/raksamp-client:<image_tag>`
- `ghcr.io/sampstack/raksamp-server:<image_tag>`

Each tag is a `linux/amd64` and `linux/arm64` manifest. The workflow does not create Git tags, GitHub Releases, or `latest`.

Configure `ghcr-publish` as a GitHub Environment with required reviewers. Both packages are public so developers and CI systems can pull them without authenticating; publishing still requires the workflow's scoped GitHub token and environment approval.

Before applying a durable tag, pull and run both images on Linux x64 and Apple Silicon:

```bash
docker pull --platform linux/amd64 ghcr.io/sampstack/raksamp-client:dev
docker pull --platform linux/arm64 ghcr.io/sampstack/raksamp-server:dev
```

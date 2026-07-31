#!/usr/bin/env bash
set -euo pipefail

package_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
manifest="$package_dir/macos-runtime.sha256"

if [[ $(uname -s) != Darwin ]]; then
  echo "This helper is only for a macOS RakSAMP package." >&2
  exit 1
fi

if [[ ${1:-} != --trust ]]; then
  echo "Usage: bash ./authorize-macos-runtime.sh --trust" >&2
  echo "This verifies the packaged executable, then removes quarantine only from this package." >&2
  exit 2
fi

if [[ ! -f $manifest ]]; then
  echo "Missing macos-runtime.sha256; refusing to change quarantine attributes." >&2
  exit 1
fi

(
  cd -- "$package_dir"
  shasum -a 256 --check macos-runtime.sha256
)

verified=0
while IFS= read -r -d '' candidate; do
  if file -b "$candidate" | grep -q 'Mach-O'; then
    codesign --verify --strict "$candidate"
    verified=$((verified + 1))
  fi
done < <(find "$package_dir" -type f -print0)

if (( verified != 1 )); then
  echo "Expected one signed Mach-O executable, found $verified; refusing authorization." >&2
  exit 1
fi

xattr -dr com.apple.quarantine "$package_dir"
echo "Authorized one signed RakSAMP executable under: $package_dir"

#!/usr/bin/env bash
set -euo pipefail

repo_root=$(git rev-parse --show-toplevel)
handoff="$repo_root/docs/AGENT-HANDOFF.md"

printf '%s\n' \
  'RakSAMP compaction recovery: the handoff may lag the interrupted operation.' \
  'Read it, then inspect repository state before repeating work or trusting old claims.' \
  '--- AGENT-HANDOFF.md ---'

if [[ -f "$handoff" ]]; then
  sed -n '1,100p' "$handoff"
else
  printf '%s\n' 'Handoff missing. Reconstruct state from the request, git status, and focused evidence.'
fi

#!/usr/bin/env bash
set -eu

dir="$(cd "$(dirname "$0")" && pwd)"
binary="${dir}/build/mini-container"

if [ "$(uname -s)" != "Linux" ]; then
  printf 'mini-container smoke test skipped: Linux required\n'
  exit 0
fi

if [ ! -x "${binary}" ]; then
  printf 'missing executable: %s\n' "${binary}" >&2
  exit 1
fi

"${binary}" --help >/dev/null

if [ "$(id -u)" != "0" ]; then
  printf 'mini-container root smoke test skipped: root required\n'
  exit 0
fi

"${binary}" --hostname mini-container-smoke / /bin/true
printf 'mini-container smoke test passed\n'

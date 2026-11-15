#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITARM="$DEVKITPRO/devkitARM"
export PATH="$DEVKITARM/bin:$PATH"
make -C "$task_root" -j"${JOBS:-6}" "PYTHON=${PYTHON:-python3}" "$@"

#!/usr/bin/env bash
set -euo pipefail

ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
cd "$ROOT"

if ! command -v semgrep >/dev/null 2>&1; then
    echo "semgrep is not installed" >&2
    exit 127
fi

CONFIG=${SEMGREP_CONFIG:-"$ROOT/.semgrep.yml"}
TIMEOUT_SECONDS=${SEMGREP_TIMEOUT_SECONDS:-45}

if [ ! -f "$CONFIG" ]; then
    echo "semgrep config not found: $CONFIG" >&2
    exit 2
fi

if [ "$#" -eq 0 ]; then
    mapfile -t TARGETS < <(git ls-files '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx')
else
    TARGETS=("$@")
fi

if [ "${#TARGETS[@]}" -eq 0 ]; then
    echo "No C/C++ files to scan."
    exit 0
fi

if command -v timeout >/dev/null 2>&1; then
    timeout "${TIMEOUT_SECONDS}s" semgrep --config "$CONFIG" --error "${TARGETS[@]}"
else
    semgrep --config "$CONFIG" --error "${TARGETS[@]}"
fi

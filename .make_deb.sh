#!/usr/bin/env bash
set -e

CURR_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

PKGS=(
  ctilog
)

for pkg in "${PKGS[@]}"; do
    cd ${CURR_DIR}/${pkg}
    bash .make_deb.sh
done
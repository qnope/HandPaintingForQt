#!/usr/bin/env bash
# Installs what building and testing need on Ubuntu 24.04, from the distribution packages only:
# compiler, CMake, Ninja, Qt 6.4 and GoogleTest. No vcpkg nor Qt installer is involved.
# Used by the "Linux (gcc, Qt 6.4)" CI job and by the cloud sessions of Claude.
set -euo pipefail

if [ "$(id -u)" -eq 0 ]; then
  as_root=()
else
  as_root=(sudo)
fi

"${as_root[@]}" apt-get update
"${as_root[@]}" env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build \
  qt6-base-dev libgl1-mesa-dev libxkbcommon-dev \
  libgtest-dev libgmock-dev \
  fonts-dejavu-core

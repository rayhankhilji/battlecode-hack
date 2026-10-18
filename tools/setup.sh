#!/usr/bin/env bash
# One-shot environment setup for this repo (Linux x86_64).
# Installs the cambc CLI into a venv, refreshes the SDK + map pool, and pre-seeds the
# toolchain cache (wasi-sdk, cmake, ninja) with curl so builds work behind TLS-intercepting proxies.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENV="${VENV:-$ROOT/.venv}"
python3 -m venv "$VENV"
"$VENV/bin/pip" install -q --upgrade "cambc==2.2.5"
export PATH="$VENV/bin:$PATH"
CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/cambc"
mkdir -p "$CACHE"
fetch() { # url dest-dir strip
  local url="$1" dest="$2" strip="${3:-1}"
  if [ ! -d "$dest" ]; then
    echo "fetching $url"; tmp="$(mktemp)"; curl -sSL --retry 3 -o "$tmp" "$url"
    mkdir -p "$dest"; tar -xzf "$tmp" --strip-components="$strip" -C "$dest"; rm -f "$tmp"
  fi
}
fetch https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-33/wasi-sdk-33.0-x86_64-linux.tar.gz "$CACHE/wasi-sdk-33"
fetch https://github.com/Kitware/CMake/releases/download/v3.31.8/cmake-3.31.8-linux-x86_64.tar.gz "$CACHE/cmake-3.31.8-linux-x86_64"
if [ ! -x "$CACHE/ninja-1.12.1-linux-x86_64/ninja" ]; then
  tmp="$(mktemp)"; curl -sSL --retry 3 -o "$tmp" https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip
  mkdir -p "$CACHE/ninja-1.12.1-linux-x86_64"; python3 -c "import sys,zipfile;zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$tmp" "$CACHE/ninja-1.12.1-linux-x86_64"
  chmod +x "$CACHE/ninja-1.12.1-linux-x86_64/ninja"; rm -f "$tmp"
fi
cd "$ROOT" && cambc init --yes . && cambc maps pull
echo "ok: run 'source $VENV/bin/activate' then 'cambc build bots/apex --release'"

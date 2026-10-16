#!/usr/bin/env bash
# new-project.sh <name> [dest-dir]
# Copy the hello_amoled template into a new ESP-IDF project for the
# Waveshare ESP32-S3-Touch-AMOLED-1.8.
set -euo pipefail

name="${1:-}"
dest="${2:-.}"
[[ -n "$name" ]] || { echo "usage: $0 <name> [dest-dir]" >&2; exit 2; }
[[ "$name" =~ ^[A-Za-z][A-Za-z0-9_]*$ ]] || { echo "name must be letters, digits and _ (C identifier style)" >&2; exit 2; }

here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target="$dest/$name"
[[ -e "$target" ]] && { echo "$target already exists" >&2; exit 1; }

mkdir -p "$dest"
cp -R "$here/templates/hello_amoled" "$target"
sed -i.bak "s/project(hello_amoled)/project($name)/" "$target/CMakeLists.txt" && rm -f "$target/CMakeLists.txt.bak"

cat <<EOF
Created $target
Next:
  cd $target
  idf.py set-target esp32s3
  idf.py build
  idf.py -p \$($here/scripts/find-port.sh) flash monitor
EOF

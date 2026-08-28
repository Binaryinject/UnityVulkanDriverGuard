#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_root/build/linux-x86_64"
output_dir="$repo_root/Native~/Linux/x86_64"
cmake -S "$repo_root" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure
mkdir -p "$output_dir"
cp "$build_dir/UnityPlayer.so" "$output_dir/UnityPlayer.so"
echo "Built $output_dir/UnityPlayer.so"

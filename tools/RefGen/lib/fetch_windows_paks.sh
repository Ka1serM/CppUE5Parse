#!/usr/bin/env bash
# Copies the Windows cooks and their reference values from the VM (ssh host "windows") into a CppUE5Parse checkout:
#   C:\RefGenWin\paks\Windows[-Vulkan]\<x.y.z>*      -> tests/Paks/Windows[-Vulkan]/
#   C:\RefGenWin\results\manifest\UE_<x.y>.json      -> tests/Paks/Windows[-Vulkan]/<x.y.z>.json (named after the pak)
# usage: fetch_windows_paks.sh <CppUE5Parse checkout>
set -euo pipefail
repo=${1:?usage: fetch_windows_paks.sh <CppUE5Parse checkout>}
manifests=$(mktemp -d)
trap 'rm -rf "$manifests"' EXIT
scp -q 'windows:C:/RefGenWin/results/manifest/UE_*.json' "$manifests/"
for flavor in Windows Windows-Vulkan; do
    out=$repo/tests/Paks/$flavor
    mkdir -p "$out"
    scp -q "windows:C:/RefGenWin/paks/$flavor/*" "$out/" || continue
    for pak in "$out"/*.pak; do
        version=$(basename "$pak" .pak)
        version=${version%-inline}
        minor=${version%.*}
        if [[ -f $manifests/UE_$minor.json ]]; then
            cp "$manifests/UE_$minor.json" "$out/$version.json"
        else
            echo "no manifest for $flavor $version" >&2
        fi
    done
done

#!/usr/bin/env bash
# check that a PKGBUILD's remote sources still download and match their
# sha256sums, without makepkg (ci runs on ubuntu). local sources are skipped.
set -euo pipefail
cd "$(dirname "$0")"

# shellcheck disable=SC1091
source ./PKGBUILD

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# source and sha256sums come from the sourced PKGBUILD
# shellcheck disable=SC2154
for i in "${!source[@]}"; do
    case "${source[$i]}" in
        *::*) ;;
        *) continue ;;
    esac
    name=${source[$i]%%::*}
    url=${source[$i]#*::}
    echo "checking $name"
    curl -fsSL "$url" -o "$tmp/$name"
    echo "${sha256sums[$i]}  $tmp/$name" | sha256sum -c --status -
done
echo "all remote sources match"

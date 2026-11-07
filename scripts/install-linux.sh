#!/usr/bin/env bash
# install dropship on linux and, unless told not to, the netscope daemon it
# shares the game's scope with. root is only needed for the netscope half.
set -euo pipefail

NETSCOPE_VERSION="0.2.1"
DROPSHIP_REPO="${DROPSHIP_REPO:-Qiiks/dropship}"
NETSCOPE_REPO="${NETSCOPE_REPO:-Qiiks/netscope}"

PREFIX="${PREFIX:-$HOME/.local}"
DRY_RUN=0
RELEASE=0
WITH_NETSCOPE=1
BINARY=""

usage() {
    cat <<'EOF'
usage: install-linux.sh [options] [binary]

  binary            dropship binary to install (default: target/release/dropship,
                    or the latest release asset with --release)
  --release         download the latest dropship linux release instead
  --no-netscope     skip the netscope install
  --dry-run         print what would happen; netscope's installer runs as a dry run
  -h, --help        this text
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --release) RELEASE=1 ;;
        --no-netscope) WITH_NETSCOPE=0 ;;
        --dry-run) DRY_RUN=1 ;;
        -h|--help) usage; exit 0 ;;
        -*) echo "install-linux.sh: unknown option $1" >&2; usage >&2; exit 2 ;;
        *) BINARY="$1" ;;
    esac
    shift
done

fail() {
    echo "install-linux.sh: $*" >&2
    exit 1
}

run() {
    if [ "$DRY_RUN" = 1 ]; then
        printf 'would: %s\n' "$*"
    else
        "$@"
    fi
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if [ "$RELEASE" = 1 ]; then
    url="https://github.com/$DROPSHIP_REPO/releases/latest/download/dropship-linux-x86_64"
    echo "[+] downloading $url"
    curl -fsSL "$url" -o "$tmp/dropship"
    BINARY="$tmp/dropship"
fi

[ -n "$BINARY" ] || BINARY="target/release/dropship"
[ -f "$BINARY" ] || fail "$BINARY not found; build it or pass --release"
[ "$(uname -m)" = "x86_64" ] || fail "dropship ships x86_64 only (found $(uname -m))"

dest="$PREFIX/bin/dropship"
if [ -e "$dest" ] && [ "$(realpath "$BINARY")" = "$(realpath "$dest")" ]; then
    echo "[ok] dropship already installed at $dest"
else
    run install -Dm755 "$BINARY" "$dest"
    echo "[ok] dropship installed to $dest"
fi

if [ "$WITH_NETSCOPE" = 1 ]; then
    command -v systemctl >/dev/null || fail "systemd is required for netscope"

    if command -v netscoped >/dev/null; then
        echo "[ok] netscope already installed: $(netscope --version 2>/dev/null || echo unknown version)"

        if ! systemctl is-active --quiet netscoped.service; then
            run sudo systemctl enable --now netscoped.service
        fi
    else
        tar_name="netscope-v$NETSCOPE_VERSION-x86_64-linux.tar.zst"
        base="https://github.com/$NETSCOPE_REPO/releases/download/v$NETSCOPE_VERSION"

        echo "[+] netscope is missing; installing $NETSCOPE_VERSION"
        curl -fsSL "$base/$tar_name" -o "$tmp/$tar_name"
        curl -fsSL "$base/SHA256SUMS" -o "$tmp/SHA256SUMS"

        expected=$(awk -v name="$tar_name" '$2 == name { print $1 }' "$tmp/SHA256SUMS")
        [ -n "$expected" ] || fail "$tar_name is not listed in SHA256SUMS"
        actual=$(sha256sum "$tmp/$tar_name" | awk '{ print $1 }')
        [ "$expected" = "$actual" ] || fail "checksum mismatch for $tar_name"

        tar --zstd -xf "$tmp/$tar_name" -C "$tmp"
        run sudo "${tmp}/netscope-v$NETSCOPE_VERSION-x86_64-linux/install.sh"
    fi
fi

echo
echo "installed. launch dropship once to write its desktop entry."
if [ "$WITH_NETSCOPE" = 1 ]; then
    echo "netscope owns the game's shared scope; dropship joins it and blocks only the game."
fi

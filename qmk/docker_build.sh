#!/bin/sh
set -eu

dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
keymap=${1:-all}

if [ -z "${RUNTIME:-}" ]; then
    if command -v docker >/dev/null 2>&1; then
        RUNTIME=docker
    elif command -v podman >/dev/null 2>&1; then
        RUNTIME=podman
    else
        echo 'Docker or Podman is required.' >&2
        exit 2
    fi
fi

if [ -n "${CORNE_MINIEC_BUILDER_IMAGE:-}" ]; then
    image=$CORNE_MINIEC_BUILDER_IMAGE
    "$RUNTIME" pull "$image"
else
    image=corne-miniec-vial
    "$RUNTIME" build --tag "$image" "$dir"
fi

mkdir -p "$dir/build"
set --
if [ "$RUNTIME" = docker ]; then
    set -- --user "$(id -u):$(id -g)"
fi

"$RUNTIME" run --rm "$@" \
    -w /home/vscode/vial-qmk \
    -v "$dir/keyboards:/home/vscode/vial-qmk/keyboards:ro" \
    -v "$dir/build:/home/vscode/vial-qmk/.build" \
    -e HOME=/tmp \
    -e SKIP_GIT=yes \
    "$image" make "goropikari/corne_miniec:$keymap"

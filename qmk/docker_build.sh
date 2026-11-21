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

# Use distinct QMK targets so left and right builds also have distinct object files.
for hand in left right; do
    handedness=$(printf '%s' "$hand" | tr '[:lower:]' '[:upper:]')
    "$RUNTIME" run --rm "$@" \
        -w /opt/vial-qmk \
        -v "$dir/keyboards:/opt/vial-qmk/keyboards:ro" \
        -v "$dir/build:/opt/vial-qmk/.build" \
        -e HOME=/tmp \
        -e SKIP_GIT=yes \
        "$image" make "goropikari/corne_miniec:$keymap" \
        "EXTRAFLAGS=-DINIT_EE_HANDS_$handedness" \
        "TARGET=\$(KEYBOARD_FILESAFE)_\$(KEYMAP)_$hand"
done

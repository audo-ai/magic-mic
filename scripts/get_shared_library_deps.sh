#!/bin/bash

# https://stackoverflow.com/questions/1527049/how-can-i-join-elements-of-an-array-in-bash
join_by() { local IFS="$1"; shift; echo "$*"; }

EXE=$1
if ! test -x "$EXE"; then
    echo "\"$EXE\" Does not exist or is not executable!"
    exit 1
fi
DEST=$2
if ! test -d "$DEST"; then
    echo "\"$DEST\" is not a directory!"
    exit 2
fi
shift 2

IFS=',' read -d '' -r -a TARGETS <<< "$@"
#printf "Targets: %s\n" "${TARGETS[@]}"

REGEX=$(join_by '|' "${TARGETS[@]}")
IFS=$'\n' read -d '' -r -a LIBS <<< "$(ldd "$EXE" | sed -E -e "s/^.*($REGEX).*=> (\/[^ ]*).*$/\2/;t;d")"

if ! test ${#LIBS[@]} -eq ${#TARGETS[@]}; then
    echo "Not enough libs found in executable"
    printf 'Found: %s\n' "${LIBS[@]}"
    echo ${#TARGETS[@]}
    echo ${#LIBS[@]}
    printf 'Requested: %s\n' "$@"
    exit 3
fi

for LIB in "${LIBS[@]}"; do
    cp "$LIB" "$DEST"
done

#!/bin/sh
# For now tauri seems to run commands very stupidly (ie. doesn't pass to shell,
# just splits on spaces and execs). So we do this
cd ./src-web && npx --no-install yarn && npx --no-install yarn build

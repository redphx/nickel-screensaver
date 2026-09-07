#!/usr/bin/env bash

set -euo pipefail

image="${NICKELTC_IMAGE:-strayrose/nickeltc@sha256:9e948a9593b5500dba99e30866575712ee2d72c02ccada96fafadda300e5fc1d}"

docker run \
    --platform=linux/amd64 \
    --volume="$PWD:$PWD" \
    --user="$(id -u):$(id -g)" \
    --workdir="$PWD" \
    --env=HOME \
    --entrypoint=make \
    --rm \
    "$image" clean all koboroot

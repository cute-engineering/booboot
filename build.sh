#!/usr/bin/env bash

set -e

if [ -z "$CUTEKIT_PYTHON" ]; then
    export CUTEKIT_PYTHON="python3"
fi

if [ -z "$CUTEKIT_VERSION" ]; then
    export CUTEKIT_VERSION="stable"
fi

$CUTEKIT_PYTHON -m cutekit > /dev/null 2>/dev/null || {
    if [ ! -d "./.cutekit/.env" ]; then
        $CUTEKIT_PYTHON -m venv .cutekit/.env
        source .cutekit/.env/bin/activate
        $CUTEKIT_PYTHON -m pip install git+https://github.com/cute-engineering/cutekit.git@${CUTEKIT_VERSION}
    else
        source .cutekit/.env/bin/activate
    fi
}

$CUTEKIT_PYTHON -m cutekit $@

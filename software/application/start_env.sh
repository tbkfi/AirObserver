#!/usr/bin/env bash

[[ -d .venv ]] || exit 1488
source .venv/bin/activate
source <(west completion bash)

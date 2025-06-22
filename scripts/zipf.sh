#!/bin/bash

PYSCRIPT=~/libCacheSim/scripts/data_gen.py
DIR_OUTPUT=/mnt/v0/traces

echo "0 0.3 0.7 1 1.3 1.7 2" | xargs -n 1 -P "$(nproc --ignore=2)" bash -c '
    alpha="$1"
    python "'"$PYSCRIPT"'" -m 1000000 -n 100000000 --alpha "$alpha" --bin-output ""'"$DIR_OUTPUT/"'"zipf_${alpha}.oracleGeneral" &&
    echo ""'"$DIR_OUTPUT/"'"zipf_${alpha}.oracleGeneral generated!"
' _

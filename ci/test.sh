#"!/bin/bash
set -e

#export DRIVER_FLAGS='-j 0 --quiet --rerun'
export DRIVER_FLAGS='-j 0'

case $1 in
    dist)
        for p in examples/*; do \
            make -C $p
        done
        make -C test_regress SCENARIOS=--dist
        ;;
    vlt)
        make -C test_regress SCENARIOS=--vlt
        ;;
    vltmt)
        make -C test_regress SCENARIOS=--vltmt
        ;;
    *)
    echo "Usage: test.sh (dist|vlt|vltmt)"
    exit -1
    ;;
esac

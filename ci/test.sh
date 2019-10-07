#"!/bin/bash
set -e

export DRIVER_FLAGS='-j 0 --quiet --rerun'

case $1 in
    dist)
        make examples
        make test_regress SCENARIOS=--dist
        ;;
    vlt)
        make test_regress SCENARIOS=--vlt
        ;;
    vltmt)
        make test_regress SCENARIOS=--vltmt
        ;;
    *)
    echo "Usage: test.sh (dist|vlt|vltmt)"
    exit -1
    ;;
esac

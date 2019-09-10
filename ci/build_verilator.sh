#!/bin/bash
set -e

if [ -z ${VERILATOR_NUM_JOBS} ]; then
    VERILATOR_NUM_JOBS=$(nproc)
fi

# Caching would be simpler if we installed without VERILATOR_ROOT, but
#   it's needed for driver.pl outside of the repo
if [ -z ${VERILATOR_ROOT} ]; then
    echo "VERILATOR_ROOT not set"
    exit -1
fi

if [ -z ${VERILATOR_CACHE} ]; then
    echo "VERILATOR_CACHE not set"
    exit -1
fi

# TODO -- remove
ls -a ${VERILATOR_CACHE}

VERILATOR_REV=$(cd ${VERILATOR_ROOT} && git rev-parse HEAD)
echo "Found Verilator rev ${VERILATOR_REV}"

CACHED_REV_FILE=${VERILATOR_CACHE}/.rev.txt

if [[ ! -f ${CACHED_REV_FILE} || \
      $(< ${CACHED_REV_FILE}) != ${VERILATOR_REV} ]]; then
    echo "Building Verilator"

# Unsure why Travis monkies with the capitalization of the stage name, but it does
    if [[ -n ${TRAVIS_BUILD_STAGE_NAME} && \
         ${TRAVIS_BUILD_STAGE_NAME} != "Build verilator" ]]; then
      echo "Building Verilator in Travis build stage other than \"Build verilator\": ${TRAVIS_BUILD_STAGE_NAME}"
      exit -1
    fi

    cd ${VERILATOR_ROOT}
    autoconf && ./configure ${VERILATOR_CONFIG_FLAGS} && make -j ${VERILATOR_NUM_JOBS}
# Copy the Verilator build artifacts
    mkdir -p ${VERILATOR_CACHE}
    rm -rf ${VERILATOR_CACHE}/*
    cp bin/*bin* ${VERILATOR_CACHE}
# Remember the Git revision
    echo ${VERILATOR_REV} > ${CACHED_REV_FILE}
else
    echo "Using cached Verilator"
    cd ${VERILATOR_ROOT}
# Create include/verilated_config.h and maybe other things
    autoconf && ./configure ${VERILATOR_CONFIG_FLAGS}
    cp ${VERILATOR_CACHE}/* bin
fi

# TODO -- remove
ls -a ${VERILATOR_CACHE}

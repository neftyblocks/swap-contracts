#!/bin/sh

CONTRACT=$1
BASEDIR=$(dirname "$0")
FULL_BASEDIR="$(cd ${BASEDIR} && pwd)"
CONTRACTS_DIR=${FULL_BASEDIR}/core/contracts
ARTIFACTS_DIR=${FULL_BASEDIR}/artifacts

cd ${CONTRACTS_DIR}/${CONTRACT}
eosio-cpp ${@:2} -abigen -I include -R resource -contract ${CONTRACT} -o ${ARTIFACTS_DIR}/${CONTRACT}.wasm src/${CONTRACT}.cpp

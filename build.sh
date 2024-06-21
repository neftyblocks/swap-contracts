#!/bin/sh

rm -rf ./core/build
mkdir ./core/build
cd ./core/build
cmake ..
make

cd ../../
rm -rf ./artifacts
mkdir ./artifacts
cp ./core/build/contracts/lptoken/*.wasm ./artifacts
cp ./core/build/contracts/lptoken/*.abi ./artifacts
cp ./core/build/contracts/swap/*.wasm ./artifacts
cp ./core/build/contracts/swap/*.abi ./artifacts

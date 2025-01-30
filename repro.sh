#! /usr/bin/env bash
set -eu -o pipefail

echo "GCC version":
g++ --version
echo

echo "Protobuf version: $(pkg-config --modversion protobuf)"
echo

set -x

g++ -Wall -I. -fopenmp gen/Repro.pb.cc Repro.cpp $(pkg-config --cflags --libs protobuf) -o repro -std=c++20 -O2
./repro

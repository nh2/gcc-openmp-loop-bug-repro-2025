#! /usr/bin/env bash
set -eu -o pipefail

echo "GCC version":
g++ --version
echo

set -x

g++ -Wall -I. -fopenmp Repro.cpp -o repro -std=c++20 -O2
./repro

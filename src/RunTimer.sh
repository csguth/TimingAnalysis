#!/bin/bash
export ISPD_BENCHMARK=$1
export ISPD_ROOT=~/Documents/Cientifico/ISPD/ispd_files/ispd$2

echo "ISPD_ROOT is " $ISPD_ROOT
echo "ISPD_BENCHMARK is " $ISPD_BENCHMARK

make && ./TimingAnalysis $ISPD_ROOT $ISPD_BENCHMARK

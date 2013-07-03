#!/bin/bash
export ISPD_BENCHMARK=$1
export ISPD_ROOT=/home/vini/Documents/ISPD_Contest/ispd2012

echo "ISPD_ROOT is " $ISPD_ROOT
echo "ISPD_BENCHMARK is " $ISPD_BENCHMARK

make && ./TimingAnalysis $ISPD_ROOT $ISPD_BENCHMARK

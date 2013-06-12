#!/bin/bash
export ISPD_ROOT=/home/chrystian/Documents/Cientifico/ISPD/ispd_files/ispd2013
export ISPD_BENCHMARK=simple

echo "ISPD_ROOT is " $ISPD_ROOT
echo "ISPD_BENCHMARK is " $ISPD_BENCHMARK

make clean && make && ./TimingAnalysis $ISPD_ROOT $ISPD_BENCHMARK
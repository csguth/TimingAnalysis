#!/bin/bash
export ISPD_ROOT=/home/chrystian/Documents/Cientifico/ISPD/ispd_files/ispd2013
export ISPD_BENCHMARK=usb_phy

echo "ISPD_ROOT is " $ISPD_ROOT
echo "ISPD_BENCHMARK is " $ISPD_BENCHMARK

make && ./TimingAnalysis $ISPD_ROOT $ISPD_BENCHMARK

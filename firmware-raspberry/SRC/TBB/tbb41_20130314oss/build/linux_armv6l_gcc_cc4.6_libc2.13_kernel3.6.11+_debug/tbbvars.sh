#!/bin/bash
export TBBROOT="/home/PoliToGruppo1/Raspberry_Gruppo_1/SRC/TBB/tbb41_20130314oss" #
tbb_bin="/home/PoliToGruppo1/Raspberry_Gruppo_1/SRC/TBB/tbb41_20130314oss/build/linux_armv6l_gcc_cc4.6_libc2.13_kernel3.6.11+_debug" #
if [ -z "$CPATH" ]; then #
    export CPATH="${TBBROOT}/include" #
else #
    export CPATH="${TBBROOT}/include:$CPATH" #
fi #
if [ -z "$LIBRARY_PATH" ]; then #
    export LIBRARY_PATH="${tbb_bin}" #
else #
    export LIBRARY_PATH="${tbb_bin}:$LIBRARY_PATH" #
fi #
if [ -z "$LD_LIBRARY_PATH" ]; then #
    export LD_LIBRARY_PATH="${tbb_bin}" #
else #
    export LD_LIBRARY_PATH="${tbb_bin}:$LD_LIBRARY_PATH" #
fi #
 #

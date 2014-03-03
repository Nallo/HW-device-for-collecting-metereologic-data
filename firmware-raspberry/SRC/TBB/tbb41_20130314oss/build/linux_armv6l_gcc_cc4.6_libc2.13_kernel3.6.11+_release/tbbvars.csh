#!/bin/csh
setenv TBBROOT "/home/PoliToGruppo1/Raspberry_Gruppo_1/SRC/TBB/tbb41_20130314oss" #
setenv tbb_bin "/home/PoliToGruppo1/Raspberry_Gruppo_1/SRC/TBB/tbb41_20130314oss/build/linux_armv6l_gcc_cc4.6_libc2.13_kernel3.6.11+_release" #
if (! $?CPATH) then #
    setenv CPATH "${TBBROOT}/include" #
else #
    setenv CPATH "${TBBROOT}/include:$CPATH" #
endif #
if (! $?LIBRARY_PATH) then #
    setenv LIBRARY_PATH "${tbb_bin}" #
else #
    setenv LIBRARY_PATH "${tbb_bin}:$LIBRARY_PATH" #
endif #
if (! $?LD_LIBRARY_PATH) then #
    setenv LD_LIBRARY_PATH "${tbb_bin}" #
else #
    setenv LD_LIBRARY_PATH "${tbb_bin}:$LD_LIBRARY_PATH" #
endif #
 #

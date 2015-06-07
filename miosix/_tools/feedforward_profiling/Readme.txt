Step response test for control scheduler
========================================

Required tools:
Miosix compiler set up, a jtag adapter, a board running Miosix,
openocd, scilab, CMake, boost libraries

1) Select Miosix OPT_OPTIMIZATION := -O0 in miosix/config/Makefile.inc
to have precise breakpoints

2) Copy test.cpp into base folder and modify Miosix' Makefile
so that test.cpp is built

3) Build Miosix, build jtag_profiler and copy main.elf, jtag_profiler and
gdb_init.script to same folder

5) start openocd

6) run
./jtag_profiler > ff_on.txt
With Miosix compiled with feedforward on and
./jtag_profiler > ff_off.txt
With Miosix compiled with feedforward off

7) run ./plot.sh to see results

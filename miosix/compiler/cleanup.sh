#!/bin/bash

# After running install-script.sh, this script will clean up temporary files

rm -rf binutils-2.19.1/ gcc-4.4.2/ gdb-7.0/ newlib-1.18.0/ newlib-obj/ lpc21isp.c

sudo rm -rf objdir/ a.txt b.txt c.txt d.txt e.txt f.txt g.txt h.txt i.txt j.txt k.txt l.txt m.txt n.txt

#! /bin/bash

# Count lines of code for Miosix
find . | grep -v 'CMSIS' | grep -E "\\.c$|\\.cpp$|\\.h$" | xargs -d \\n cat | wc -l

#!/usr/bin/env bash

# Script for installing all multilibs of libsyscalls.a
# This script can also be used to update an existing install of the Miosix
# compiler to a newer libsyscalls.a.

quit() {
    echo $1
    exit 1
}

if [[ -z "$PREFIX" ]]; then
    quit 'PREFIX must be a toolchain prefix path like ''/opt/arm-miosix-eabi'''
fi

all_multilibs=$(arm-miosix-eabi-gcc --print-multi-lib)
for libspec in $all_multilibs; do
    libspec_parts=(${libspec//;/ })
    MULTILIB_PATH=${libspec_parts[0]}
    MULTILIB_FLAGS=${libspec_parts[1]//@/ -}
    echo "Multilib path $MULTILIB_PATH (flags:$MULTILIB_FLAGS)"
    if [[ "$MULTILIB_PATH" == '.' ]]; then
        echo 'Root multilib, skipped.'
    elif [[ "$MULTILIB_FLAGS" == *-qkernelspace* ]]; then
        echo 'Kernel multilib, skipped.'
    else
        make clean
        make \
            PREFIX="$PREFIX" \
            DESTDIR="$DESTDIR" \
            TARGET=arm-miosix-eabi \
            MULTILIB_PATH="$MULTILIB_PATH" \
            MULTILIB_FLAGS="$MULTILIB_FLAGS" || quit 'Compilation error'
        $SUDO make install \
            PREFIX="$PREFIX" \
            DESTDIR="$DESTDIR" \
            TARGET=arm-miosix-eabi \
            MULTILIB_PATH="$MULTILIB_PATH" \
            MULTILIB_FLAGS="$MULTILIB_FLAGS" || quit 'Installation error'
    fi
done
make clean

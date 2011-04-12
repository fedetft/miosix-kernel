## Script to make the patches
cd ..

GCC=gcc-4.5.2
NEWLIB=newlib-1.19.0

cp gcc-patches/patched_files/eh_alloc.cc $GCC/libstdc++-v3/libsupc++/eh_alloc_patched.cc
cp gcc-patches/patched_files/t-arm-elf   $GCC/gcc/config/arm/t-arm-elf_patched
cp gcc-patches/patched_files/__atexit.c  $NEWLIB/newlib/libc/stdlib/__atexit_patched.c
cp gcc-patches/patched_files/stdio.h     $NEWLIB/newlib/libc/include/stdio_patched.h

diff -ruN $GCC/libstdc++-v3/libsupc++/eh_alloc.cc $GCC/libstdc++-v3/libsupc++/eh_alloc_patched.cc > eh_alloc.patch
diff -ruN $GCC/gcc/config/arm/t-arm-elf           $GCC/gcc/config/arm/t-arm-elf_patched > t-arm-elf.patch
diff -ruN $NEWLIB/newlib/libc/stdlib/__atexit.c   $NEWLIB/newlib/libc/stdlib/__atexit_patched.c > __atexit.patch
diff -ruN $NEWLIB/newlib/libc/include/stdio.h     $NEWLIB/newlib/libc/include/stdio_patched.h > stdio.patch

mv eh_alloc.patch  gcc-patches/
mv t-arm-elf.patch gcc-patches/
mv __atexit.patch  gcc-patches/
mv stdio.patch     gcc-patches/

rm $GCC/libstdc++-v3/libsupc++/eh_alloc_patched.cc
rm $GCC/gcc/config/arm/t-arm-elf_patched
rm $NEWLIB/newlib/libc/stdlib/__atexit_patched.c
rm $NEWLIB/newlib/libc/include/stdio_patched.h
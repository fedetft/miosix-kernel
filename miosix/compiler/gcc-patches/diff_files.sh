## Script to make the patches
cd ..

cp gcc-patches/patched_files/eh_alloc.cc gcc-4.5.2/libstdc++-v3/libsupc++/eh_alloc_patched.cc
cp gcc-patches/patched_files/t-arm-elf   gcc-4.5.2/gcc/config/arm/t-arm-elf_patched

diff -ruN gcc-4.5.2/libstdc++-v3/libsupc++/eh_alloc.cc gcc-4.5.2/libstdc++-v3/libsupc++/eh_alloc_patched.cc > eh_alloc.patch
diff -ruN gcc-4.5.2/gcc/config/arm/t-arm-elf gcc-4.5.2/gcc/config/arm/t-arm-elf_patched > t-arm-elf.patch

mv eh_alloc.patch  gcc-patches/
mv t-arm-elf.patch gcc-patches/

rm gcc-4.5.2/libstdc++-v3/libsupc++/eh_alloc_patched.cc
rm gcc-4.5.2/gcc/config/arm/t-arm-elf_patched


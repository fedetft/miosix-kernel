# Optimization flags, choose one.
# -O0 produces large and slow code, but is useful for in circuit debugging.
# -O2 is recomended otherwise, as it provides a good balance between code
# size and speed

#OPT_OPTIMIZATION := -O0
set(OPT_OPTIMIZATION -O2)
#OPT_OPTIMIZATION := -O3
#OPT_OPTIMIZATION := -Os

# C++ Exception/rtti support disable flags.
# To save code size if not using C++ exceptions (nor some STL code which
# implicitly uses it) uncomment this option.
# the -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.

#set(OPT_EXCEPT "-fno-exceptions -fno-rtti -D__NO_EXCEPTIONS")

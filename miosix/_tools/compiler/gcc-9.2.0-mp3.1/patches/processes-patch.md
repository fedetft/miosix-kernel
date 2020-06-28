
# Notes on the patch to support processes in Miosix

TODO: Intro, pie, single-pic-base, no fixed offset between .text/.rodata and .got/.data/.bss, the usual stuff of how Miosix processes work.

TODO: Comment the changes that were introduced in 4.7.3 and their limitations (processes couldn't use stadard libraries as malloc was broken), as what comes next is a delta compared to it.




## The problem with extern const (FIXED)

In the GCC 4.7.3 processes: 

- stuff   in .data are accessed from the GOT, which is OK
- strings in .rodata are accessed PC-relative, which is OK
- consts in .rodata are accessed from the GOT, which is WRONG! (more on that later when pointer are involved, though)

```
extern const int aRodata;
extern int aData;
const char str[]="Hello world\n";

int get1() { return aRodata; }
int get2() { return aData; }
const char *get3() { return str; }
```

used to compile as:

```
get1:
	ldr	r3, .L3
	ldr	r3, [r9, r3]
	ldr	r0, [r3]
	bx	lr

get2:
	ldr	r3, .L6
	ldr	r3, [r9, r3]
	ldr	r0, [r3]
	bx	lr


get3:
	ldr	r0, .L9
.LPIC0:
	add	r0, pc
	bx	lr
```

(Compiled with `arm-miosix-eabi-gcc -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fpie -msingle-pic-base -O2 -S processes.c`).

What usually masks the issue is constant folding. Unless it's an extern const the value gets folded and the problem does not arise.

Solution:
the previous patch relied on a GCC function, `decl_readonly_section()` to check whether the global variable to be loaded is const or not, but that function missed a few cases, so a dedicated function, the `miosix_processes_ref_demux()` function was added in `arm.c`. This new function handles the corner cases better, and also allows to fix some of the next issues.




## Why malloc failed (FIXED)

`malloc()` uses a convoluted global array of pointers with an initializer list which makes them point inside the array itself. For a long time this was thought to confuse the codegen/relocations/whatever, somehow.

Segfault was at address 0xf2 of this code:

```
  e6:	2318      	movs	r3, #24
  e8:	f8df 2538 	ldr.w	r2, [pc, #1336]	; 624 <_malloc_r+0x55c>
  ec:	f859 6002 	ldr.w	r6, [r9, r2]
  f0:	4433      	add	r3, r6
newlib-3.1.0/newlib/libc/stdlib/mallocr.c:2378
  f2:	685c      	ldr	r4, [r3, #4]
```

But replicating similar code that referenced the same concolute array, `__malloc_av_` did not cause segfaults.

An objdump of main.o found at the end of func:

```
			44: R_ARM_GOT32	__malloc_av_
```

While an objdump of lib_a-mallocr.o at the end of malloc_r:

```
			55c: R_ARM_GOTOFF32	.LANCHOR0
			560: R_ARM_GOTOFF32	.LANCHOR1
			564: R_ARM_GOTOFF32	.LANCHOR2
```

Page 208 of linkers and loaders explains the difference between GOT32 and GOTOFF, and GOTOFF relaocations only work if the gap between .text and .got is known, which is not.

Turns out the issue is unrelated to the convolute array of malloc, a much simpler test case that triggers the issue causing GOTOFF relocations is:

```
int *ptr = { 0 };
int *get6()
{
    return ptr;
}
```

Solution: comment out the part that generates GOTOFF relocations in the `arm_assemble_integer()` function in `arm.c`. In Miosix processes GOTOFF relocations should never appear.




## The problem with const pointers (FIXED)

Accessing `_GLOBAL_REENT` in newlib segfaults, such as in this function:

```
struct _reent *__getreent()
{
    return _GLOBAL_REENT;
}

__getreent():
 a78:	4b01      	ldr	r3, [pc, #4]	; (a80 <__getreent+0x8>)
 a7a:	447b      	add	r3, pc
 a7c:	6818      	ldr	r0, [r3, #0]
 a7e:	4770      	bx	lr
```

Now, `_GLOBAL_REENT` is declared as:

```
// sys/reent.h
extern struct _reent *const _global_impure_ptr;
#define _GLOBAL_REENT _global_impure_ptr
```

and defined as:

```
// impure.c
static struct _reent impure_data = _REENT_INIT(impure_data);
struct _reent *_impure_ptr = &impure_data;
struct _reent *const _global_impure_ptr = &impure_data;
```

At first it was believed that the issue is with the constness of pointer being more complex than normal variables, as we need not confuse the constness of the thing pointed to from the constness of the pointer itself.

However, forther investigation found out the const patch works in this case too:

```
extern const int * cptr;
const int *get() { return cptr; }

	ldr	r3, .L3
	ldr	r3, [r9, r3]
	ldr	r0, [r3]

(symbol_ref:SI ("cptr") [flags 0xc0] <var_decl 0x7f9a30c3d240 cptr>)
 <var_decl 0x7f9a30c3d240 cptr
    type <pointer_type 0x7f9a2ff71b28
        type <integer_type 0x7f9a2ff71a80 int readonly SI
            size <integer_cst 0x7f9a2fe86f78 constant 32>
            unit-size <integer_cst 0x7f9a2fe86f90 constant 4>
            align:32 warn_if_not_align:0 symtab:0 alias-set -1 canonical-type 0x7f9a2ff71a80
            precision:32 min <integer_cst 0x7f9a2fe971c8 -2147483648>
            max <integer_cst 0x7f9a2fe971e0 2147483647>
            pointer_to_this <pointer_type 0x7f9a2ff71b28>>
        unsigned SI size <integer_cst 0x7f9a2fe86f78 32> unit-size <integer_cst 0x7f9a2fe86f90 4>
        align:32 warn_if_not_align:0 symtab:0 alias-set 1 canonical-type 0x7f9a2ff71b28
        pointer_to_this <pointer_type 0x7f9a2ff71dc8>>
    used public unsigned external common read SI processes.c:22:20
    size <integer_cst 0x7f9a2fe86f78 32> unit-size <integer_cst 0x7f9a2fe86f90 4>
    align:32 warn_if_not_align:0 context <translation_unit_decl 0x7f9a2fc95ac8 processes.c>
    (mem/f/c:SI (symbol_ref:SI ("cptr") [flags 0xc0] <var_decl 0x7f9a30c3d240 cptr>)
    [1 cptr+0 S4 A32]) chain <function_decl 0x7f9a2fc92500 get7>>
variable (decl!=0)



extern int * const cptr;
const int *get() { return cptr; }

	add	r3, pc
	ldr	r0, [r3]

(symbol_ref:SI ("cptr") [flags 0xc0] <var_decl 0x7f6823267240 cptr>)
 <var_decl 0x7f6823267240 cptr
    type <pointer_type 0x7f682259ba80
        type <integer_type 0x7f68224bf5e8 int public SI
            size <integer_cst 0x7f68224b0f78 constant 32>
            unit-size <integer_cst 0x7f68224b0f90 constant 4>
            align:32 warn_if_not_align:0 symtab:0 alias-set -1 canonical-type 0x7f68224bf5e8
            precision:32 min <integer_cst 0x7f68224c11c8 -2147483648>
            max <integer_cst 0x7f68224c11e0 2147483647>
            pointer_to_his <pointer_type 0x7f68224c79d8>>
        readonly unsigned SI size <integer_cst 0x7f68224b0f78 32> unit-size <integer_cst 0x7f68224b0f90 4>
        align:32 warn_if_not_align:0 symtab:0 alias-set -1 canonical-type 0x7f682259ba80
        pointer_to_this <pointer_type 0x7f682259bf18>>
    readonly used public unsigned external common read SI processes.c:22:20
    size <integer_cst 0x7f68224b0f78 32> unit-size <integer_cst 0x7f68224b0f90 4>
    align:32 warn_if_not_align:0 context <translation_unit_decl 0x7f68222bfac8 processes.c>
    (mem/u/f/c:SI (symbol_ref:SI ("cptr") [flags 0xc0] <var_decl 0x7f6823267240 cptr>)
    [1 cptr+0 S4 A32]) chain <function_decl 0x7f68222bc500 get7>>
constant (TYPE_READONLY)
```

Basically, `extern const int * cptr;` is a non-const pointer, so it ends up in .data, and is accessed from the GOT, which is OK, and `extern int * const cptr;` is a const pointer which should end up in .rodata, so is accessed PC-relative, which seems good. In general, when deciding whether to use GOT or PC-relative addressing to get a pointer we are concerned about how to acccess the pointer itself, not what it points to (accessing what it points to is trivial and uniform, as you just need to dereference it in all cases).

Although it seems reasonable to assume a const pointer ends up in .rodata just like any otheer const, it's also wrong.

It looks that to know whether a pointer ends up in .rodata or not you can't just look at its **declaration**, you have to look at is **definition**, see for yourself:

```
extern int aData;
int * const ptr1=0;
int * const ptr2=&aData;

	.section	.rodata
ptr1:
	.space	4

	.section	.data.rel.ro,"aw"
ptr2:
	.word	aData
```
If the const pointer is defined as pointing to another variable, it needs to be relocated. That is, the content of the memory cell of the pointer itself isn't known till run-time, because the address of the variable it should point to isn't known. But relocations in .rodata can't happen, as its... readonly, so the constant pointer gets promoted to another section that is in RAM so the relocation can take place.

The implemented fix requres a compromise: in the general case we can't see the pointer definition, as it may be in another translation unit. Thus, we assume that all pointers will need a relocation, and thus we access all pointers (or all complex data structures that may contain pointers, using the `contains_pointers_p()` function of GCC) and use GOT addressing for those.

At first, it was believed that to make this work, we need to actually move them into RAM to make the relocation work.

This patch fragment of `categorize_decl_for_section()` in `varasm.c` did exactly that.

```
diff -ruN gcc-9.2.0-old/gcc/varasm.c gcc-9.2.0/gcc/varasm.c
--- gcc-9.2.0-old/gcc/varasm.c	2019-04-12 09:28:35.000000000 +0200
+++ gcc-9.2.0/gcc/varasm.c	2020-06-25 00:36:05.732146923 +0200
@@ -56,6 +56,8 @@
 #include "asan.h"
 #include "rtl-iter.h"
 #include "file-prefix-map.h" /* remap_debug_filename()  */
+#include "print-tree.h"
+#include <assert.h>
 
 #ifdef XCOFF_DEBUGGING_INFO
 #include "xcoffout.h"		/* Needed for external data declarations.  */
@@ -6675,7 +6677,21 @@
 	/* C and C++ don't allow different variables to share the same
 	   location.  -fmerge-all-constants allows even that (at the
 	   expense of not conforming).  */
-	ret = SECCAT_RODATA;
+    {
+        //TODO: #ifdef _MIOSIX does not work in this context
+        /*
+         * This code matches the if(contains_pointers_p(type)) in
+         * miosix_processes_ref_demux() in arm.c. It disallows pointer-containing
+         * data structures to be const in Miosix processes, as they are always
+         * accessed from the GOT.
+         */
+        tree type = TREE_TYPE(d);
+        assert(type != NULL && "Miosix: TREE_TYPE is null");
+        //printf("--- %d %d\n",flag_pic,contains_pointers_p(type));
+        //debug_tree(d);
+        if(flag_pic && contains_pointers_p(type)) ret = SECCAT_DATA;
+        else ret = SECCAT_RODATA;
+    }
       else if (DECL_INITIAL (decl)
 	       && TREE_CODE (DECL_INITIAL (decl)) == STRING_CST)
 	ret = SECCAT_RODATA_MERGE_STR_INIT;
```

But it was found out that this patch failed to work when arrays were concerned.
An example found in nelib that caused segfaults is in `dtoa.c` when accessing the `tinytens` and `bigtens` arrays defined in `mprec.c`.

A simpler testcase can be made but it requires two translation units to prevent folding masking the issue.

```
// stuff.h
extern const int ptr[];

// stuff.c
const int ptr[] = { 0x12345678 };

// main.c
// do something that accesses ptr

```

This example and the one in libc do cause the appearance of GOT entries which point to .rodata.

Instead of chasing every case where we need to promote something from .rodata to .data, a patch was made in the OS kernel relocation code:
if the relocation target address is not greater than DATA_START (0x10000000), it means that the relocation points to .rodata, and in that case the relocation is done starting from the CODE base address, not from the RAM base address.

This fix made obsolete the idea of forcedly promoting pointer-containing data structures to RAM, so the `varasm.c` patch above was removed. This choice saves RAM, as now

* pointer containing global data structures that need no relocation stay in .rodata (saving RAM) but are accessed from the GOT anyway as-if they were in RAM. The OS relocation code patch makes this work, and there is a small but unavoidable RAM size penalty for the GOT entries, which is however unavoidable as we need an uniform way to access them without seeing their definition.

* pointer containing global data structures that do need relocations get promoted to RAM (necessary) and are accessed from the GOT (necessary, as being in RAM, PC-relative addressing won't work).

TL;DR: in pointer containing data structures, sometimes we go through the GOT even if the data structure we're accessing is in .rodata. This slight inefficiency is however necessary for uniformity in accessing said data structures.



## Addendum

How to recompile GCC only (not the stdlibs) for quick experiments

```
INSTALL_DIR=`pwd`/gcc/arm-miosix-eabi
LIB_DIR=`pwd`/lib
PATH=$INSTALL_DIR/bin:$PATH
cd objdir
make all-gcc
make install-gcc
```

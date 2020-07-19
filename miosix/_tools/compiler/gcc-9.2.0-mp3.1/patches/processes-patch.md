
# Notes on the patch to support processes in Miosix

TODO: Intro, pie, single-pic-base, no fixed offset between .text/.rodata and .got/.data/.bss, the basic stuff of how Miosix processes work.

TODO: Comment the changes that were introduced in 4.7.3 and their limitations (processes couldn't use standard libraries as malloc and everything that used it was broken), as what comes next is a delta compared to it.

NOTE: Most of this document contains addresses from debugging sessions done when DATA_BASE, or the base address of the data segment in a Miosix process before being dynamically relocated, is 0x10000000. This was later changed to 0x40000000, so watch out if some address seems strange.



## The problem with extern const (FIXED)

In processes compiled with GCC 4.7.3: 

- stuff   in .data are accessed from the GOT, which is OK
- strings in .rodata are accessed PC-relative, which is OK
- consts in .rodata are accessed from the GOT, which is WRONG! (more on that later when pointer are involved, though)

Here's an example:

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

`get1()` is the wrong one, of course.

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

But replicating similar code that referenced the same convolute array, `__malloc_av_` did not cause segfaults.

An objdump of `main.o` found at the end of func:

```
			44: R_ARM_GOT32	__malloc_av_
```

While an objdump of `lib_a-mallocr.o` at the end of `malloc_r`:

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

However, further investigation found out the const patch works in this case too:

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
An example found in newlib that caused segfaults is in `dtoa.c` when accessing the `tinytens` and `bigtens` arrays defined in `mprec.c`.

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
if the relocation target address is not greater than DATA_BASE (which was 0x10000000 and later as part of these patch was changed to 0x40000000), it means that the relocation points to .rodata, and in that case the relocation is done starting from the CODE base address, not from the RAM base address.

This fix made obsolete the idea of forcedly promoting pointer-containing data structures to RAM, so the `varasm.c` patch above was removed. This choice saves RAM, as now

* pointer containing global data structures that need no relocation stay in .rodata (saving RAM) but are accessed from the GOT anyway as-if they were in RAM. The OS relocation code patch makes this work, and there is a small but unavoidable RAM size penalty for the GOT entries, which is however unavoidable as we need an uniform way to access them without seeing their definition.

* pointer containing global data structures that do need relocations get promoted to RAM (necessary) and are accessed from the GOT (necessary, as being in RAM, PC-relative addressing won't work).

TL;DR: in pointer containing data structures, sometimes we go through the GOT even if the data structure we're accessing is in .rodata. This slight inefficiency is however necessary for uniformity in accessing said data structures.




## The problem with vtables (FIXED)

An example with `cout << "Hello world" << endl;` fails with a segfault during static construction of the cout object. The problem arises during a `dynamic_cast`.

```
Process 1 terminated due to a fault
* Code base address was 0x64017268
* Data base address was 0x64100000
* MPU region 6 0x64000000-0x64100000 r-x
* MPU region 7 0x64100000-0x64104000 rw-
* Attempted data access @ 0x74017818 (PC was 0x6403a610)
Process 1 terminated
Process segfaulted
```

Trying to understand where the wrong address goes `0x74017818 - 0x64017268 = 0x100005b0` and this address points to

```
 .data.rel.ro._ZTVSt5ctypeIcE
                0x00000000100005b0       0x30 libstdc++.a(ctype.o)
                0x00000000100005b0                _ZTVSt5ctypeIcE
```

and `_ZTVSt5ctypeIcE` is `vtable for std::ctype<char>`.

Further testing with a simpler program fails in the same way

```
#include <cstdio>

class Base
{
public:
    virtual void print() const { puts("I'm Base"); }
    virtual ~Base() {}
};

class Derived : public Base
{
public:
    virtual void print() const { puts("I'm Derived"); }
};

void __attribute__((noinline)) call(Base *base)
{
    base->print();
}

Base *__attribute__((noinline)) mkbase()
{
   return new Base;
}

Derived *__attribute__((noinline)) mkderived()
{
   return new Derived;
}

int main()
{
    volatile int i=0;
    Base *base = i==0 ? mkderived() : mkbase();
    call(base);
    delete base;
}
```

It appears that the issue occurs in the constructor

```
0000ead4 <_Z9mkderivedv>:
_Z9mkderivedv():
    ead4:	b508      	push	{r3, lr}
    ead6:	2004      	movs	r0, #4
    ead8:	f000 f946 	bl	ed68 <_Znwj>
    eadc:	4b02      	ldr	r3, [pc, #8]	; (eae8 <_Z9mkderivedv+0x14>)
    eade:	447b      	add	r3, pc
    eae0:	3308      	adds	r3, #8
    eae2:	6003      	str	r3, [r0, #0]
    eae4:	bd08      	pop	{r3, pc}
    eae6:	bf00      	nop
    eae8:	0fff162e 	svceq	0x00ff162e
```

where the object memory is allocated, and the vptr is set to point to the vtable using PC-relative addressing even though the vtable is in RAM, as it's in .dat.rel.ro

The vtable is considered const even though it contains pointers becauses it passes the `decl_readonly_section` check, which is done before the `contains_pointers_p` check.

```
(symbol_ref/i:SI ("_ZTV4Base") [flags 0x82] <var_decl 0x7ff42878d480 _ZTV4Base>)
 <var_decl 0x7ff42878d480 _ZTV4Base
    type <array_type 0x7ff42781d498
        type <pointer_type 0x7ff427aea498 __vtbl_ptr_type type <function_type 0x7ff427aea348>
            unsigned type_6 SI
            size <integer_cst 0x7ff4279e3108 constant 32>
            unit-size <integer_cst 0x7ff4279e3120 constant 4>
            align:32 warn_if_not_align:0 symtab:0 alias-set 4 canonical-type 0x7ff427aea498
            pointer_to_this <pointer_type 0x7ff427aea7e0>>
        BLK
        size <integer_cst 0x7ff4279e3468 constant 128>
        unit-size <integer_cst 0x7ff4279e3480 constant 16>
        align:32 warn_if_not_align:0 symtab:0 alias-set 4 canonical-type 0x7ff42781d498
        domain <integer_type 0x7ff42781d3f0 type <integer_type 0x7ff4279ed000 sizetype>
            type_6 SI size <integer_cst 0x7ff4279e3108 32> unit-size <integer_cst 0x7ff4279e3120 4>
            align:32 warn_if_not_align:0 symtab:0 alias-set -1 canonical-type 0x7ff42781d3f0 precision:32 min <integer_cst 0x7ff4279e3138 0> max <integer_cst 0x7ff42781a258 3>>
        pointer_to_this <pointer_type 0x7ff427825bd0>>
    readonly addressable used public static tree_1 tree_2 tree_5 ignored weak read virtual decl_5 BLK vtable.cpp:2:7 size <integer_cst 0x7ff4279e3468 128> unit-size <integer_cst 0x7ff4279e3480 16>
    user align:32 warn_if_not_align:0 context <record_type 0x7ff427af3348 Base> initial <constructor 0x7ff42781a270>
   
    (mem/u/c:BLK (symbol_ref/i:SI ("_ZTV4Base") [flags 0x82] <var_decl 0x7ff42878d480 _ZTV4Base>) [4 _ZTV4Base+0 S16 A32])>
constant (decl_readonly_section)
```

As the vtable contains constant pointers (that need to be initialized to within .text to point to the member functions), relocations need to be done.
The solution is to move the `contains_pointers_p` check first.




## The problem with R_ARM_REL32 relocations (FIXED)

Trying to compile C++ programs that do not use exceptions causes compilation to fail. An example as simple as this triggers the issue:

```
#include <cstdio>

using namespace std;

int main()
{
    printf("Hello world\n");
    return 0;
}
```

which fails with:

```
ld: libgcc.a(unwind-arm.o): relocation R_ARM_REL32 against external or undefined symbol `__cxa_call_unexpected' can not be used when making a PIE executable; recompile with -fPIC
libgcc.a(unwind-arm.o): in function `__gnu_unwind_pr_common':
unwind-arm-common.inc:824:(.text+0x744): dangerous relocation: unsupported relocation
```

Compiling with `-fno-exceptions` or adding code that throws/catches exceptions fixes the issue, but code that does not throw should not fail to compile.

The problem is in `unwind-arm-common.inc` which declares a few functions prototypes as `__attribute__((weak))` that it then calls. Moreover, in one case it checkes whether the pointer to the `__gnu_Unwind_Find_exidix` function is not null, to see if the function exists in the (runtime) linked binary.

The fix consists in patching `unwind-arm-common.inc` so that the function prototypes are no longer weak, and removing completely the check **and** then call to `__gnu_Unwind_Find_exidix` as it's not needed in Miosix.




## The problem with unwinding exception tables (FIXED)

A simple program that throws an exception would segfault

```
#include <cstdio>

void __attribute__((noinline)) f()
{
   throw 1; 
}

int main() try {
    puts("in");
    f();
    puts("out");
    return 0;
} catch(int& e) {
    puts("exc");
    return e;
}
```

in `__cxa_type_match`

```
0000eee4 <__cxa_type_match>:
[...]
gcc-9.2.0/libstdc++-v3/libsupc++/eh_arm.cc:86
    ef08:	6823      	ldr	r3, [r4, #0]
```

The fault happens when dereferencing a pointer, but the pointer does not get computed in this function, but passed as a parameter.
The caller is `__gxx_personality_v0`

```
line 576 of eh_personality.cc

      while (1)
	{
	  p = action_record;
	  p = read_sleb128 (p, &ar_filter);
	  read_sleb128 (p, &ar_disp);

	  if (ar_filter == 0)
	    {
	      // Zero filter values are cleanups.
	      saw_cleanup = true;
	    }
	  else if (ar_filter > 0)
	    {
	      // Positive filter values are handlers.
	      catch_type = get_ttype_entry (&info, ar_filter);

	      // Null catch type is a catch-all handler; we can catch foreign
	      // exceptions with this.  Otherwise we must match types.
	      if (! catch_type
		  || (throw_type
		      && get_adjusted_ptr (catch_type, throw_type,
					   &thrown_ptr)))
```

The `get_adjusted_ptr` is a macro to the `__cxa_type_match` code that is faulting.
The corrupted variable is `catch_type`, and is returned by `get_ttype_entry`, which in turn gets it by calling `read_encoded_value_with_base` in `libgcc/unwind-pe.h`.

The `read_encoded_value_with_base` is called with the following parameters:
```
unsigned char encoding = 0x10
_Unwind_Ptr base       = 0
const unsigned char *p = 0x64027198 - 0x64017268 = 0xff30
```

and at memory location 0xff30 in the elf file there is the .ARM.extab section, so this code is retrieving a pointer from the exception unwinding tables.
So, summing up, the unwind tables are coded assuming that it's possible to construct addresses to the typeinfo structures (which are in .data.rel.ro, thus in RAM) through PC-relative addressing which is not possible.

From the same file we also find another useful function:

```
static _Unwind_Ptr
base_of_encoded_value (unsigned char encoding, struct _Unwind_Context *context)
{
  if (encoding == DW_EH_PE_omit)
    return 0;

  switch (encoding & 0x70)
    {
    case DW_EH_PE_absptr:
    case DW_EH_PE_pcrel:
    case DW_EH_PE_aligned:
      return 0;

    case DW_EH_PE_textrel:
      return _Unwind_GetTextRelBase (context);
    case DW_EH_PE_datarel:
      return _Unwind_GetDataRelBase (context);
    case DW_EH_PE_funcrel:
      return _Unwind_GetRegionStart (context);
    }
  __gxx_abort ();
}
```

This function computes the `base` parameter the the previous function, and returns zero since encoding is 0x10 or `DW_EH_PE_pcrel`.

However, `_Unwind_GetDataRelBase()` is in `libgcc/config/arm/pr-support.c`

```
/* These two should never be used.  */

_Unwind_Ptr
_Unwind_GetDataRelBase (_Unwind_Context *context __attribute__ ((unused)))
{
  abort ();
}
```

great, so that's unimplemented...

One last bit we need to get the whole picture: how the compiler selects which encoding to use: `cd gcc/config/arm && grep -R 'DW_EH_PE_'`

which found the list of constants:

```
#define DW_EH_PE_absptr		0x00

#define DW_EH_PE_pcrel		0x10
#define DW_EH_PE_textrel	0x20
#define DW_EH_PE_datarel	0x30
#define DW_EH_PE_funcrel	0x40
#define DW_EH_PE_aligned	0x50

#define DW_EH_PE_indirect	0x80
```

and this file `gcc/config/arm/arm.h` which says

```
#ifndef ARM_TARGET2_DWARF_FORMAT
#define ARM_TARGET2_DWARF_FORMAT DW_EH_PE_pcrel
#endif

/* ttype entries (the only interesting data references used)
   use TARGET2 relocations.  */
#define ASM_PREFERRED_EH_DATA_FORMAT(code, data) \
  (((code) == 0 && (data) == 1 && ARM_UNWIND_INFO) ? ARM_TARGET2_DWARF_FORMAT \
			       : DW_EH_PE_absptr)
```

And this is actually documented!

`https://gcc.gnu.org/onlinedocs/gccint/Exception-Handling.html`

`ASM_PREFERRED_EH_DATA_FORMAT (code, global)`

So, despite the macro implementation in `arm.h` calls the parameters `code` and `data`, they are actually `code` and `global`:

`code`:

* 0 for data
* 1 for code labels
* 2 for function pointers

while `global` is true if the symbol may be affected by dynamic relocations.

From my understanding, the encoding is absptr unless we're accessing data and dynamic relocations may occur (ARM_UNWIND_INFO is the constant 1 so it's always true), in that case it's pcrel.

A possible solution that was tested is:

* `#define ARM_TARGET2_DWARF_FORMAT DW_EH_PE_datarel`
* implement `_Unwind_GetDataRelBase`.

This is the patch that does the first thing:

```
diff -ruN gcc-9.2.0-old/gcc/config/arm/arm.h gcc-9.2.0/gcc/config/arm/arm.h
--- gcc-9.2.0-old/gcc/config/arm/arm.h	2019-04-23 12:03:41.000000000 +0200
+++ gcc-9.2.0/gcc/config/arm/arm.h	2020-07-14 09:14:20.611848691 +0200
@@ -878,7 +878,12 @@
 #define EH_RETURN_STACKADJ_RTX	gen_rtx_REG (SImode, ARM_EH_STACKADJ_REGNUM)
 
 #ifndef ARM_TARGET2_DWARF_FORMAT
-#define ARM_TARGET2_DWARF_FORMAT DW_EH_PE_pcrel
+//TODO: #ifdef _MIOSIX does not work in this context
+//Produce exception unwinding tables that work with Miosix processes
+//see processes-patch.md, section "The problem with unwinding exception tables"
+//we want pcrel as usual for the Miosix kernel, and datarel for processes (pic)
+#define ARM_TARGET2_DWARF_FORMAT (flag_pic ? DW_EH_PE_datarel : DW_EH_PE_pcrel)
+//#define ARM_TARGET2_DWARF_FORMAT DW_EH_PE_pcrel
 #endif
 
 /* ttype entries (the only interesting data references used)
```

This is the patch that does the second:

```
diff -ruN gcc-9.2.0-old/libgcc/config/arm/pr-support.c gcc-9.2.0/libgcc/config/arm/pr-support.c
--- gcc-9.2.0-old/libgcc/config/arm/pr-support.c	2019-04-23 12:03:41.000000000 +0200
+++ gcc-9.2.0/libgcc/config/arm/pr-support.c	2020-07-14 09:14:20.615848615 +0200
@@ -376,7 +376,14 @@
 _Unwind_Ptr
 _Unwind_GetDataRelBase (_Unwind_Context *context __attribute__ ((unused)))
 {
-  abort ();
+//TODO: #ifdef _MIOSIX does not work in this context
+//Support exception unwinding that work with Miosix processes
+//see processes-patch.md, section "The problem with unwinding exception tables"
+//NOTE: this code gets linked (even though it never gets used) also in the kernel,
+//so the symbol name we coose here must also exist in the kernel linker scripts
+  extern char _data asm("_data"); //defined in the linker script
+  return &_data;
+//   abort ();
 }
 
 _Unwind_Ptr
```

And this just adds a print to see what happens:

```
diff -ruN gcc-9.2.0-old/gcc/except.c gcc-9.2.0/gcc/except.c
--- gcc-9.2.0-old/gcc/except.c	2019-03-11 14:58:44.000000000 +0100
+++ gcc-9.2.0/gcc/except.c	2020-07-15 09:55:53.382783507 +0200
@@ -3022,6 +3022,11 @@
   else
     {
       tt_format = ASM_PREFERRED_EH_DATA_FORMAT (/*code=*/0, /*global=*/1);
+      // This is here for debugging the change to ARM_TARGET2_DWARF_FORMAT
+      // in Miosix processes: when compiling C++ code that throws and catches
+      // exceptions, it should print 0x10 (DW_EH_PE_pcrel) when compiling the
+      // kernel (non-pic) and 0x30 (DW_EH_PE_datarel) when compiling processes
+      printf("\n\n-- called 0x%x --\n\n",tt_format);
       if (HAVE_AS_LEB128)
 	ASM_GENERATE_INTERNAL_LABEL (ttype_label,
 				     section ? "LLSDATTC" : "LLSDATT",
```


But these patch don't work. With those changes throwing from a process still fails as before.
Even though at compile-time the DW_EH_PE_datarel value seems to be selected and the printf patch prints 0x30 (the call to `ASM_PREFERRED_EH_DATA_FORMAT` is in `output_one_function_exception_table` in `gcc/except.c` which is where the printf patch above is), `read_encoded_value_with_base` at runtime still gets called with 0x10 (pcrel)...

More digging found this kludge in `eh_personality.cc`:

```
#if _GLIBCXX_OVERRIDE_TTYPE_ENCODING
      /* Older ARM EABI toolchains set this value incorrectly, so use a
	 hardcoded OS-specific format.  */
      info->ttype_encoding = _GLIBCXX_OVERRIDE_TTYPE_ENCODING;
#endif
```

Yes, despite we're wasting bytes in the binary to encode the format of pointer entries, they serve nothing as they are overridden by a compile-time kludge that forces the runtime library to ignore it...

`https://gcc.gnu.org/legacy-ml/gcc-patches/2011-09/msg00765.html`

The `#define _GLIBCXX_OVERRIDE_TTYPE_ENCODING` occurs in `libgcc/config/arm/unwind-arm.h`,
in the middle of the `_Unwind_decode_typeinfo_ptr` function (if you need to make a kludge, do it well...).

Ok, more patching to remove the kludge:

```
diff -ruN gcc-9.2.0-old/libgcc/config/arm/unwind-arm.h gcc-9.2.0/libgcc/config/arm/unwind-arm.h
--- gcc-9.2.0-old/libgcc/config/arm/unwind-arm.h	2019-01-01 13:31:55.000000000 +0100
+++ gcc-9.2.0/libgcc/config/arm/unwind-arm.h	2020-07-14 09:14:20.615848615 +0200
@@ -57,7 +57,14 @@
 #elif defined(__symbian__) || defined(__uClinux__)
 #define _GLIBCXX_OVERRIDE_TTYPE_ENCODING (DW_EH_PE_absptr)
       /* Absolute pointer.  Nothing more to do.  */
+#elif defined(_MIOSIX)
+     //DO NOT DEFINE _GLIBCXX_OVERRIDE_TTYPE_ENCODING, we don't want that kludge
+     //as the encoding could be either pc-relative (kernel) or data-relative (processes)
+     //see processes-patch.md
+     //This relies on base_of_encoded_value() setting base to 0 for DW_EH_PE_pcrel
+     tmp += base ? base : ptr;
 #else
+#error FIXME deleteme added just in case
 #define _GLIBCXX_OVERRIDE_TTYPE_ENCODING (DW_EH_PE_pcrel)
       /* Pc-relative pointer.  */
       tmp += ptr;
```

but this does not work either...

At runtime, in the process, `read_encoded_value_with_base` is called with the following parameters:
```
unsigned char encoding = 0x30
_Unwind_Ptr base       = 0x64100000
const unsigned char *p = 0x640271c4
```

The first two parameters are correct, the third one, is not.

This time the encoding is the correct value. Also the base is ok, so we have the data base address. However, p points to where the offset is stored in the unwinding tables. And dereferencing that location we find 0x0fff023c. This is wrong, as the target we want is at 0x10000188, and 0x64100000 + 0x0fff023c = 0x740f023c which is the faulting address causing the segfault and not the target address.

So while everything is set up for data-relative, the offset that gets encoded is still pc-relative...

And it is here when things get complicated, as we need to llok deep into what's encoded in the unwinding tables.

When compiling the simple `main.cpp` that throws at the beginning of this chapter, GCC produces the following tables, and the unwind tables end up in the binary, with a one-to-one match, (after the ULEB128 encoding is understood `https://en.wikipedia.org/wiki/LEB128`):


```
 .ARM.extab.text.startup.main
                0x000000000000ff30       0x20 main.o

 0ff28                   79f5ff7f b0b0a800  ........y.......
 0ff38 ff301501 0c06080e 011c042a 002e0400  .0.........*....
 0ff48 00010000 3c02ff0f 

	.global	__gxx_personality_v0
	.personality	__gxx_personality_v0
	.handlerdata
	.align	2
                                     79f5ff7f b0b0a800 = personality?
.LLSDA2:
	.byte	0xff                     ff
	.byte	0x30                     30
	.uleb128 .LLSDATT2-.LLSDATTD2    15
.LLSDATTD2:
	.byte	0x1                      01
	.uleb128 .LLSDACSE2-.LLSDACSB2   0c
.LLSDACSB2:
	.uleb128 .LEHB0-.LFB2            06
	.uleb128 .LEHE0-.LEHB0           08
	.uleb128 .L9-.LFB2               0e
	.uleb128 0x1                     01
	.uleb128 .LEHB1-.LFB2            1c
	.uleb128 .LEHE1-.LEHB1           04
	.uleb128 .L10-.LFB2              2a
	.uleb128 0                       00
	.uleb128 .LEHB2-.LFB2            2e
	.uleb128 .LEHE2-.LEHB2           04
	.uleb128 0                       00
	.uleb128 0                       00
.LLSDACSE2:
	.byte	0x1                      01
	.byte	0                        00
	.align	2                        00
	.word	_ZTIi(TARGET2)           3c02ff0f (0x0fff023c)
```

So the problematic offset is right at the end, 0x0fff023c. However, GCC is not producing the address, it just outputs `.word	_ZTIi(TARGET2)`. In this declaration, `_ZTIi` is the target symbol we want to access, and `(TARGET2)` a relocation type.

Who prints this `.word	_ZTIi(TARGET2)` in GCC? We're back to our friend, the `output_one_function_exception_table` in `gcc/except.c`. Just like the address is last in the exception tables, also the code that prints is last in the function,

```
  if (targetm.arm_eabi_unwinder)
    {
      tree type;
      for (i = 0;
	   vec_safe_iterate (cfun->eh->ehspec_data.arm_eabi, i, &type); ++i)
	output_ttype (type, tt_format, tt_format_size);
    }
```

The job is done by the `output_ttype`, but not exactly, as this function contains a

```
  /* Allow the target to override the type table entry format.  */
  if (targetm.asm_out.ttype (value))
    return;
```

and the ARM target has this function pointer non-null, and thus overrides the default `output_ttype` behavior. Following the code we get back to `arm.c`.

```
static bool
arm_output_ttype (rtx x)
{
  fputs ("\t.word\t", asm_out_file);
  output_addr_const (asm_out_file, x);
  /* Use special relocations for symbol references.  */
  if (!CONST_INT_P (x))
    fputs ("(TARGET2)", asm_out_file);
  fputc ('\n', asm_out_file);

  return TRUE;
}
```

and this is the function that prints the symbol name and `(TARGET2)`.

adding an `if(flag_pic) return FALSE;` at the beginning of this function just affords an internal compiler error, apparently as by returning false we get back to `output_ttype` which calls `dw2_asm_output_encoded_addr_rtx` that contains a

```
#ifdef ASM_OUTPUT_DWARF_DATAREL
	case DW_EH_PE_datarel:
	  gcc_assert (GET_CODE (addr) == SYMBOL_REF);
	  ASM_OUTPUT_DWARF_DATAREL (asm_out_file, size, XSTR (addr, 0));
	  break;
#endif
```

and none is provided (sigh).

Ok, backtracking, batching and trying to patch `arm_output_ttype` to produce another relocation type. But which type? The "ELF for the ARM Architecture" document availbale online seems to hint that ` R_ARM_BASE_ABS` is the right one, as it's a static relocation where the target is accessed as B(S) + A, which seems right.


```
diff -ruN gcc-9.2.0-old/gcc/config/arm/arm.c gcc-9.2.0/gcc/config/arm/arm.c
--- gcc-9.2.0-old/gcc/config/arm/arm.c	2019-04-23 12:03:41.000000000 +0200
+++ gcc-9.2.0/gcc/config/arm/arm.c	2020-07-15 23:37:34.457141163 +0200
@@ -27847,7 +28036,23 @@
   output_addr_const (asm_out_file, x);
   /* Use special relocations for symbol references.  */
   if (!CONST_INT_P (x))
+  {
+      //TODO: #ifdef _MIOSIX does not work in this context
+      //When generation C++ exception unwinding tables, DO generate data-relative
+      //entries instead of overriding them with pc-relative relocations
+      //See processes-patch.md
+      if(flag_pic)
+      {
+          //Use R_ARM_RELATIVE to generate a data-relative static relocation
+          //see "ELF for the ARM AELF for the ARM Architecture"
+          printf("using R_ARM_RELATIVE relocation for exception unwinding tables\n");
+          fputs ("(BASE_ABS)", asm_out_file);
+      } else {
+          //Produce the default R_ARM_TARGET2 static relocation that is supposed
+          //to be platform specific but is actually pc-relative
     fputs ("(TARGET2)", asm_out_file);
+      }
+  }
   fputc ('\n', asm_out_file);
 
   return TRUE;
```

And here we find more trouble: the gnu assember does not support the relocation we want being input from assembly files...

Ok, backtracking again, we'll have to make TARGET2 relocations work for our platform. GNU ld has an option, `--target2=<type>` that allows to override how it handles target2 relocations. Great!
By grepping the sources (couldn't find what strings are accepted as type) it looks like the options are `abs`, `rel`, `got-rel`. The default behavior we're seeing is `rel`. `got-rel` seems promising on paper, but it produces garbage for unknown reasons. `abs` is best actually, as it produces the absolute address of the symbol, like 0x10000178. Of course, by just subtracting DATA_BASE, we get the data-relative offset we want. Maybe we could patch the unwinder to subtract DATA_BASE, but it looks like there's a problem.

Miosix does not load binaries compiled with `--target2=abs`, and the reason is simple, other than producing the absolute value, the linker produces a dynamic relocation to fix it up at runtime, as we're in PIE/PIC mode, and for the first time that's NOT what we want, as relocations in a readonly sections can't be made. However, by temporarily commenting out that check in the kernel, adding a check to skip this wrong relocation and patching the binary by hand by removing 0x10000000 to that address, the process works and throws correctly with the readonly unwinding tables.

Now, we just need a non-kludge way to do this.

For this, there's no escape to patching binutils too. The patch first adds the `--target2=mx-data-rel` option and maps it to a brand new static relocation type, `R_ARM_MIOSIXPROC_TGT2`, that is basically the same as `R_ARM_ABS32` (the one behind `--target2=abs`) except it does not leave dynamic relocations behind and subtracts DATA_BASE), making a true data-rel static relocation encoding the offset of the symbol from the data base address.

And that, finally, worked.




## Caveat fot future patchers

If you do an `arm-miosix-eaby-objdump -Dslx main.bin` (of course before it's stripped and mx-postlinked), the start of the disassembly is something like

```
Program Header:
    LOAD off    0x00000098 vaddr 0x00000098 paddr 0x00000098 align 2**3
         filesz 0x00004fcc memsz 0x00004fcc flags r-x
    LOAD off    0x00005068 vaddr 0x40000000 paddr 0x40000000 align 2**3
         filesz 0x00000640 memsz 0x00000840 flags rw-
 DYNAMIC off    0x000050f8 vaddr 0x40000090 paddr 0x40000090 align 2**2
         filesz 0x00000048 memsz 0x00000048 flags rw-

Dynamic Section:
  DEBUG                0x00000000
  REL                  0x00004564
  RELSZ                0x00000b00
  RELENT               0x00000008
  FLAGS_1              0x08000000
  RELCOUNT             0x00000001
private flags = 5000200: [Version5 EABI] [soft-float ABI]
```

which is more useful than it looks, as it allows to see if the binary is good looking. Sometimes innocuous-looking changes in the linker script such as renaming an output section trigger special behavior in the linker with strange side effects, such as adding useless entries to dynamic (such as if you call an output section `.init_array`), adding useless nested segments (such as calling an output section `.ARM.exidx`), or making the text segment writable, which then Miosix will of course fail to load as it violates `W^X`. So do check those when making changes!





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

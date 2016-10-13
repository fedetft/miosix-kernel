#!/usr/bin/perl

#
# usage: perl kernel_global_objects.pl <list of .o files to check>
# returns 0 on success, !=0 on failure.
#
# This program checks every object file in the kernel for the presence of
# global objects, and renames the section used by the compiler to call their
# constructors to a miosix-specific name, so that they can be called early
# in the kernel boot, and not late, which is what occurs for the application
# global objects. This issue arises due to the fact that the kernel and
# application are statically linked in a single executable file, so there is
# the need to separate the kernel global objects from the application ones.
# 
# In Miosix up to 1.61 constructors were called as soon as possible, right
# after copying .data in RAM and clearing .bss, therefore before the kernel
# was started. This allowed to make use of global objects within the kernel,
# as their constructors were called before the objects were used. However,
# this created a problem with user code: also the global constructors of user
# code were called before the kernel was started, and this meant that an
# attempt to printf(), open files, create threads, or other operations possible
# only after the kernel is started would cause a crash of the kernel.
#
# To fix this in Miosix 2.0, application global constructors are called as late
# as possible, right before calling main(), when the kernel is already started.
# However this creates a problem within the kernel, as this creates the
# possibility of accessing an object before its constructor is called.
# The problem was initially fixed in Miosix 2.0 by writing a script, called
# check_global_objects.pl that checked that no global constructors are used
# in the kernel code. However this was found to be too restrictive, so this
# new approach divides the global constructor in two:
# 1) Those of the kernel (i.e: of object files in the miosix directory), which
#    are called after copying .data in RAM and clearing .bss 
# 2) Those of the application code (i.e: those in the miosix top level
#    directory), which are called right before main
#

use warnings;
use strict;

my $verbose=0; # Edit this file and set this to 1 for testing

my @files_with_global_objects;
my @files_to_fix;
my @files_broken;

# Step #1: check all kernel object files and categorize them based on the
# relevant sections
foreach my $filename (@ARGV)
{
	# First, check that the argument is really an object file
	die "$filename is not a file name."    unless -f $filename;
	die "$filename is not an object file." unless    $filename=~/\.o$/;

	# Then use readelf to dump all sections of the file
	my $output=`arm-miosix-eabi-readelf -SW \"$filename\"`;
	my @lines=split("\n",$output);

	my $sections=0;
	foreach my $line (@lines)
	{
		# Lines containing section information have the following format
		# "  [ 2] .text             PROGBITS        00000000 000044 000088 00  AX  0   0  4"
		next unless($line=~/^  \[ ?\d+\] (\S+) /);
		my $section_name=$1;
		$sections++;

		# This is the section that the C++ compiler uses and need to be fixed.
		# Note: we dont care about .fini_array, as the kernel does not call
		# finalizers on reboot
		if($section_name=~/^\.init_array/)
		{
			push(@files_to_fix,$filename);
			push(@files_with_global_objects,"$filename (.init_array)");
		}
		# An already fixed file, may occur when make is called multiple times
		if($section_name=~/^\.miosix_init_array/)
		{
			push(@files_with_global_objects,"$filename (.miosix_init_array)");
		}
		# These sections are related to global objects, but they have not
		# been observed in the wild with the current ABI miosix is using,
		# so for now, to be on the safe side, we will fail compiling if they
		# are found. Probably it is enough to transform their name as well.
		if($section_name=~/^\.preinit_array|^\.ctors|^\.dtors|^\.init |^\.fini /)
		{
			push(@files_broken,$filename);
		}
		
	}

	# Sanity check, if a file has no sections probably something is wrong
	die "$filename has no sections. Something is wrong." unless $sections>0;
}


# Step #2: fix the required files so that they are called before the kernel is
# started, not after
foreach my $filename (@files_to_fix)
{
	my $exitcode=system("arm-miosix-eabi-objcopy \"$filename\" --rename-section .init_array=.miosix_init_array");
	die "Error calling objcopy" unless($exitcode==0);
}

# Step #3: if required to be verbose, print the list of files with global objects
if($verbose)
{
	print("List of kernel files with global constructors\n");
	print("=============================================\n");
	foreach my $filename (@files_with_global_objects)
	{
		$filename=~s/\.o /.cpp /;
		print("$filename\n");
	}
	print("=============================================\n");
}

# Step #4: check that there are no broken files
if(@files_broken!=0)
{
	print("kernel_global_objects.pl: Error: @files_broken contain unexpected sections\n");
	exit(1);
}

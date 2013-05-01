#!/usr/bin/perl

#
# This program checks that there are no global objects in the kernel code.
# usage: perl check_global_objects.pl <list of .o files to check>
# returns 0 on success, 1 on failure. In this case the list of files with
# global objects will be printed.
#
# This program is useful to prevent the use of global constructors.
# This is because the use of global constructors in a kernel creates the
# problem of when to call those constructors.
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
# To fix this in Miosix 2.0, global constructors are called as late as
# possible, right before calling main(), when the kernel is already started.
# However by doing so it is important that global constructors are never used
# inside the kernel itself, as this creates the possibility of accessing an
# object before its constructor is called. To be sure that this does not happen
# a way is needed to automatically check that no global constructors are used
# in the kernel code. Fortunately, this is possible, and this program does
# just that.
#

use warnings;
use strict;

my @broken_files;
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
		$sections++;

		# Look for .preinit_array* .init_array* .fini_array* .ctors* .dtors*
		# sections. These are the sign that the file has global constructors.
		if($1=~/^\.preinit_array|^\.init_array|^\.ctors|^\.fini_array|^\.dtors/)
		{
			push(@broken_files,$filename);
		}
	}

	# Sanity check, if a file has no sections probably something is wrong
	die "$filename has no sections. Something is wrong." unless $sections>0;
}

if(@broken_files!=0)
{
	print("check_global_objects.pl: Error: @broken_files contain global constructors\n");
	exit(1);
}

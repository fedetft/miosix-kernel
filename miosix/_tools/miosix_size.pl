#!/usr/bin/perl

# usage: miosix_size.pl [--prefix=arm-miosix-eabi] <elf files>
#
# The Miosix build system always used the size tool (arm-miosix-eabi-size for
# ARM CPUs) to print the firmware size at the end of a build. However, newer
# linker scripts in Miosix 3 add sections .irq_stack, .heap and .process_pool
# that the size tool incorrectly accounts as .bss, requiring a workaround.
#
# In addition, despite size having three print formats, they all can be improved
#
# 1) The -A option prints detailed information. It is correct but too verbose.
# $ arm-miosix-eabi-size -A main.elf
# main.elf  :
# section              size        addr
# .text               40368   134217728
# .ARM.exidx           1256   134258096
# .irq_stack            512   536870912
# .data                1144   536871424
# .bss                 2512   536872568
# .heap              782264   536875080
# .debug_info        782389           0
# .debug_abbrev      102603           0
# .debug_aranges       6744           0
# .debug_rnglists     15246           0
# .debug_line        174556           0
# .debug_str         296100           0
# .comment               24           0
# .ARM.attributes        56           0
# .debug_frame        17292           0
# .debug_loclists    102608           0
# .debug_line_str       368           0
# Total             2326042
#
# 2) The default is what we've been used so far. It used to provide correct data
# until the aforementioned sections were introduced, now it produces a wrong
# .bss size. Moreover, what's the point of printing the total size in decimal
# and hex?
# $ arm-miosix-eabi-size main.elf
#   text    data     bss     dec     hex filename
#  41624    1144  785288  828056   ca298 main.elf
#
# 3) The GNU format is the most minimalistic and imho useful, but it comes with
# a subtle bug: the .ARM.exidx is accounted as part of .data instead of .text!
# See for yourself in the example below how .text is smaller and .data bigger.
# Additionally, it too gets .bss wrong with the newer linker scripts
# $ arm-miosix-eabi-size -G main.elf
#       text       data        bss      total filename
#      40368       2400     785288     828056 main.elf
#
# This script provides the needed workaround. It spawns arm-miosix-eabi-size -A
# to get the most comprenesive information, ad then it prints the size in the
# GNU format, but getting it right for Miosix.
#

use warnings;
use strict;
use Getopt::Long;

GetOptions(
    "prefix=s" => \( my $prefix = "arm-miosix-eabi")
);

my $text;
my $data;
my $bss;
my $total;
my $filename;

print("      text       data        bss      total filename\n");
format STDOUT=
@>>>>>>>>> @>>>>>>>>> @>>>>>>>>> @>>>>>>>>> @*
$text, $data, $bss, $total, $filename
.

foreach my $idx (0 .. $#ARGV)
{
	$filename=$ARGV[$idx];
	my $printout=`$prefix-size -A $filename`;
	$text = 0;
	$data = 0;
	$bss = 0;
	foreach(split("\n", $printout))
	{
		next unless(/^(\.\S+)\s+(\d+)\s+\d+/);
		if($1 eq ".text")             { $text+=$2; }
		elsif($1 eq ".ARM.exidx")     { $text+=$2; }
		elsif($1 eq ".rodata")        { $text+=$2; } # For processes
		elsif($1 eq ".ARM.extab")     { $text+=$2; } # For processes
		elsif($1 eq ".ARM.exidx.mx")  { $text+=$2; } # For processes
		elsif($1 eq ".rel.dyn")       { $text+=$2; } # For processes
		elsif($1 eq ".rel.data")      { $text+=$2; } # For processes
		elsif($1 eq ".rel.got")       { $text+=$2; } # For processes
		elsif($1 eq ".data")          { $data+=$2; }
		elsif($1 eq ".got")           { $data+=$2; } # For processes
		elsif($1 eq ".dynamic")       { $data+=$2; } # For processes
		elsif($1 eq ".init_array.mx") { $data+=$2; } # For processes
		elsif($1 eq ".bss")           { $bss+=$2;  }
	}
	$total=$text+$data+$bss;
	write(STDOUT);
}

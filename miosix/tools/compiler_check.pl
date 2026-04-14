#!/usr/bin/perl

# usage: compiler_check.pl <compiler name>
# example: perl compiler_check.pl arm-miosix-eabi-gcc
# returns: 0 if the compiler version is compatible with the current kernel
#
# This program is written in perl to help make, which can't even do greater-than
# comparisons without using the shell, let alone regex. Miosix Makefiles need to
# work also on Windows, so we can't use bash and use perl instead.

use warnings;
use strict;

# NOTE: use echo | ... instead of ... < /dev/null for Windows compatibility
my $fullversion=`echo | $ARGV[0] -E -dM -`;
my $patch_major=-1;
for(split /\n/, $fullversion)
{
    next unless(/#define _MIOSIX_GCC_PATCH_MAJOR (\d+)/);
    $patch_major=$1;
}
if($patch_major>=4)
{
    print "0";
} else {
    print "1";
}

#!/usr/bin/perl

# This script is used by the makefiles to convert a relative path from
# a source directory to a relative path to a target directory when recursively
# calling make. Unfortunately, GNU make has no such feature built in. It has,
# however, a way to convert a relative path to an absolute one, that can be
# passed to recursive makefiles while still referencing the same directory,
# but dealing with absolute paths and makefiles is totally useless. Why?
# Because an absolute path may easily end up containing a space character in
# some of the directories, and GNU make fails *very* badly when dealing with
# spaces in filenames. Using relative paths instead only requires that the
# paths between directories containing source files are whitespace free, and
# noth *all* the directory names up to the root one.

use Cwd qw(abs_path);
use File::Spec;

die "Use: perl relpath.pl from to\n" unless($#ARGV+1==2);
my $from=abs_path($ARGV[0]);
my $to=abs_path($ARGV[1]);
my $relpath=File::Spec->abs2rel($to,$from);
$relpath =~ tr#\\#/#; # Windows quirk
print "$relpath";
#print STDERR "========== from='$from' to='$to' relpath='$relpath'\n";

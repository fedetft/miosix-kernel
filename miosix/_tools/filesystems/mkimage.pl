#!/usr/bin/perl

use warnings;
use strict;
use File::Slurp qw(read_file); # apt install libfile-slurp-perl

if($#ARGV+1 < 2 || $ARGV[0] eq "--help") {
    print("Miosix mkimage utility v1.01\n");
    print("Use: mkimage <output image> [<options> <inputimage>]+\n\n");
    print("Options are only valid for the next input image, can be repeated\n");
    print("if multiple images require the same options.\n\n");
    print("Options list\n");
    print("    --offset=<number>  Place the next image at the specified offset\n");
    print("                       instead of after the previous image\n");
    print("    --align=<number>   Align the next image so that the start byte\n");
    print("                       is at an integer multiple of the requested\n");
    print("                       alignment (default alignment = 64 Bytes)\n");
    print("    --pad=<hex number> When padding to meet offset/alignment,\n");
    print("                       choose the byte filler (default = 0x00)\n");
    exit(1);
}

# Open output file, which is always first arg
open(my $out, '>:raw', $ARGV[0]) or die "Error: can't open output file $ARGV[0]";

# Options for next input file, cleared after every file
my $offset, my $align, my $pad_byte;
sub reset_options {
    $offset=0;
    $align=64;
    $pad_byte="\0";
}
reset_options();

# Read all other args
my $out_pos=0;
for(my $i=1; $i<$#ARGV+1; $i++) {
    my $arg=$ARGV[$i];
    if($arg =~ /^--offset=(\d+)([kmKM]?)$/) {
        # Arg is --offset option
        $offset=$1;
        $offset*=1024 if($2 && ($2 eq 'k' || $2 eq 'K'));
        $offset*=1024*1024 if($2 && ($2 eq 'm' || $2 eq 'M'));
        if($out_pos>$offset) {
            die "Error: image size $out_pos overflows requested offset $offset";
        }
    } elsif($arg =~ /^--align=(\d+)([kmKM]?)$/) {
        # Arg is --align option
        $align=$1;
        $align*=1024 if($2 && ($2 eq 'k' || $2 eq 'K'));
        $align*=1024*1024 if($2 && ($2 eq 'm' || $2 eq 'M'));
    } elsif($arg =~ /^--pad=(0x)?([[:xdigit:]]+)$/) {
        # Arg is --pad option
        my $number=hex($2);
        die "Error: $1$2 is not a single byte value" unless($number<256);
        $pad_byte=pack("C",$number);
    } elsif($arg =~ /^--/) {
        # Unknown option
        die "Error: unrecognized option $arg";
    } else {
        # Arg is an input file, handle offset/alignment
        if($offset > 0) {
            my $pad_size=$offset-$out_pos;
            die "Unexpcted" unless($pad_size>=0);
            my $pad=$pad_byte x $pad_size;
            print $out $pad;
            $out_pos=$offset;
        } elsif($align>0) {
            my $prev_out_pos=$out_pos;
            {
                use integer; # Makes division integer
                $out_pos=($out_pos+$align-1)/$align*$align;
            }
            my $pad_size=$out_pos-$prev_out_pos;
            die "Unexpcted" unless($pad_size>=0);
            my $pad=$pad_byte x $pad_size;
            print $out $pad;
        }
        # Copy input file to output file
        die "Error: can't open intput file $arg" unless(-e $arg);
        my $in=read_file($arg, { binmode => ':raw' });
        {
            use bytes; # Makes length return the length in bytes
            $out_pos+=length($in);
        }
        print $out $in;
        reset_options();
    }
}

print("Image size $out_pos\n");

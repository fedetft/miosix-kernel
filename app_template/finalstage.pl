#!/usr/bin/perl
## Copyright 2012 by Terraneo Federico
## Released under the GPLv3
##
## This program is used to perform the last transformations on the
## generated elf file to make it suitable to be loaded by the Miosix
## kernel. It also optionally removes the elf file's section header
## as it is not needed for executing the program and increases the
## binary's size
use warnings;
use strict;

sub slurpin
{
	my $name=shift;
	open(my $file, '<', $name) or die "Can't open input file $name";
	binmode($file);
	my $result;
	$result.=$_ while(<$file>);
	close($file);
	return $result;
}

sub slurpout
{
	my ($name, $data)=@_;
	open(my $file, '>', $name) or die "Can't open output file $name";
	binmode($file);
	print $file $data;
	close($file);
}

die "perl finalstage.pl file.elf [--strip-sectheader]" unless($#ARGV>=0);
my $filename=$ARGV[0];
my $strip=($#ARGV>0 && ($ARGV[1] eq '--strip-sectheader'));

my $elf=slurpin($filename);

# For some unknown reason passing -fpie to gnu ld causes the elf file to be
# marked as a shared library, correct this by writing ET_EXEC (0x02) to
# e_type in the elf header
substr($elf,16,2,"\x02\x00");

if($strip)
{
	# Find the address in the file of the .shstrtab section which marks the
	# beginning of the part to cut, as the file ends with this string table
	# followed by the section header
	sub find_sectheader_start
	{
		my $filename=shift;
		my @lines=split("\n",`arm-miosix-eabi-readelf -S $filename`);
		foreach $_ (@lines)
		{
			s/^.*]//; # Remove [NN] fileld that may contain spaces
			my @tokenized=split /\s+/;
			my $numtokens=@tokenized;
			next unless($numtokens>6);
			return hex $tokenized[4] if($tokenized[1] eq ".shstrtab");
		}
		return 0;
	}

	my $elfend=find_sectheader_start($filename);
	die "Already stripped" if($elfend==0);
	my $elf2=substr($elf,0,$elfend);
	$elf=$elf2;
	substr($elf,32,4,"\x00\x00\x00\x00");# Clear e_shoff     in elf header
	substr($elf,46,2,"\x00\x00");        # Clear e_shentsize in elf header
	substr($elf,48,2,"\x00\x00");        # Clear e_shnum     in elf header
	substr($elf,50,2,"\x00\x00");        # Clear e_shstrndx  in elf header
}

slurpout($filename,$elf);

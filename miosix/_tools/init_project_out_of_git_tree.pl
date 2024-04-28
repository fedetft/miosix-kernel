#!/usr/bin/perl

# This script allows to replicate the content of the Miosix top level directory
# outside of the Miosix git repository, in order to write an application making
# use of Miosix without creating or altering any file in the Miosix git repo.
# This simplifies updating the kernel with git pull and allows to have the
# Miosix git repository as a submodule of another git repository.
#
# To use this script, open a shell in the directory where you want the
# project to be created and run
# perl <path to the kernel>/miosix/_tools/init_project_out_of_source_tree.pl

use warnings;
use strict;
use Cwd qw(cwd abs_path);
use File::Copy;
use File::Copy::Recursive qw(dircopy); # apt install libfile-copy-recursive-perl
use File::Basename;
use File::Spec;

# Target directory is where the script is launched from
my $target=cwd();

# Get the kernel top level directory from the script path. The script must be
# placed in the miosix/_tools subdirectory of the Miosix git repo
my $source=dirname(abs_path($0));
die "Can't locate the kernel directory\n" unless($source =~ /(\\|\/)miosix(\\|\/)_tools$/);
$source =~ s/(\\|\/)miosix(\\|\/)_tools$//;
die "The project directory must be outside of the kernel tree\n" if(index($target,$source)==0);

die "Error: file main.cpp already exists\n" if -e "main.cpp";
die "Error: file Makefile already exists\n" if -e "Makefile";
die "Error: directory config already exists\n" if -e "config";

# Copy the Makefile fixing the KPATH and CONFPATH lines. This is what makes
# building out the git tree possible
sub copy_and_fixup_makefile
{
	my ($in_name,$out_name)=@_;
	open(my $in, '<', $in_name) or die;
	open(my $out, '>', $out_name) or die;
	# KPATH must contain the path to the kernel. Said path must be relative and
	# in the unix format (i.e: with / as path separator, not \). This is
	# because the generated Makefile may be put in a public git repository that
	# has the miosix kernel as a submodule, and it would be bad to put an
	# absolute path in a git repo, or a path that is only usable from windows
	my $relpath=File::Spec->abs2rel($source,$target);
	$relpath =~ s/\\/\//; # Force the use of / as path separator
	while(<$in>)
	{
		s/^KPATH := miosix$/KPATH := $relpath\/miosix/;
		s/^CONFPATH := \$\(KPATH\)$/CONFPATH := \./;
		print $out "$_";
	}
	close($in);
	close($out);
}

copy("$source/main.cpp","$target/main.cpp") or die;
copy_and_fixup_makefile("$source/Makefile","$target/Makefile");
dircopy("$source/miosix/config","$target/config") or die;

print "Successfully created Miosix project\n";
print "Target directory: $target\n";
print "Kernel directory: $source\n";

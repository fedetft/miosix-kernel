#!/usr/bin/perl

# This script allows to replicate the content of the Miosix top level directory
# outside of the Miosix git repository, in order to write an application making
# use of Miosix without creating or altering any file in the Miosix git repo.
# This simplifies updating the kernel with git pull and allows to have the
# Miosix git repository as a submodule of another git repository.
#
# To use this script, open a shell in the directory where you want the
# project to be created and run
# perl <path to the kernel>/tools/init_project_out_of_git_tree.pl

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
# placed in the tools subdirectory of the Miosix git repo
my $script_path=dirname(abs_path($0));
die "Can't locate the kernel directory\n" unless($script_path =~ /(\\|\/)tools$/);
(my $repo_path = $script_path) =~ s/(\\|\/)tools$//;
my $kpath = "$repo_path/miosix";
die "Can't locate the kernel directory\n" unless(-d $kpath);
my $source = "$repo_path/templates/simple";
die "Can't locate the simple example directory\n" unless(-d $source);
die "The project directory must be outside of the kernel tree\n" if(index($target,$source)==0);

die "Error: file main.cpp already exists\n" if -e "main.cpp";
die "Error: file Makefile already exists\n" if -e "Makefile";
die "Error: file CMakeLists.txt already exists\n" if -e "CMakeLists.txt";
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
	my $relpath=File::Spec->abs2rel($kpath,$target);
	$relpath =~ s/\\/\//; # Force the use of / as path separator
	while(<$in>)
	{
		s/^KPATH := .*$/KPATH := $relpath/;
		s/^CONFPATH := .*$/CONFPATH := ./;
		print $out "$_";
	}
	close($in);
	close($out);
}

# Copy the CMakeLists.txt fixing the KPATH line. This is what makes
# building out the git tree possible
sub copy_and_fixup_cmake
{
	my ($in_name,$out_name)=@_;
	open(my $in ,'<', $in_name) or die $!;
	open(my $out,'>', $out_name) or die $!;
	my $relpath=File::Spec->abs2rel($kpath,$target);
	$relpath =~ s/\\/\//;
	while(<$in>)
	{
		s/^set\(MIOSIX_KPATH [^)]+\)$/set(MIOSIX_KPATH \${CMAKE_SOURCE_DIR}\/$relpath)/;
		s/# set\(MIOSIX_USER_CONFIG_PATH [^)]+\)$/set(MIOSIX_USER_CONFIG_PATH \${CMAKE_SOURCE_DIR}\/config)/;
		print $out "$_";
	}
	close $in;
	close $out;
}

copy("$source/main.cpp","$target/main.cpp") or die;
copy_and_fixup_makefile("$source/Makefile","$target/Makefile");
copy_and_fixup_cmake("$source/CMakeLists.txt","$target/CMakeLists.txt");
dircopy("$kpath/config","$target/config") or die;

print "Successfully created Miosix project\n";
print "Target directory: $target\n";
print "Kernel directory: $kpath\n";

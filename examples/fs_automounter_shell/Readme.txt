This example provides a tiny shell to exercise the SD automounter from a
serial terminal.

The shell supports a few simple commands to inspect the mounted filesystem and
toggle the automounter worker:

cd <dir>      change directory
ls [dir]      list directory contents
stat <path>   print device, size and mode
cat <path>    print a text file
echo ...      print text or redirect to a file with > or >>
touch <path>  create an empty file if it does not exist
rm <path>     remove a file or an empty directory
enable        enable the SD automounter
disable       disable the SD automounter

The example assumes the board support package already configured the
automounter. On the STM32F4Discovery setup used during development this is
done in bspInit2().

Loop mount is the process of mounting a file containing a disk image to a
mountpoint. The most common example is to mount an .iso image to extract some
files without burning it on a Cd or DVD.

Linux supports arbitrary loop mounts, and also allows write access to the
mounted filesystem, which reflects into changes to the disk image file.

Miosix 2.0 supports loop mount too, and this example shows how to perform it.

This readme assumes you are on a Linux machine, so that you can create the
disk image and mount it. If you are on windows, sorry, but windows does not
support loop mount out of the box, so you'd better install a Linux virtual
machine.

To begin, let's create a 1MB disk image, and format it in FAT32

dd if=/dev/zero of=disk.dd bs=1024 count=1000
mkfs.vfat disk.dd

Then let's mount the disk image in Linux and add some files

mkdir disk
sudo mount disk.dd disk -o loop,rw,users,uid=`id -u`,gid=`id -g`
cd disk
touch it_works.txt
cd ..

You can feel free to add files to the disk image, simply copy them to the disk
directory. When you are satisfied, umount the disk image with the following
commands

sudo umount disk
rmdir disk

You can then move the disk.dd file to and SD card so that Miosix can mount it.
Compile the loop_mount.cpp file together with the Miosix kernel, and run it
the SD card containing the disk.dd file on any supported board.
The example program will mount the disk, list the name of all files in the root
directory of the disk image, and create a file named file.txt, to test write
access.

To verify the file creation, copy the disk image back to Linux and mount it.

/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module configuration file  R0.10  (C)ChaN, 2013
/----------------------------------------------------------------------------/
/
/ CAUTION! Do not forget to make clean the project after any changes to
/ the configuration options.
/
/----------------------------------------------------------------------------*/
#ifndef _FFCONF
#define _FFCONF 80286	/* Revision ID */
#include "config/miosix_settings.h"


/*---------------------------------------------------------------------------/
/ Functions and Buffer Configurations
/----------------------------------------------------------------------------*/

#define	_FS_TINY		0	/* 0:Normal or 1:Tiny */
/* When _FS_TINY is set to 1, FatFs uses the sector buffer in the file system
/  object instead of the sector buffer in the individual file object for file
/  data transfer. This reduces memory consumption 512 bytes each file object. */


#define _FS_READONLY	0	/* 0:Read/Write or 1:Read only */
/* Setting _FS_READONLY to 1 defines read only configuration. This removes
/  writing functions, f_write(), f_sync(), f_unlink(), f_mkdir(), f_chmod(),
/  f_rename(), f_truncate() and useless f_getfree(). */


#define _FS_MINIMIZE	0	/* 0 to 3 */
/* The _FS_MINIMIZE option defines minimization level to remove API functions.
/
/   0: All basic functions are enabled.
/   1: f_stat(), f_getfree(), f_unlink(), f_mkdir(), f_chmod(), f_utime(),
/      f_truncate() and f_rename() function are removed.
/   2: f_opendir(), f_readdir() and f_closedir() are removed in addition to 1.
/   3: f_lseek() function is removed in addition to 2. */


#define	_USE_STRFUNC	0	/* 0:Disable or 1-2:Enable */
/* To enable string functions, set _USE_STRFUNC to 1 or 2. */

#define _USE_FIND		0
/* This option switches filtered directory read functions, f_findfirst() and
/  f_findnext(). (0:Disable, 1:Enable 2:Enable with matching altname[] too) */

#define	_USE_MKFS		0	/* 0:Disable or 1:Enable */
/* To enable f_mkfs() function, set _USE_MKFS to 1 and set _FS_READONLY to 0 */


#define	_USE_FASTSEEK	0	/* 0:Disable or 1:Enable */
/* To enable fast seek feature, set _USE_FASTSEEK to 1. */

#define _USE_EXPAND	0
/* This option switches f_expand function. (0:Disable or 1:Enable) */


#define _USE_CHMOD	0
/* This option switches attribute manipulation functions, f_chmod() and f_utime().
/  (0:Disable or 1:Enable) Also _FS_READONLY needs to be 0 to enable this option. */


#define _USE_LABEL		0	/* 0:Disable or 1:Enable */
/* To enable volume label functions, set _USE_LAVEL to 1 */


#define	_USE_FORWARD	0	/* 0:Disable or 1:Enable */
/* To enable f_forward() function, set _USE_FORWARD to 1 and set _FS_TINY to 1. */

#define _USE_STRFUNC	0
#define _PRINT_LLI	1
#define _PRINT_FLOAT	1
#define _STRF_ENCODE	3
/* _USE_STRFUNC switches string functions, f_gets(), f_putc(), f_puts() and
/  f_printf().
/
/   0: Disable. _PRINT_LLI, _PRINT_FLOAT and _STRF_ENCODE have no effect.
/   1: Enable without LF-CRLF conversion.
/   2: Enable with LF-CRLF conversion.
/
/  _PRINT_LLI = 1 makes f_printf() support long long argument and _PRINT_FLOAT = 1/2
/  makes f_printf() support floating point argument. These features want C99 or later.
/  When FF_LFN_UNICODE >= 1 with LFN enabled, string functions convert the character
/  encoding in it. _STRF_ENCODE selects assumption of character encoding ON THE FILE
/  to be read/written via those functions.
/
/   0: ANSI/OEM in current CP
/   1: Unicode in UTF-16LE
/   2: Unicode in UTF-16BE
/   3: Unicode in UTF-8
*/


/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/----------------------------------------------------------------------------*/

#define _CODE_PAGE	1252
/* The _CODE_PAGE specifies the OEM code page to be used on the target system.
/  Incorrect setting of the code page can cause a file open failure.
/
/   932  - Japanese Shift-JIS (DBCS, OEM, Windows)
/   936  - Simplified Chinese GBK (DBCS, OEM, Windows)
/   949  - Korean (DBCS, OEM, Windows)
/   950  - Traditional Chinese Big5 (DBCS, OEM, Windows)
/   1250 - Central Europe (Windows)
/   1251 - Cyrillic (Windows)
/   1252 - Latin 1 (Windows)
/   1253 - Greek (Windows)
/   1254 - Turkish (Windows)
/   1255 - Hebrew (Windows)
/   1256 - Arabic (Windows)
/   1257 - Baltic (Windows)
/   1258 - Vietnam (OEM, Windows)
/   437  - U.S. (OEM)
/   720  - Arabic (OEM)
/   737  - Greek (OEM)
/   775  - Baltic (OEM)
/   850  - Multilingual Latin 1 (OEM)
/   858  - Multilingual Latin 1 + Euro (OEM)
/   852  - Latin 2 (OEM)
/   855  - Cyrillic (OEM)
/   866  - Russian (OEM)
/   857  - Turkish (OEM)
/   862  - Hebrew (OEM)
/   874  - Thai (OEM, Windows)
/   1    - ASCII (Valid for only non-LFN cfg.)
*/

//Note by TFT: DEF_NAMEBUF is used in the following functions: f_open(),
//f_opendir(), f_readdir(), f_stat(), f_unlink(), f_mkdir(), f_chmod(),
//f_utime(), f_rename(), and Miosix always locks a mutex before calling any of,
//these. For this reason it was chosen to allocate the LFN buffer statically,
//since the mutex protects from concurrent access to the buffer.
#define	_USE_LFN	1		/* 0 to 3 */
#define	_MAX_LFN	255		/* Maximum LFN length to handle (12 to 255) */
/* The _USE_LFN option switches the LFN feature.
/
/   0: Disable LFN feature. _MAX_LFN has no effect.
/   1: Enable LFN with static working buffer on the BSS. Always NOT reentrant.
/   2: Enable LFN with dynamic working buffer on the STACK.
/   3: Enable LFN with dynamic working buffer on the HEAP.
/
/  To enable LFN feature, Unicode handling functions ff_convert() and ff_wtoupper()
/  function must be added to the project.
/  The LFN working buffer occupies (_MAX_LFN + 1) * 2 bytes. When use stack for the
/  working buffer, take care on stack overflow. When use heap memory for the working
/  buffer, memory management functions, ff_memalloc() and ff_memfree(), must be added
/  to the project. */

//Note by TFT: we do want unicode and not ancient code pages
#define	_LFN_UNICODE	1	/* 0:ANSI/OEM or 1:Unicode */
/* To switch the character encoding on the FatFs API to Unicode, enable LFN feature
/  and set _LFN_UNICODE to 1. */


#define _STRF_ENCODE	3	/* 0:ANSI/OEM, 1:UTF-16LE, 2:UTF-16BE, 3:UTF-8 */
/* When Unicode API is enabled, character encoding on the all FatFs API is switched
/  to Unicode. This option selects the character encoding on the file to be read/written
/  via string functions, f_gets(), f_putc(), f_puts and f_printf().
/  This option has no effect when _LFN_UNICODE is 0. */



#define _LFN_BUF		255
#define _SFN_BUF		12
/* This set of options defines size of file name members in the FILINFO structure
/  which is used to read out directory items. These values should be suffcient for
/  the file names to read. The maximum possible length of the read file name depends
/  on character encoding. When LFN is not enabled, these options have no effect. */


#define _FS_RPATH		0	/* 0 to 2 */
/* The _FS_RPATH option configures relative path feature.
/
/   0: Disable relative path feature and remove related functions.
/   1: Enable relative path. f_chdrive() and f_chdir() function are available.
/   2: f_getcwd() function is available in addition to 1.
/
/  Note that output of the f_readdir() fnction is affected by this option. */


/*---------------------------------------------------------------------------/
/ Drive/Volume Configurations
/----------------------------------------------------------------------------*/

#define _VOLUMES	1
/* Number of volumes (logical drives) to be used. */

#define _STR_VOLUME_ID	0
#define _VOLUME_STRS		"RAM","NAND","CF","SD","SD2","USB","USB2","USB3"
/* _STR_VOLUME_ID switches support for volume ID in arbitrary strings.
/  When _STR_VOLUME_ID is set to 1 or 2, arbitrary strings can be used as drive
/  number in the path name. _VOLUME_STRS defines the volume ID strings for each
/  logical drives. Number of items must not be less than _VOLUMES. Valid
/  characters for the volume ID strings are A-Z, a-z and 0-9, however, they are
/  compared in case-insensitive. If _STR_VOLUME_ID >= 1 and _VOLUME_STRS is
/  not defined, a user defined volume string table is needed as:
/
/  const char* VolumeStr[_VOLUMES] = {"ram","flash","sd","usb",...
*/


#define	_MULTI_PARTITION	0	/* 0:Single partition, 1:Enable multiple partition */
/* When set to 0, each volume is bound to the same physical drive number and
/ it can mount only first primaly partition. When it is set to 1, each volume
/ is tied to the partitions listed in VolToPart[]. */


#define  _MIN_SS		512
#define	_MAX_SS		512		/* 512, 1024, 2048 or 4096 */
/* Maximum sector size to be handled.
/  Always set 512 for memory card and hard disk but a larger value may be
/  required for on-board flash memory, floppy disk and optical disk.
/  When _MAX_SS is larger than 512, it configures FatFs to variable sector size
/  and GET_SECTOR_SIZE command must be implemented to the disk_ioctl() function. */


#define _LBA64		_FS_EXFAT
/* This option switches support for 64-bit LBA. (0:Disable or 1:Enable)
/  To enable the 64-bit LBA, also exFAT needs to be enabled. (FF_FS_EXFAT == 1) */


#define _MIN_GPT		0x10000000
/* Minimum number of sectors to switch GPT as partitioning format in f_mkfs and
/  f_fdisk function. 0x100000000 max. This option has no effect when FF_LBA64 == 0. */


#define _USE_TRIM		0
/* This option switches support for ATA-TRIM. (0:Disable or 1:Enable)
/  To enable Trim function, also CTRL_TRIM command should be implemented to the
/  disk_ioctl() function. */

#define	_USE_ERASE	0	/* 0:Disable or 1:Enable */
/* To enable sector erase feature, set _USE_ERASE to 1. Also CTRL_ERASE_SECTOR command
/  should be added to the disk_ioctl() function. */


#define _FS_NOFSINFO	0	/* 0 or 1 */
/* If you need to know the correct free space on the FAT32 volume, set this
/  option to 1 and f_getfree() function at first time after volume mount will
/  force a full FAT scan.
/
/  0: Load all informations in the FSINFO if available.
/  1: Do not trust free cluster count in the FSINFO.
*/



/*---------------------------------------------------------------------------/
/ System Configurations
/----------------------------------------------------------------------------*/

#define _WORD_ACCESS	0	/* 0 or 1 */
/* The _WORD_ACCESS option is an only platform dependent option. It defines
/  which access method is used to the word data on the FAT volume.
/
/   0: Byte-by-byte access. Always compatible with all platforms.
/   1: Word access. Do not choose this unless under both the following conditions.
/
/  * Byte order on the memory is little-endian.
/  * Address miss-aligned word access is always allowed for all instructions.
/
/  If it is the case, _WORD_ACCESS can also be set to 1 to improve performance
/  and reduce code size.
*/

//#define _FS_EXFAT		1 // Now in the miosix_settings.h configuration file
/* This option switches support for exFAT filesystem. (0:Disable or 1:Enable)
/  To enable exFAT, also LFN needs to be enabled. (_USE_LFN >= 1)
/  Note that enabling exFAT discards ANSI C (C89) compatibility. */
#if (_FS_EXFAT ^ _LBA64)
// #error _FS_EXFAT need to be enabled in case of _LBA64 enabled. Also, off64_t is needed
// By Raul Radu: off_t is guaranteed to be 8 bytes in miosix so no need for off64_t.
#error _FS_EXFAT need to be enabled in case of _LBA64 enabled.
#endif

/* A header file that defines sync object types on the O/S, such as
/  windows.h, ucos_ii.h and semphr.h, must be included prior to ff.h. */

//Note by TFT: the reentrant option uses just one big lock for each FATFS, so
//given there's no concurrency advantage in using this option, we're just using
//an ordinary KernelMutex in class Fat32Fs and locking it before calling FatFs.
#define _FS_REENTRANT	0		/* 0:Disable or 1:Enable */
#define _FS_TIMEOUT		1000	/* Timeout period in unit of time ticks */
#define	_SYNC_t			HANDLE	/* O/S dependent type of sync object. e.g. HANDLE, OS_EVENT*, ID and etc.. */

/* The _FS_REENTRANT option switches the re-entrancy (thread safe) of the FatFs module.
/
/   0: Disable re-entrancy. _SYNC_t and _FS_TIMEOUT have no effect.
/   1: Enable re-entrancy. Also user provided synchronization handlers,
/      ff_req_grant(), ff_rel_grant(), ff_del_syncobj() and ff_cre_syncobj()
/      function must be added to the project. */

//Note by TFT: this is very useful, as it avoids the danger of opening the same
//file for writing multiple times
#define	_FS_LOCK	/* 0:Disable or >=1:Enable */
/* To enable file lock control feature, set _FS_LOCK to 1 or greater.
   The value defines how many files can be opened simultaneously. */


#endif /* _FFCONFIG */

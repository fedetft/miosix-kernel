/*-------------------------------------------*/
/* Integer type definitions for FatFs module */
/*-------------------------------------------*/

#include "config/miosix_settings.h"
#include "filesystem/fatfs/ffconf.h"

#ifndef _FF_INTEGER
#define _FF_INTEGER

#ifdef _WIN32	/* FatFs development platform */

#include <windows.h>
#include <tchar.h>

#else			/* Embedded platform */

/* This type MUST be 8 bit */
typedef unsigned char	BYTE;

/* These types MUST be 16 bit */
typedef short			SHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types MUST be 16 bit or 32 bit */
typedef int				INT;
typedef unsigned int	UINT;

/* These types MUST be 32 bit */
typedef long			LONG;
typedef unsigned long	DWORD;

#endif //_WIN32

#if _FS_EXFAT

/* These types MUST be 64 bit */
typedef unsigned long long  QWORD;
typedef QWORD FSIZE_t;

/* These types MUST be 32 bit or 64 bit */
#if _LBA64
typedef unsigned long long  LBA_t;
#else
	typedef unsigned long   LBA_t;
#endif // _LBA64

#else
/* These types MUST be 32 bit if not EXFAT (FAT) */
typedef unsigned long   FSIZE_t;
typedef unsigned long   LBA_t;

#endif //_FS_EXFAT
#endif //_FF_INTEGER

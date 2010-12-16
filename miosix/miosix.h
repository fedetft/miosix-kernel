
//Common #include are grouped here for ease of use 

#ifndef MIOSIX_H
#define	MIOSIX_H

/* Hardware */
#include "interfaces/arch_registers.h"
#include "interfaces/gpio.h"
#include "interfaces/delays.h"
#include "interfaces/console.h"
#include "interfaces/bsp.h"
/* Miosix kernel */
#include "kernel/kernel.h"
#include "kernel/sync.h"
#include "kernel/filesystem/filesystem.h"
/* Utilities */
#include "util/util.h"
/* Settings */
#include "config/miosix_settings.h"

#endif	//MIOSIX_H

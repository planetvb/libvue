/*
  Copyright (C) 2017 Planet Virtual Boy

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/



/* This file is intended to be #included by vue.c */
#ifdef VUEAPI



/*****************************************************************************
 *                                 Constants                                 *
 *****************************************************************************/

/* Instruction cache fields */
#define ICC 0x00000001
#define ICE 0x00000002
#define ICD 0x00000010
#define ICR 0x00000020



/*****************************************************************************
 *                             Library Functions                             *
 *****************************************************************************/

/* Perform instruction cache commands */
/* RESEARCH: Can ICC and ICD/ICR be performed simultaneously? */
static uint32_t cpuCacheControl(VUE_CONTEXT *vb, uint32_t value) {
    uint32_t cec, cen, x;

    /* Clear instruction cache entries */
    if (value & ICC) {

        /* Parse clear parameters from the value */
        cec = value >>  8 & 0x0FFF;
        cen = value >> 20 & 0x0FFF;

        /* Parameters specify a valid operation */
        if (cen < 128 && cec > 0) {

            /* Restrict the number of entries to clear */
            if (cen + cec > 128)
                cec = 128 - cen;

            /* Clear the entries */
            for (x = 0; x < cec; x++) {
                /* entry[cen + x] = 0 */
            }
        }
    }

    /* Dump instruction cache to memory */
    if (value & ICD & ICR == ICD) {
        /* dump_to_address(value & 0xFFFFFF00) */
    }

    /* Restore instruction cache from memory */
    if (value & ICD & ICR == ICR) {
        /* restore_from_address(value & 0xFFFFFF00) */
    }

    /* Update and return the system register value */
    return vb->cpu.chcw = value & ICE;
}

/* Perform a read on the CPU bus                                   */
/* Returns non-zero if the application requests an emulation break */
static int cpuRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Call the application-supplied access handler if available */
    if (vb->debug.onread)
        return vb->debug.onread(vb, access);

    /* Call the bus handler directly */
    busRead(vb, access);
    return 0;
}

/* Perform a write on the CPU bus                                  */
/* Returns non-zero if the application requests an emulation break */
static int cpuWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Call the application-supplied access handler if available */
    if (vb->debug.onwrite)
        return vb->debug.onwrite(vb, access);

    /* Call the bus handler directly */
    busWrite(vb, access);
    return 0;
}



#endif /* VUEAPI */

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

#define VUE_API
#include <vue.h>



/*****************************************************************************
 *                                 Constants                                 *
 *****************************************************************************/

/* stdio.h isn't included, so NULL may not be present */
#ifndef NULL
#define NULL ((void *) 0)
#endif

/* Fixed system register values */
#define PIR  0x00005346
#define SR30 0x00000004
#define TKCW 0x000000E0

/* Maximum unsigned register value */
#define WORD_MAX ((uint32_t) (int32_t) -1)

/* Maximum negative register value */
#define WORD_MIN (-2147483647 - 1)



/*****************************************************************************
 *                             Library Functions                             *
 *****************************************************************************/

/* Determine whether a number is a power of 2 */
static int vueIsPowerOfTwo(uint32_t x) {
    uint32_t bit;
    for (bit = 1; bit; bit <<= 1)
        if (bit == x) return 1;
    return 0;
}

/* Mask an address according to format */
static uint32_t vueMaskAddress(uint32_t address, uint8_t format) {
    return address & ((uint32_t) -1 << (((format & 0x7F) >> 3) - 1));
}

/* Convert a signed value into an unsigned value */
/* Unsigned-to-signed conversions are implementation-defined in C89 */
static int32_t vueSign(uint32_t value) {
    return *(int32_t *)&value; /* The binary format is guaranteed */
}

/* Sign-extend a value of arbitrary bit length */
static int32_t vueSignExtend(int32_t value, int32_t bits) {
    if (bits < 32 && (value & ((int32_t) 1 << bits)))
        value |= (int32_t) -1 << bits;
    return value;
}

/* Zero-extend a value of arbitrary bit length */
static int32_t vueZeroExtend(int32_t value, int32_t bits) {
    if (bits < 32)
        value &= ~((int32_t) -1 << bits);
    return value;
}



/*****************************************************************************
 *                           Sub-Library Includes                            *
 *****************************************************************************/

/* Forward references for library functions */
static int      cpuCheckCondition(VUE_CONTEXT *, int);
static int      cpuLDSR          (VUE_CONTEXT *, int, uint32_t, int);
static int      cpuRaiseException(VUE_CONTEXT *, uint16_t);
static int      cpuRead          (VUE_CONTEXT *, VUE_ACCESS *);
static uint32_t cpuSTSR          (VUE_CONTEXT *, int);
static int      cpuWrite         (VUE_CONTEXT *, VUE_ACCESS *);

#include "bus.c"
#include "instructions.c"
#include "instructiondefs.c"
#include "cpu.c"



/*****************************************************************************
 *                               API Functions                               *
 *****************************************************************************/

/* Determine whether a particular CPU status condition is met */
int vueCheckCondition(VUE_CONTEXT *vb, int id) {
    return cpuCheckCondition(vb, id);
}

/* Process one emulation step */
int vueEmulate(VUE_CONTEXT *vb, int32_t *cycles) {
    int break_code; /* Emulation break request */

    /* Keep processing while there are still cycles left */
    for (; *cycles > 0; *cycles -= vb->cycles) {
        vb->cycles = 0;

        /* Process CPU if not halting */
        break_code = cpuEmulate(vb);
        if (break_code) return break_code;

        /* The CPU is halting */
        /* Checking cpu.halt is wrong because cycles may still have elapsed */
        if (vb->cycles == 0) {
            /* vb->cycles = min(*cycles, until_interrupt); */
        }

        /* Process hardware components */
        /* vipEmulate(vb); */
        /* vsuEmulate(vb); */
        /* lnkEmulate(vb); */
        /* padEmulate(vb); */
        /* tmrEmulate(vb); */

    }

    return 0;
}

/* Fetch and decode an instruction from state memory */
void vueFetch(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint32_t address) {

    /* Restrict the address to a valid value */
    address &= (int32_t) -2;

    /* Fetch the instruction */
    cpuFetch16(vb, inst, address);
    if (inst->size == 4)
        cpuFetch32(vb, inst, address);

    /* Decode the instruction */
    cpuDecode(vb, inst, address);
}

/* Retrieve a value from a system register */
uint32_t vueGetSystemRegister(VUE_CONTEXT *vb, int id) {
    return cpuSTSR(vb, id);
}

/* Initialize a Virtual Boy state context for use */
int vueInitialize(VUE_CONTEXT *vb, uint8_t *rom, uint32_t rom_size,
    uint8_t *sram, uint32_t sram_size) {

    /* Error checking */
    if (rom == NULL || rom_size < 1024 || !vueIsPowerOfTwo(rom_size))
        return VUE_BADARG;
    if (sram && !vueIsPowerOfTwo(sram_size))
        return VUE_BADARG;

    /* Configure cartridge */
    vb->cartridge.rom      = rom;
    vb->cartridge.rom_size = rom_size;
    vb->cartridge.ram      = sram;
    vb->cartridge.ram_size = sram_size;

    return VUE_NOERROR;
}

/* Read a value from the CPU bus */
void vueRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busRead(vb, access);
}

/* Perform a system reset on a Virtual Boy state context */
void vueReset(VUE_CONTEXT *vb) {
    uint8_t  *bytes;     /* Memory pointer to vb */
    uint8_t  *rom;       /* Cartridge ROM */
    uint32_t  rom_size;  /* Size in bytes of cartridge ROM */
    uint8_t  *sram;      /* Cartridge RAM */
    uint32_t  sram_size; /* Size in bytes of cartridge RAM */
    uint32_t  x;         /* Iterator */

    /* Preserve the contents of the cartridge configuration */
    rom       = vb->cartridge.rom;
    rom_size  = vb->cartridge.rom_size;
    sram      = vb->cartridge.ram;
    sram_size = vb->cartridge.ram_size;

    /* Clear all memory to zeroes */
    bytes = (uint8_t *) vb;
    for (x = 0; x < sizeof (VUE_CONTEXT); x++)
        bytes[x] = 0;

    /* Restore cartridge configuration */
    vb->cartridge.rom      = rom;
    vb->cartridge.rom_size = rom_size;
    vb->cartridge.ram      = sram;
    vb->cartridge.ram_size = sram_size;

    /* Initialize CPU */
    vb->cpu.pc       = (int32_t) -1 << 4; /* 0xFFFFFFF0 */
    vb->cpu.psw.np   = 1;
    vb->cpu.ecr.eicc = 0xFFF0;
}

/* Set a value in a system register */
/* This function can modify ECR */
uint32_t vueSetSystemRegister(VUE_CONTEXT *vb, int id, uint32_t value) {

    /* Special processing for ECR */
    if (id == VUE_ECR) {
        vb->cpu.ecr.eicc = (value >>  0) & 0xFFFF;
        vb->cpu.ecr.fecc = (value >> 16) & 0xFFFF;
        return value;
    }

    /* Set the new value, skipping access callbacks on CHCW operations */
    cpuLDSR(vb, id, value, VUE_FALSE);

    /* Return the actual value stored in the system register */
    return cpuSTSR(vb, id);
}

/* Write a value to the CPU bus */
void vueWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWrite(vb, access);
}

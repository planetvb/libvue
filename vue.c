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



#define VUEAPI
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



/*****************************************************************************
 *                                  Macros                                   *
 *****************************************************************************/

/* Sign-extend a value that is some number of bits in size */
#define vueSignExtend(x, bits) (x & 1 << bits - 1 ? x | -1 << bits : x)



/*****************************************************************************
 *                           Sub-Library Includes                            *
 *****************************************************************************/

/* Forward references */
static int cpuRead (VUE_CONTEXT *, VUE_ACCESS *);
static int cpuWrite(VUE_CONTEXT *, VUE_ACCESS *);

#include "bus.c"
#include "instructions.c"
#include "instructiondefs.c"
#include "cpu.c"



/*****************************************************************************
 *                             Non-API Functions                             *
 *****************************************************************************/

/* Verify whether an access format is valid */
static int vueValidateFormat(uint8_t format) {
    return (
        format == VUE_S8  ||
        format == VUE_U8  ||
        format == VUE_S16 ||
        format == VUE_U16 ||
        format == VUE_32
    ) ? 1 : 0;
}



/*****************************************************************************
 *                               API Functions                               *
 *****************************************************************************/

/* Determine whether a particular status condition is met */
int vueCheckCondition(VUE_CONTEXT *vb, int id) {

    /* Select operation by condition ID */
    switch (id) {
        case VUE_V:  return vb->cpu.psw.ov;
        case VUE_C:  return vb->cpu.psw.cy;
        case VUE_Z:  return vb->cpu.psw.z;
        case VUE_NH: return vb->cpu.psw.cy | vb->cpu.psw.z;
        case VUE_N:  return vb->cpu.psw.s;
        case VUE_T:  return 1;
        case VUE_LT: return vb->cpu.psw.s ^ vb->cpu.psw.ov;
        case VUE_LE: return vb->cpu.psw.s ^ vb->cpu.psw.ov | vb->cpu.psw.z;
        case VUE_NV: return vb->cpu.psw.ov ^ 1;
        case VUE_NC: return vb->cpu.psw.cy ^ 1;
        case VUE_NZ: return vb->cpu.psw.z ^ 1;
        case VUE_H:  return (vb->cpu.psw.cy | vb->cpu.psw.z) ^ 1;
        case VUE_P:  return vb->cpu.psw.s ^ 1;
        case VUE_F:  return 0;
        case VUE_GE: return vb->cpu.psw.s ^ vb->cpu.psw.ov ^ 1;
        case VUE_GT: return (vb->cpu.psw.s ^ vb->cpu.psw.ov | vb->cpu.psw.z)^1;
        default:;
    }

    /* Invalid condition ID */
    return 0;
}

/* Perform all emulation tasks for some number of CPU cycles    */
/* Returns the application-supplied break code, or zero if none */
int vueEmulate(VUE_CONTEXT *vb, int32_t *cycles) {
    int     break_code;  /* Application-supplied emulation break code */
    int32_t cycles_this; /* CPU cycles taken by current iteration     */

    /* Keep processing until all cycles have been processed */
    for (; *cycles > 0; *cycles = *cycles - vb->cpu.cycles) {

        /* Reset CPU cycle counter */
        vb->cpu.cycles = 0;

        /* CPU operations if not halting */
        if (!vb->cpu.halt) {

            /* Process CPU */
            break_code = cpuEmulate(vb);

            /* An emulation break was requested */
            if (break_code)
                return break_code;
        }

        /* CPU is halting */
        else {
          /* cycles_this = min(until_interrupt, *cycles) */
        }

        /* Process hardware components */
        /* vipEmulate(vb); */
        /* vsuEmulate(vb); */
        /* lnkEmulate(vb); */
        /* padEmulate(vb); */
        /* tmrEmulate(vb); */

    }

    /* No emulation break was requested */
    return 0;
}

/* Fetch and decode an instruction from the CPU bus */
void vueFetch(VUE_CONTEXT *vb, uint32_t address, VUE_INSTRUCTION *inst) {
    int32_t        cycles;  /* Initial CPU cycle count      */
    VUE_ACCESSPROC onread;  /* Initial read access handler  */
    VUE_ACCESSPROC onwrite; /* Initial write access handler */

    /* Temporarily disable access handlers and cycle counter */
    cycles  = vb->cpu.cycles;
    onread  = vb->debug.onread;  vb->debug.onread  = NULL;
    onwrite = vb->debug.onwrite; vb->debug.onwrite = NULL;

    /* Fetch and decode the instruction */
    cpuFetch16(vb, inst, address);
    if (inst->size == 4)
        cpuFetch32(vb, inst, address);
    cpuDecode(NULL, inst, 0);

    /* Restore access handlers and cycle counter */
    vb->cpu.cycles    = cycles;
    vb->debug.onread  = onread;
    vb->debug.onwrite = onwrite;
}

/* Retrieve the value of a CPU system register given its numeric ID */
uint32_t vueGetSystemRegister(VUE_CONTEXT *vb, int id) {

    /* Select the system register by its ID */
    switch (id) {
        case VUE_EIPC:  return vb->cpu.eipc;
        case VUE_EIPSW: return vb->cpu.eipsw;
        case VUE_FEPC:  return vb->cpu.fepc;
        case VUE_FEPSW: return vb->cpu.fepsw;
        case VUE_ECR:   return vb->cpu.ecr;
        case VUE_PIR:   return PIR;
        case VUE_TKCW:  return TKCW;
        case VUE_CHCW:  return vb->cpu.chcw;
        case VUE_ADTRE: return vb->cpu.adtre;
        case VUE_SR29:  return vb->cpu.sr29;
        case VUE_SR30:  return SR30;
        case VUE_SR31:  return vb->cpu.sr31;
        case VUE_PSW:
            return
                (uint32_t) vb->cpu.psw.z      <<  0 |
                (uint32_t) vb->cpu.psw.s      <<  1 |
                (uint32_t) vb->cpu.psw.ov     <<  2 |
                (uint32_t) vb->cpu.psw.cy     <<  3 |
                (uint32_t) vb->cpu.psw.fpr    <<  4 |
                (uint32_t) vb->cpu.psw.fud    <<  5 |
                (uint32_t) vb->cpu.psw.fov    <<  6 |
                (uint32_t) vb->cpu.psw.fzd    <<  7 |
                (uint32_t) vb->cpu.psw.fiv    <<  8 |
                (uint32_t) vb->cpu.psw.fro    <<  9 |
                (uint32_t) vb->cpu.psw.id     << 12 |
                (uint32_t) vb->cpu.psw.ae     << 13 |
                (uint32_t) vb->cpu.psw.ep     << 14 |
                (uint32_t) vb->cpu.psw.np     << 15 |
                (uint32_t) vb->cpu.psw.i & 15 << 16
            ;
        default:;
    }

    /* An invalid system register ID was specified */
    return 0;
}

/* Performs a read access on the CPU bus */
void vueRead(VUE_CONTEXT *vb, VUE_ACCESS *access, int internal) {
    int32_t cycles; /* CPU cycles taken by bus access */

    /* Error checking */
    if (!vueValidateFormat(access->format))
        return;

    /* Restrict the address based on the data format */
    access->address &= -(access->format >> 3 & 3);

    /* Perform the bus access */
    cycles = busRead(vb, access);

    /* Update CPU cycle counter for internal accesses */
    if (internal)
        vb->cpu.cycles += cycles;
}

/* Specify a new value for a CPU system register given its numeric ID */
/* Returns the actual value written to the system register            */
/* This function is allowed to write to ECR                           */
uint32_t vueSetSystemRegister(VUE_CONTEXT *vb, int id, uint32_t value) {

    /* Select the system register by its ID */
    switch (id) {
        case VUE_EIPC:  return vb->cpu.eipc  = value & -2;
        case VUE_EIPSW: return vb->cpu.eipsw = value;
        case VUE_FEPC:  return vb->cpu.fepc  = value & -2;
        case VUE_FEPSW: return vb->cpu.fepsw = value;
        case VUE_ECR:   return vb->cpu.ecr   = value;
        case VUE_PIR:   return PIR;
        case VUE_TKCW:  return TKCW;
        case VUE_CHCW:  return cpuCacheControl(vb, value);
        case VUE_ADTRE: return vb->cpu.adtre = value & -2;
        case VUE_SR29:  return vb->cpu.sr29  = value;
        case VUE_SR30:  return SR30;
        case VUE_SR31:  return vb->cpu.sr31  = value & 1;
        case VUE_PSW:
            vb->cpu.psw.z   = value >>  0 &  1;
            vb->cpu.psw.s   = value >>  1 &  1;
            vb->cpu.psw.ov  = value >>  2 &  1;
            vb->cpu.psw.cy  = value >>  3 &  1;
            vb->cpu.psw.fpr = value >>  4 &  1;
            vb->cpu.psw.fud = value >>  5 &  1;
            vb->cpu.psw.fov = value >>  6 &  1;
            vb->cpu.psw.fzd = value >>  7 &  1;
            vb->cpu.psw.fiv = value >>  8 &  1;
            vb->cpu.psw.fro = value >>  9 &  1;
            vb->cpu.psw.id  = value >> 12 &  1;
            vb->cpu.psw.ae  = value >> 13 &  1;
            vb->cpu.psw.ep  = value >> 14 &  1;
            vb->cpu.psw.np  = value >> 15 &  1;
            vb->cpu.psw.i   = value >> 16 & 15;
            return value & 0x000FF3FF;
        default:;
    }

    /* An invalid system register ID was specified */
    return 0;
}

/* Performs a write access on the CPU bus */
void vueWrite(VUE_CONTEXT *vb, VUE_ACCESS *access, int internal) {
    int32_t cycles; /* CPU cycles taken by bus access */

    /* Error checking */
    if (!vueValidateFormat(access->format))
        return;

    /* Restrict the address based on the data format */
    access->address &= -(access->format >> 3 & 3);

    /* Perform the bus access */
    cycles = busWrite(vb, access);

    /* Update CPU cycle counter for internal accesses */
    if (internal)
        vb->cpu.cycles += cycles;
}

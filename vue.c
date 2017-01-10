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
 *                           Sub-Library Includes                            *
 *****************************************************************************/

#include "bus.c"
#include "cpu.c"



/*****************************************************************************
 *                               API Functions                               *
 *****************************************************************************/

/* Retrieve the value of a CPU system register given its numeric ID */
uint32_t vueGetSystemRegister(VUE_CONTEXT *vb, int id) {

    /* Select the system register by its ID */
    switch (id) {
        case VUE_EIPC:  return vb->cpu.eipc;
        case VUE_EIPSW: return vb->cpu.eipsw;
        case VUE_FEPC:  return vb->cpu.fepc;
        case VUE_FEPSW: return vb->cpu.fepsw;
        case VUE_ECR:   return vb->cpu.ecr;
        case VUE_PSW:   return vb->cpu.psw;
        case VUE_PIR:   return PIR;
        case VUE_TKCW:  return TKCW;
        case VUE_CHCW:  return vb->cpu.chcw;
        case VUE_ADTRE: return vb->cpu.adtre;
        case VUE_SR29:  return vb->cpu.sr29;
        case VUE_SR30:  return SR30;
        case VUE_SR31:  return vb->cpu.sr31;
        default:;
    }

    /* An invalid system register ID was specified */
    return 0;
}

/* Performs a read access on the CPU bus */
void vueRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Error checking */
    if (access->format > VUE_32)
        return;

    /* Restrict the address based on the data format */
    access->address &= -(access->format >> 3 & 3);

    /* Perform the bus access */
    busRead(vb, access);
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
        case VUE_PSW:   return vb->cpu.psw   = value & 0x000FF3FF;
        case VUE_PIR:   return PIR;
        case VUE_TKCW:  return TKCW;
        case VUE_CHCW:  return cpuCacheControl(vb, value);
        case VUE_ADTRE: return vb->cpu.adtre = value & -2;
        case VUE_SR29:  return vb->cpu.sr29  = value;
        case VUE_SR30:  return SR30;
        case VUE_SR31:  return vb->cpu.sr31  = value & 1;
        default:;
    }

    /* An invalid system register ID was specified */
    return 0;
}


/* Performs a write access on the CPU bus */
void vueWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Error checking */
    if (access->format > VUE_32)
        return;

    /* Restrict the address based on the data format */
    access->address &= -(access->format >> 3 & 3);

    /* Perform the bus access */
    busWrite(vb, access);
}

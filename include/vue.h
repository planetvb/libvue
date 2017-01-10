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



#ifndef __VUE_H__
#define __VUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VUEAPI
#define VUEAPI extern
#endif



#include <stdint.h>



/*****************************************************************************
 *                                 Constants                                 *
 *****************************************************************************/

/* Access formats */
#define VUE_S8  0x88
#define VUE_U8  0x08
#define VUE_S16 0x90
#define VUE_U16 0x10
#define VUE_32  0x20

/* CPU pipeline stages */
#define VUE_DECODE    0x02
#define VUE_EXCEPTION 0x04
#define VUE_EXECUTE   0x03
#define VUE_FETCH     0x01
#define VUE_FETCH8    0x01
#define VUE_FETCH16   0x11

/* System register IDs */
#define VUE_EIPC   0
#define VUE_EIPSW  1
#define VUE_FEPC   3
#define VUE_FEPSW  4
#define VUE_ECR    5
#define VUE_PSW    6
#define VUE_PIR    7
#define VUE_TKCW   8
#define VUE_CHCW  24
#define VUE_ADTRE 25
#define VUE_SR29  29
#define VUE_SR30  30
#define VUE_SR31  31



/*****************************************************************************
 *                                   Types                                   *
 *****************************************************************************/

/* Forward references */
typedef struct VUE_ACCESS  VUE_ACCESS;
typedef struct VUE_CONTEXT VUE_CONTEXT;

/* Function pointers */
typedef int (*VUE_ACCESSPROC)(VUE_CONTEXT *, VUE_ACCESS *);

/* Descriptor for bus accesses */
struct VUE_ACCESS {
    uint32_t address; /* Bus address to access                       */
    int32_t  value;   /* Value to write, or receives value read      */
    int8_t   format;  /* Data size/format                            */
    uint8_t  cycles;  /* Additional CPU cycles taken by bus activity */
};

/* Top-level emulation state context */
struct VUE_CONTEXT {

    /* Debug settings */
    struct {
        VUE_ACCESSPROC onread;  /* Called during read accesses  */
        VUE_ACCESSPROC onwrite; /* Called during write accesses */
    } debug;

    /* Cartridge RAM state information */
    struct {
        uint8_t  *data; /* Binary byte data        */
        uint32_t  size; /* Size, in bytes, of data */
    } cart_ram;

    /* Cartridge ROM state information */
    struct {
        uint8_t  *data; /* Binary byte data        */
        uint32_t  size; /* Size, in bytes, of data */
    } cart_rom;

    /* CPU state information */
    struct {

        /* System registers */
        uint32_t psw;   /* Program Status Word                 */
        uint32_t eipc;  /* Exception/Interrupt restore PC      */
        uint32_t eipsw; /* Exception/Interrupt restore PSW     */
        uint32_t fepc;  /* Duplexed Exception restore PC       */
        uint32_t fepsw; /* Duplexed Exception restore PSW      */
        uint32_t ecr;   /* Exception Cause Register            */
        uint32_t chcw;  /* Cache Control Word                  */
        uint32_t adtre; /* Address Trap Register for Execution */
        uint32_t sr29;  /* "System Register 29"                */
        uint32_t sr31;  /* "System Register 31"                */

        /* Program registers */
        uint32_t pc;            /* Program counter   */
        int32_t  registers[32]; /* Program registers */

        /* Miscellaneous state information */
        uint8_t stage; /* Current pipeline stage */

    } cpu;

    /* WRAM state information */
    uint8_t wram[0x10000];

};



/*****************************************************************************
 *                            Function Prototypes                            *
 *****************************************************************************/

VUEAPI uint32_t vueGetSystemRegister(VUE_CONTEXT *vb, int id);
VUEAPI void     vueRead             (VUE_CONTEXT *vb, VUE_ACCESS *access);
VUEAPI uint32_t vueSetSystemRegister(VUE_CONTEXT *vb, int id, uint32_t value);
VUEAPI void     vueWrite            (VUE_CONTEXT *vb, VUE_ACCESS *access);



#ifdef __cplusplus
}
#endif

#endif /* __VUE_H__ */

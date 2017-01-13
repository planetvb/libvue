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



/* Aliases for future-implemented functions */
#define vipRead  busReadNull
#define vsuRead  busReadNull
#define hwcRead  busReadNull
#define vipWrite busWriteNull
#define vsuWrite busWriteNull
#define hwcWrite busWriteNull



/*****************************************************************************
 *                                   Types                                   *
 *****************************************************************************/

/* Function pointer for read and write handlers */
typedef int32_t (*ROUTE)(VUE_CONTEXT *, VUE_ACCESS *);



/*****************************************************************************
 *                              Read Functions                               *
 *****************************************************************************/

/* Common memory-based read handler */
static void busReadMemory(VUE_ACCESS *access, uint8_t *data, uint32_t size) {
    uint32_t offset;    /* Buffer offset to access */
    int      data_size; /* Size of accessed data unit */

    /* Resolve the actual buffer offset to access */
    offset = access->address & (size - 1);

    /* Parse the access format */
    data_size = access->format & 0x7F;

    /* Read data according to data size */
    switch (data_size) {
        case 8:
            access->value =
                data[offset]
            ; break;
        case 16:
            access->value =
                data[offset    ]      |
                data[offset + 1] << 8
            ; break;
        case 32:
            access->value =
                data[offset    ]       |
                data[offset + 1] <<  8 |
                data[offset + 2] << 16 |
                data[offset + 3] << 24
            ; break;
        default:;
    }

    /* Sign-extend if appropriate */
    if (access->format & 0x80)
        access->value = vueSignExtend(access->value, data_size);
}

/* Read a value from cartridge RAM */
/* RESEARCH: How many cycles does this take? */
static int32_t busReadCartRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the common memory read routine */
    if (vb->cart.ram_size != 0)
        busReadMemory(access, vb->cart.ram, vb->cart.ram_size);

    /* No cartridge RAM is present */
    else access->value = 0;

    /* Return the number of cycles taken */
    return 0;
}

/* Read a value from cartridge ROM */
/* RESEARCH: How many cycles does this take? */
static int32_t busReadCartROM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the common memory read routine */
    busReadMemory(access, vb->cart.rom, vb->cart.rom_size);

    /* Return the number of cycles taken */
    return 0;
}

/* Unconfigured read handler for any data format */
/* RESEARCH: How many cycles does this take? */
static int32_t busReadNull(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Suppress unused parameter warning */
    vb = vb;

    /* Load a zero for unmapped addresses */
    access->value = 0;

    /* Return the number of cycles taken */
    return 0;
}

/* Read a value from WRAM */
/* RESEARCH: How many cycles does this take? */
static int32_t busReadWRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the common memory read routine */
    busReadMemory(access, vb->wram, 0x10000);

    /* Return the number of cycles taken */
    return 0;
}



/*****************************************************************************
 *                              Write Functions                              *
 *****************************************************************************/

/* Common memory-based write handler */
static void busWriteMemory(VUE_ACCESS *access, uint8_t *data, uint32_t size) {
    uint32_t offset;    /* Buffer offset to access */
    int      data_size; /* Size of accessed data unit */

    /* Resolve the actual buffer offset to access */
    offset = access->address & (size - 1);

    /* Parse the access format */
    data_size = access->format & 0x7F;

    /* Write data according to data size */
    switch (data_size) {
        case 8:
            data[offset] = access->value & 0xFF;
            break;
        case 16:
            data[offset + 0] = access->value >> 0 & 0xFF;
            data[offset + 1] = access->value >> 8 & 0xFF;
            break;
        case 32:
            data[offset + 0] = access->value >>  0 & 0xFF;
            data[offset + 1] = access->value >>  8 & 0xFF;
            data[offset + 2] = access->value >> 16 & 0xFF;
            data[offset + 3] = access->value >> 24 & 0xFF;
            break;
        default:;
    }
}

/* Write a value to cartridge RAM */
/* RESEARCH: How many cycles does this take? */
static int32_t busWriteCartRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the common memory write routine */
    if (vb->cart.ram_size != 0)
        busWriteMemory(access, vb->cart.ram, vb->cart.ram_size);

    /* Return the number of cycles taken */
    return 0;
}

/* Write a value to cartridge ROM */
/* RESEARCH: How many cycles does this take? */
static int32_t busWriteCartROM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the common memory write routine */
    busWriteMemory(access, vb->cart.rom, vb->cart.rom_size);

    /* Return the number of cycles taken */
    return 0;
}

/* Unconfigured write handler for any data format */
/* RESEARCH: How many cycles does this take? */
static int32_t busWriteNull(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Suppress unused parameter warnings */
    vb     = vb;
    access = access;

    /* Take no action */

    /* Return the number of cycles taken */
    return 0;
}

/* Write a value to WRAM */
/* RESEARCH: How many cycles does this take? */
static int32_t busWriteWRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the common memory write routine */
    busWriteMemory(access, vb->wram, 0x10000);

    /* Return the number of cycles taken */
    return 0;
}



/*****************************************************************************
 *                             Library Functions                             *
 *****************************************************************************/

/* Read a value from the CPU bus */
/* The access's format and address must be valid */
static int32_t busRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Address routing table */
    static const ROUTE ROUTES[] = {
        &vipRead,     &vsuRead,     &hwcRead,        &busReadNull,
        &busReadNull, &busReadWRAM, &busReadCartRAM, &busReadCartROM
    };

    /* Route the access to the appropriate module */
    return ROUTES[access->address >> 24 & 7](vb, access);
}

/* Write a value to the CPU bus */
/* The access's format and address must be valid */
static int32_t busWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Address routing table */
    static const ROUTE ROUTES[] = {
        &vipWrite,     &vsuWrite,     &hwcWrite,        &busWriteNull,
        &busWriteNull, &busWriteWRAM, &busWriteCartRAM, &busWriteCartROM
    };

    /* Route the access to the appropriate module */
    return ROUTES[access->address >> 24 & 7](vb, access);
}



#endif /* VUEAPI */

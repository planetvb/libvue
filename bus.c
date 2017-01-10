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
 *                                   Types                                   *
 *****************************************************************************/

/* Function pointer for read and write handlers */
typedef void (*ROUTE)(VUE_CONTEXT *, VUE_ACCESS *);



/*****************************************************************************
 *                              Read Functions                               *
 *****************************************************************************/

/* Common memory-based read handler */
static void busReadMemory(VUE_ACCESS *access, uint8_t *data, uint32_t size) {
    uint32_t offset;      /* Buffer offset to access    */
    int      data_size;   /* Size of accessed data unit */
    int      sign_extend; /* Sign extension is required */

    /* Resolve the actual buffer offset to access */
    offset = access->address & size - 1;

    /* Parse the access format */
    data_size   = access->format & 0x7F;
    sign_extend = access->format >> 7 & 1;

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
    if (sign_extend)
        access->value |= (int32_t) -1 << data_size;
}

/* Read a value from cartridge RAM */
static void busReadCartRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* No cartridge RAM is present */
    if (vb->cart_ram.size == 0) {
        access->value = 0;
        return;
    }

    /* Use the common memory read routine */
    busReadMemory(access, vb->cart_ram.data, vb->cart_ram.size);
}

/* Read a value from cartridge ROM */
static void busReadCartROM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadMemory(access, vb->cart_rom.data, vb->cart_rom.size);
}

/* Unconfigured read handler for any data format */
static void busReadNull(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    access->value = 0;
}

/* Read a value from WRAM */
static void busReadWRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadMemory(access, vb->wram, 0x10000);
}



/*****************************************************************************
 *                              Write Functions                              *
 *****************************************************************************/

/* Common memory-based write handler */
static void busWriteMemory(VUE_ACCESS *access, uint8_t *data, uint32_t size) {
    uint32_t offset;     /* Buffer offset to access    */
    int      data_size;  /* Size of accessed data unit */

    /* Resolve the actual buffer offset to access */
    offset = access->address & size - 1;

    /* Parse the access format */
    data_size = access->format & 0x7F;

    /* Common data for all data sizes */
    data[offset] = access->value & 0xFF;

    /* Data absent from 8-bit writes */
    if (data_size != 8) {
        data[offset + 1] = access->value >> 8 & 0xFF;

        /* Data only for 32-bit writes */
        if (data_size == 32) {
            data[offset + 2] = access->value >> 16 & 0xFF;
            data[offset + 3] = access->value >> 24 & 0xFF;
        }
    }
}

/* Write a value to cartridge RAM */
static void busWriteCartRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* No cartridge RAM is present */
    if (vb->cart_ram.size == 0)
        return;

    /* Use the common memory write routine */
    busWriteMemory(access, vb->cart_ram.data, vb->cart_ram.size);
}

/* Write a value to cartridge ROM */
static void busWriteCartROM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteMemory(access, vb->cart_rom.data, vb->cart_rom.size);
}

/* Unconfigured write handler for any data format */
static void busWriteNull(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    /* Take no action */
}

/* Write a value to WRAM */
static void busWriteWRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteMemory(access, vb->wram, 0x10000);
}



/*****************************************************************************
 *                             Library Functions                             *
 *****************************************************************************/

/* Read a value from the CPU bus */
/* The access's format and address must be valid */
static void busRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Address routing table */
    static const ROUTE ROUTES[] = {
        &busReadNull, &busReadNull, &busReadNull,    &busReadNull,
        &busReadNull, &busReadWRAM, &busReadCartRAM, &busReadCartROM
    };

    /* Initialize the cycle counter */
    access->cycles = 0;

    /* Route the access to the appropriate module */
    ROUTES[access->address >> 24 & 7](vb, access);
}

/* Write a value to the CPU bus */
/* The access's format and address must be valid */
static void busWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Address routing table */
    static const ROUTE ROUTES[] = {
        &busWriteNull, &busWriteNull, &busWriteNull,    &busWriteNull, 
        &busWriteNull, &busWriteWRAM, &busWriteCartRAM, &busWriteCartROM
    };

    /* Initialize the cycle counter */
    access->cycles = 0;

    /* Route the access to the appropriate module */
    ROUTES[access->address >> 24 & 7](vb, access);
}



#endif /* VUEAPI */

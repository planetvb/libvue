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

/* This file is included by vue.c and should not be compiled on its own. */

#ifdef VUE_API



/*****************************************************************************
 *                                   Types                                   *
 *****************************************************************************/

/* Handler for bus accesses */
typedef void (*ROUTEPROC)(VUE_CONTEXT *, VUE_ACCESS *);



/*****************************************************************************
 *                              Read Functions                               *
 *****************************************************************************/

/* Read a value from state memory */
static void busReadMemory(VUE_ACCESS *access, uint8_t *data,
    uint32_t data_size) {
    uint32_t offset;      /* Position in data to read from */
    uint8_t  sign_extend; /* The read value should be sign-extended */
    uint8_t  value_size;  /* Size in bytes of the value to read */

    /* Pre-calculate some values */
    offset      = access->address & (data_size - 1);
    sign_extend = access->format  & 0x80;
    value_size  = access->format  & 0x7F;

    /* Read the value depending on its data size */
    switch (value_size) {
        case 8:
            access->value = (int32_t) data[offset];
            break;
        case 16:
            access->value =
                 (int32_t) data[offset] |
                ((int32_t) data[offset + 1] << 8);
            break;
        case 32:
            access->value =
                 (int32_t) data[offset] |
                ((int32_t) data[offset + 1] <<  8) |
                ((int32_t) data[offset + 2] << 16) |
                ((int32_t) data[offset + 3] << 24);
            break;
        default:;
    }

    /* Sign-extend the value if needed */
    if (sign_extend)
        access->value = vueSignExtend(access->value, value_size);
}

/* Read a value from unmapped memory */
static void busReadNull(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    vb = vb;           /* Suppress unused parameter warning */
    access->value = 0; /* Simulate a loaded zero */
}

/* Read a value from cartridge expansion */
static void busReadCartExp(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadNull(vb, access);
}

/* Read a value from cartridge RAM */
static void busReadCartRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadMemory(access, vb->cartridge.ram, vb->cartridge.ram_size);
}

/* Read a value from cartridge ROM */
static void busReadCartROM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadMemory(access, vb->cartridge.rom, vb->cartridge.rom_size);
}

/* Read a value from hardware control */
static void busReadHWCtl(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadNull(vb, access);
}

/* Read a value from the VIP */
static void busReadVIP(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadNull(vb, access);
}

/* Read a value from the VSU */
static void busReadVSU(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadNull(vb, access);
}

/* Read a value from WRAM */
static void busReadWRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busReadMemory(access, vb->wram, 0x10000);
}



/*****************************************************************************
 *                              Write Functions                              *
 *****************************************************************************/

/* Write a value to state memory */
static void busWriteMemory(VUE_ACCESS *access, uint8_t *data,
    uint32_t data_size) {
    uint32_t offset;     /* Position in data to write to */
    uint8_t  value_size; /* Size in bytes of the value to write */

    /* Pre-calculate some values */
    offset     = access->address & (data_size - 1);
    value_size = access->format  & 0x7F;

    /* Write the value depending on its data size */
    switch (value_size) {
        case 8:
            data[offset] = (uint8_t) (access->value & 0xFF);
            break;
        case 16:
            data[offset]     = (uint8_t) ( access->value       & 0xFF);
            data[offset + 1] = (uint8_t) ((access->value >> 8) & 0xFF);
            break;
        case 32:
            data[offset]     = (uint8_t) ( access->value        & 0xFF);
            data[offset + 1] = (uint8_t) ((access->value >>  8) & 0xFF);
            data[offset + 2] = (uint8_t) ((access->value >> 16) & 0xFF);
            data[offset + 3] = (uint8_t) ((access->value >> 24) & 0xFF);
            break;
        default:;
    }
}

/* Write a value to unmapped memory */
static void busWriteNull(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    /* Take no action. Suppress unused parameter warnings */
    vb = vb;
    access = access;
}

/* Write a value to cartridge expansion */
static void busWriteCartExp(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteNull(vb, access);
}

/* Write a value to cartridge RAM */
static void busWriteCartRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteMemory(access, vb->cartridge.ram, vb->cartridge.ram_size);
}

/* Write a value to cartridge ROM */
static void busWriteCartROM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteNull(vb, access);
}

/* Write a value to hardware control */
static void busWriteHWCtl(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteNull(vb, access);
}

/* Write a value to the VIP */
static void busWriteVIP(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteNull(vb, access);
}

/* Write a value to the VSU */
static void busWriteVSU(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteNull(vb, access);
}

/* Write a value to WRAM */
static void busWriteWRAM(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    busWriteMemory(access, vb->wram, 0x10000);
}

/* Write fatal exception debugging information */
static void busWriteFatal(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* All writes are 32-bit */
    access->format = VUE_32;

    /* Write the exception code */
    access->address = 0x00000000;
    busWriteVIP(vb, access);

    /* Write PSW */
    access->address = 0x00000004;
    access->value   = vueSign(cpuSTSR(vb, VUE_PSW));
    busWriteVIP(vb, access);

    /* Write PC */
    access->address = 0x00000008;
    access->value   = vueSign(vb->cpu.pc);
    busWriteVIP(vb, access);
}



/*****************************************************************************
 *                             Library Functions                             *
 *****************************************************************************/

/* Read a value from the bus */
static void busRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Bus routing lookup table */
    static const ROUTEPROC ROUTES[] = {
        &busReadVIP,     &busReadVSU,  &busReadHWCtl,   &busReadNull,
        &busReadCartExp, &busReadWRAM, &busReadCartRAM, &busReadCartROM
    };

    /* Restrict the address based on the access format */
    access->address = vueMaskAddress(access->address, access->format);

    /* Route the access to the appropriate handler */
    ROUTES[(access->address >> 24) & 7](vb, access);
}

/* Write a value to the bus */
static void busWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Bus routing lookup table */
    static const ROUTEPROC ROUTES[] = {
        &busWriteVIP,     &busWriteVSU,  &busWriteHWCtl,   &busWriteNull,
        &busWriteCartExp, &busWriteWRAM, &busWriteCartRAM, &busWriteCartROM
    };

    /* Special processing for fatal exception writes */
    if (access->format == VUE_FATAL) {
        busWriteFatal(vb, access);
        return;
    }

    /* Restrict the address based on the access format */
    access->address = vueMaskAddress(access->address, access->format);

    /* Route the access to the appropriate handler */
    ROUTES[(access->address >> 24) & 7](vb, access);
}



#endif /* VUE_API */

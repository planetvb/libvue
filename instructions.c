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

#ifdef VUEAPI



/*****************************************************************************
 *                              Format Decoders                              *
 *****************************************************************************/

/* Illegal instruction */
static void cpfIllegal(VUE_INSTRUCTION *inst) {
    /* Take no action. Suppress unused parameter warning */
    inst = inst;
}

/* Format I */
static void cpfFormatI(VUE_INSTRUCTION *inst) {
    inst->register1 = inst->bits & 0x1F;
    inst->register2 = (inst->bits >> 5) & 0x1F;
}

/* Format II */
static void cpfFormatII(VUE_INSTRUCTION *inst) {
    inst->immediate = inst->bits & 0x1F;
    inst->register2 = (inst->bits >> 5) & 0x1F;
    if (inst->sign_extend)
        inst->immediate = vueSignExtend(inst->immediate, 5);
}

/* Format III */
static void cpfFormatIII(VUE_INSTRUCTION *inst) {
    inst->displacement = vueSignExtend(inst->bits & 0x01FF, 9);
    inst->condition    = (inst->bits >> 9) & 0x0F;
}

/* Format IV */
static void cpfFormatIV(VUE_INSTRUCTION *inst) {
    inst->displacement = vueSignExtend(inst->bits & 0x03FFFFFF, 26);
}

/* Format V */
static void cpfFormatV(VUE_INSTRUCTION *inst) {
    inst->immediate = inst->bits & 0xFFFF;
    inst->register1 = (inst->bits >> 16) & 0x1F;
    inst->register2 = (inst->bits >> 21) & 0x1F;
    if (inst->sign_extend)
        inst->immediate = vueSignExtend(inst->immediate, 16);
}

/* Format VI */
static void cpfFormatVI(VUE_INSTRUCTION *inst) {
    inst->displacement = vueSignExtend(inst->bits & 0xFFFF, 16);
    inst->register1    = (inst->bits >> 16) & 0x1F;
    inst->register2    = (inst->bits >> 21) & 0x1F;
}

/* Format VII */
static void cpfFormatVII(VUE_INSTRUCTION *inst) {
    inst->subopcode = (inst->bits >> 10) & 0x3F;
    inst->register1 = (inst->bits >> 16) & 0x1F;
    inst->register2 = (inst->bits >> 21) & 0x1F;
}



/*****************************************************************************
 *                             Common Operations                             *
 *****************************************************************************/

/* Common status flags */
static void cpoFlags(VUE_CONTEXT *vb, uint32_t result) {
    vb->cpu.psw.s = (result >> 31) & 1;
    vb->cpu.psw.z = result ? 0 : 1;
}

/* Addition processor */
static int32_t cpoAdd(VUE_CONTEXT *vb, uint32_t left, uint32_t right) {
    uint32_t result; /* Operation output */

    /* Perform the operation */
    result = left + right;

    /* Calculate status flags */
    vb->cpu.psw.cy = (result < left) ? 1 : 0;
    vb->cpu.psw.ov = (((~left ^ right) & (left ^ result)) >> 31) & 1;
    cpoFlags(vb, result);

    return vueSign(result);
}

/* Arithmetic right-shift processor */
/* RESEARCH: Are shift amounts AND'd with 31? */
static int32_t cpoShArith(VUE_CONTEXT *vb, int32_t left, int32_t right) {
    int32_t result; /* Operation output */

    /* Perform the operation */
    result = right ? vueSignExtend(left >> right, 32 - right) : left;

    /* Calculate status flags */
    vb->cpu.psw.cy = right ? (left >> (right - 1)) & 1 : 0;
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, result);

    return result;
}

/* Left-shift processor */
/* RESEARCH: Are shift amounts AND'd with 31? */
static int32_t cpoShLeft(VUE_CONTEXT *vb, int32_t left, int32_t right) {
    int32_t result; /* Operation output */

    /* Perform the operation */
    result = left << right;

    /* Calculate status flags */
    vb->cpu.psw.cy = right ? (left >> (32 - right)) & 1 : 0;
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, result);

    return result;
}

/* Right-shift processor */
/* RESEARCH: Are shift amounts AND'd with 31? */
static int32_t cpoShRight(VUE_CONTEXT *vb, int32_t left, int32_t right) {
    int32_t result; /* Operation output */

    /* Perform the operation */
    result = right ? vueZeroExtend(left >> right, 32 - right) : left;

    /* Calculate status flags */
    vb->cpu.psw.cy = right ? (left >> (right - 1)) & 1 : 0;
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, result);

    return result;
}

/* Subtraction processor */
static int32_t cpoSubtract(VUE_CONTEXT *vb, uint32_t left, uint32_t right) {
    uint32_t result; /* Operation output */

    /* Perform the operation */
    result = left - right;

    /* Calculate status flags */
    vb->cpu.psw.cy = (right < left) ? 1 : 0;
    vb->cpu.psw.ov = (((left ^ right) & (left ^ result)) >> 31) & 1;
    cpoFlags(vb, result);

    return vueSign(result);
}

/* Common updates to emulation state after executing an instruction */
static int cpoUpdate(VUE_CONTEXT *vb, uint32_t cycles) {
    vb->cpu.pc += vb->instruction.size;
    vb->cycles += cycles;
    return 0; /* Provided for use as a break code */
}

/* Read a value from the bus */
static int cpoRead(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint8_t format,
    int32_t cycles) {
    VUE_ACCESS access;     /* Bus access descriptor */
    int        break_code; /* Emulation break request */

    /* Read the value */
    access.address = inst->address;
    access.cycles  = 0;
    access.format  = format;
    break_code = cpuRead(vb, &access);
    if (break_code) return break_code;

    /* Update emulation state */
    vb->cpu.registers[inst->register2] = access.value;
    return cpoUpdate(vb, cycles + access.cycles);
}

/* Write a value to the bus */
static int cpoWrite(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint8_t format,
    int32_t cycles) {
    VUE_ACCESS access;     /* Bus access descriptor */
    int        break_code; /* Emulation break request */

    /* Read the value */
    access.address = inst->address;
    access.cycles  = 0;
    access.format  = format;
    access.value   = vb->cpu.registers[inst->register2];
    break_code = cpuWrite(vb, &access);
    if (break_code) return break_code;

    /* Update emulation state */
    return cpoUpdate(vb, cycles + access.cycles);
}



/*****************************************************************************
 *                           Instruction Handlers                            *
 *****************************************************************************/

/* Illegal instruction */
static int cpiIllegal(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    inst = inst; /* Suppress unused parameter warning */
    return cpuRaiseException(vb, VUE_INVALIDOPCODE);
}

/* Add Immediate */
static int cpiADD_IMM(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoAdd(vb,
        vb->cpu.registers[inst->register2],
        inst->immediate
    );
    return cpoUpdate(vb, 1);
}

/* Add Register */
static int cpiADD_REG(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoAdd(vb,
        vb->cpu.registers[inst->register2],
        vb->cpu.registers[inst->register1]
    );
    return cpoUpdate(vb, 1);
}

/* Add Immediate */
static int cpiADDI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoAdd(vb,
        vb->cpu.registers[inst->register1],
        inst->immediate
    );
    return cpoUpdate(vb, 1);
}

/* And Register */
static int cpiAND(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Perform operation */
    vb->cpu.registers[inst->register2] &= vb->cpu.registers[inst->register1];

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, vb->cpu.registers[inst->register2]);

    return cpoUpdate(vb, 1);
}

/* And Immediate */
static int cpiANDI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Perform operation */
    vb->cpu.registers[inst->register2] &= inst->immediate;

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, vb->cpu.registers[inst->register2]);

    return cpoUpdate(vb, 1);
}

/* Branch on Condition */
static int cpiBCOND(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* The branch is taken */
    if (inst->is_true) {
        vb->cpu.pc = inst->address;
        vb->cycles += 3;
    }

    /* The branch is not taken */
    else {
        vb->cpu.pc += inst->size;
        vb->cycles += 1;
    }

    return 0;
}

/* Compare and Exchange Interlocked */
static int cpiCAXI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    VUE_ACCESS access;     /* Bus access descriptor */
    int        break_code; /* Emulation break request */

    /* Read the lock word */
    access.address = inst->address;
    access.cycles  = 0;
    access.format  = VUE_32;
    break_code = cpuRead(vb, &access);
    if (break_code) return break_code;

    /* Compare the lock word with the previous lock value */
    cpoSubtract(vb, vb->cpu.registers[inst->register2], access.value);

    /* Store a value back into the lock word address */
    if (vb->cpu.psw.z)
        access.value = vb->cpu.registers[30];
    break_code = cpuWrite(vb, &access);
    if (break_code) return break_code;

    return cpoUpdate(vb, 26 + access.cycles);
}

/* Clear Interrupt Disable Flag */
static int cpiCLI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    inst = inst;        /* Suppress unused parameter warning */
    vb->cpu.psw.id = 0; /* Update emulation state */
    return cpoUpdate(vb, 12);
}

/* Compare Immediate */
static int cpiCMP_IMM(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    cpoSubtract(vb,
        vb->cpu.registers[inst->register2],
        inst->immediate
    );
    return cpoUpdate(vb, 1);
}

/* Compare Register */
static int cpiCMP_REG(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    cpoSubtract(vb,
        vb->cpu.registers[inst->register2],
        vb->cpu.registers[inst->register1]
    );
    return cpoUpdate(vb, 1);
}

/* Divide */
static int cpiDIV(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    int32_t left;   /* Left-hand operand */
    int32_t result; /* Operation output */
    int32_t right;  /* Right-hand operand */

    /* Retrieve operation operands */
    left  = vb->cpu.registers[inst->register2];
    right = vb->cpu.registers[inst->register1];

    /* Attempting to divide by zero */
    if (right == 0)
        return cpuRaiseException(vb, VUE_ZERODIVISION);

    /* Special processing for division of minimum value by -1 */
    if (left == WORD_MIN && right == -1) {
        result                = left;
        vb->cpu.registers[30] = 0;
        vb->cpu.psw.ov        = 1;
    }

    /* Processing for regular division */
    else {
        result                = left / right;
        vb->cpu.registers[30] = left - right * result;
        vb->cpu.psw.ov        = 0;
    }

    /* Update emulation state */
    vb->cpu.registers[inst->register2] = result;

    /* Calculate status flags */
    cpoFlags(vb, result);

    return cpoUpdate(vb, 38);
}

/* Divide Unsigned */
static int cpiDIVU(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    uint32_t left;   /* Left-hand operand */
    uint32_t result; /* Operation output */
    uint32_t right;  /* Right-hand operand */

    /* Retrieve operation operands */
    left  = vb->cpu.registers[inst->register2];
    right = vb->cpu.registers[inst->register1];

    /* Attempting to divide by zero */
    if (right == 0)
        return cpuRaiseException(vb, VUE_ZERODIVISION);

    /* Perform the operation */
    result = left / right;
    vb->cpu.registers[30] = vueSign(left - right * result);
    vb->cpu.registers[inst->register2] = vueSign(result);

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, result);

    return cpoUpdate(vb, 36);
}

/* Halt */
static int cpiHALT(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    inst = inst; /* Suppress unused parameter warning */
    vb->cpu.halt = 1;
    return 0;
}

/* Input Byte */
static int cpiIN_B(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoRead(vb, inst, VUE_U8, 3);
}

/* Input Halfword */
static int cpiIN_H(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoRead(vb, inst, VUE_U16, 5);
}

/* Input Word */
static int cpiIN_W(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoRead(vb, inst, VUE_32, 5);
}

/* Jump and Link */
static int cpiJAL(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[31] = vb->cpu.pc + 4;
    vb->cpu.pc = inst->address;
    vb->cycles += 3;
    return 0;
}

/* Jump Register */
static int cpiJMP(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.pc = vb->cpu.registers[inst->register1] & (int32_t) -2;
    vb->cycles += 3;
    return 0;
}

/* Jump Relative */
static int cpiJR(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.pc  = inst->address;
    vb->cycles += 3;
    return 0;
}

/* Load Byte */
static int cpiLD_B(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoRead(vb, inst, VUE_S8, 5);
}

/* Load Halfword */
static int cpiLD_H(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoRead(vb, inst, VUE_S16, 5);
}

/* Load Word */
static int cpiLD_W(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoRead(vb, inst, VUE_32, 5);
}

/* Load to System Register */
static int cpiLDSR(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    int break_code; /* Emulation break request */

    /* Write to the system register */
    break_code = cpuLDSR(vb, inst->immediate & 15,
        vb->cpu.registers[inst->register2], VUE_TRUE);
    if (break_code) return break_code;

    return cpoUpdate(vb, 8);
}

/* Move Immediate */
static int cpiMOV_IMM(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = inst->immediate;
    return cpoUpdate(vb, 1);
}

/* Move Register */
static int cpiMOV_REG(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = vb->cpu.registers[inst->register1];
    return cpoUpdate(vb, 1);
}

/* Add Immediate (lower 16 bits) */
static int cpiMOVEA(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] =
        vb->cpu.registers[inst->register1] + inst->immediate;
    return cpoUpdate(vb, 1);
}

/* Add (upper 16 bits) */
static int cpiMOVHI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] =
        vb->cpu.registers[inst->register1] + (inst->immediate << 16);
    return cpoUpdate(vb, 1);
}

/* Multiply Halfword */
static int cpiMPYHW(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] *=
        vueSignExtend(vb->cpu.registers[inst->register1], 17);
    return cpoUpdate(vb, 9);
}

/* Multiply */
static int cpiMUL(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    int64_t left;     /* Left-hand operand */
    int64_t result;   /* Operation output */
    int32_t result32; /* Lower 32 bits of result */
    int64_t right;    /* Right-hand operand */

    /* Perform the operation */
    left     = vb->cpu.registers[inst->register2];
    right    = vb->cpu.registers[inst->register1];
    result   = left * right;
    result32 = vueSign(result & WORD_MAX);

    /* Update emulation state */
    vb->cpu.registers[30]              = vueSign((result >> 32) & WORD_MAX);
    vb->cpu.registers[inst->register2] = result32;

    /* Calculate status flags */
    vb->cpu.psw.ov = (result != result32) ? 1 : 0;
    cpoFlags(vb, result32);

    return cpoUpdate(vb, 13);
}

/* Multiply Unsigned */
static int cpiMULU(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    uint64_t left;     /* Left-hand operand */
    uint64_t result;   /* Operation output */
    uint32_t result32; /* Lower 32 bits of result */
    uint64_t right;    /* Right-hand operand */

    /* Perform the operation */
    left     = vb->cpu.registers[inst->register2];
    right    = vb->cpu.registers[inst->register1];
    result   = left * right;
    result32 = result & WORD_MAX;

    /* Update emulation state */
    vb->cpu.registers[30]              = vueSign((result >> 32) & WORD_MAX);
    vb->cpu.registers[inst->register2] = vueSign(result32);

    /* Calculate status flags */
    vb->cpu.psw.ov = (result != result32) ? 1 : 0;
    cpoFlags(vb, result32);

    return cpoUpdate(vb, 13);
}

/* Not */
static int cpiNOT(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Perform operation */
    vb->cpu.registers[inst->register2] = ~vb->cpu.registers[inst->register1];

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, vb->cpu.registers[inst->register2]);

    return cpoUpdate(vb, 1);
}

/* Or */
static int cpiOR(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Perform operation */
    vb->cpu.registers[inst->register2] |= vb->cpu.registers[inst->register1];

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, vb->cpu.registers[inst->register2]);

    return cpoUpdate(vb, 1);
}

/* Or Immediate */
static int cpiORI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Perform operation */
    vb->cpu.registers[inst->register2] |= inst->immediate;

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, vb->cpu.registers[inst->register2]);

    return cpoUpdate(vb, 1);
}

/* Output Byte */
static int cpiOUT_B(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoWrite(vb, inst, VUE_8, 4);
}

/* Output Halfword */
static int cpiOUT_H(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoWrite(vb, inst, VUE_16, 4);
}

/* Output Word */
static int cpiOUT_W(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoWrite(vb, inst, VUE_32, 4);
}

/* Return from Trap or Interrupt */
static int cpiRETI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    int      break_code; /* Emulation break request */
    uint32_t pc;         /* Restore PC */
    uint32_t psw;        /* Restore PSW */

    /* Suppress unused parameter warning */
    inst = inst;

    /* Returning from duplexed exception */
    if (vb->cpu.psw.np) {
        pc  = vb->cpu.fepc;
        psw = vb->cpu.fepsw;
    }

    /* Returning from another context */
    else {
        pc  = vb->cpu.eipc;
        psw = vb->cpu.eipsw;
    }

    /* Restore PC and PSW */
    break_code = cpuLDSR(vb, VUE_PSW, psw, VUE_TRUE);
    if (break_code) return break_code;
    vb->cpu.pc = pc;

    return cpoUpdate(vb, 10);
}

/* Reverse Bits in Word */
static int cpiREV(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    uint8_t *dest;  /* Direct access to output bytes */
    uint8_t *src;   /* Direct access to input bytes */
    int32_t  value; /* Temporary value */

    /* Bit-reversed byte lookup table */
    static const uint8_t FLIPS[] = {
        0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,
        0x70,0xF0,0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,
        0x38,0xB8,0x78,0xF8,0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,
        0x54,0xD4,0x34,0xB4,0x74,0xF4,0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,
        0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,0x02,0x82,0x42,0xC2,0x22,0xA2,
        0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,0x0A,0x8A,0x4A,0xCA,
        0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,0x06,0x86,
        0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
        0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,
        0x7E,0xFE,0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,
        0x31,0xB1,0x71,0xF1,0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,
        0x59,0xD9,0x39,0xB9,0x79,0xF9,0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,
        0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,
        0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,0x03,0x83,0x43,0xC3,
        0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,0x0B,0x8B,
        0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
        0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,
        0x77,0xF7,0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,
        0x3F,0xBF,0x7F,0xFF
    };

    /* Configure references */
    dest  = (uint8_t *) &vb->cpu.registers[inst->register2];
    src   = (uint8_t *) &value;
    value = *(int32_t *)dest;

    /* Perform operation */
    dest[0] = FLIPS[src[3]];
    dest[1] = FLIPS[src[2]];
    dest[2] = FLIPS[src[1]];
    dest[3] = FLIPS[src[0]];

    return cpoUpdate(vb, 22);
}

/* Shift Arithmetic Right by Immediate */
static int cpiSAR_IMM(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoShArith(vb,
        vb->cpu.registers[inst->register2],
        inst->immediate
    );
    return cpoUpdate(vb, 1);
}

/* Shift Arithmetic Right by Register */
static int cpiSAR_REG(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoShArith(vb,
        vb->cpu.registers[inst->register2],
        vb->cpu.registers[inst->register1]
    );
    return cpoUpdate(vb, 1);
}

/* Set Interrupt Disable Flag */
static int cpiSEI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    inst = inst;        /* Suppress unused parameter warning */
    vb->cpu.psw.id = 1; /* Update emulation state */
    return cpoUpdate(vb, 12);
}

/* Set Flag Condition */
static int cpiSETF(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] =
        cpuCheckCondition(vb, inst->immediate & 15);
    return cpoUpdate(vb, 1);
}

/* Shift Left by Immediate */
static int cpiSHL_IMM(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoShLeft(vb,
        vb->cpu.registers[inst->register2],
        inst->immediate
    );
    return cpoUpdate(vb, 1);
}

/* Shift Left by Register */
static int cpiSHL_REG(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoShLeft(vb,
        vb->cpu.registers[inst->register2],
        vb->cpu.registers[inst->register1]
    );
    return cpoUpdate(vb, 1);
}

/* Shift Right by Immediate */
static int cpiSHR_IMM(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoShRight(vb,
        vb->cpu.registers[inst->register2],
        inst->immediate
    );
    return cpoUpdate(vb, 1);
}

/* Shift Right by Register */
static int cpiSHR_REG(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoShRight(vb,
        vb->cpu.registers[inst->register2],
        vb->cpu.registers[inst->register1]
    );
    return cpoUpdate(vb, 1);
}

/* Store Byte */
static int cpiST_B(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoWrite(vb, inst, VUE_8, 4);
}

/* Store Halfword */
static int cpiST_H(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoWrite(vb, inst, VUE_16, 4);
}

/* Store Word */
static int cpiST_W(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    return cpoWrite(vb, inst, VUE_32, 4);
}

/* Store Contents of System Register */
static int cpiSTSR(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpuSTSR(vb, inst->immediate);
    return cpoUpdate(vb, 8);
}

/* Subtract */
static int cpiSUB(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    vb->cpu.registers[inst->register2] = cpoSubtract(vb,
        vb->cpu.registers[inst->register2],
        vb->cpu.registers[inst->register1]
    );
    return cpoUpdate(vb, 1);
}

/* Trap */
static int cpiTRAP(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    int break_code; /* Emulation break request */

    /* Raise an exception */
    break_code = cpuRaiseException(vb, VUE_TRAPCC + (inst->immediate & 15));
    if (break_code) return break_code;

    return cpoUpdate(vb, 15);
}

/* Exchange Byte */
static int cpiXB(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    int32_t x; /* Temporary value */

    /* Perform operation */
    x = vb->cpu.registers[inst->register2];
    vb->cpu.registers[inst->register2] =
        (x & (0xFFFF << 16)) | ((x >> 8) & 0xFF) | ((x & 0xFF) << 8);

    return cpoUpdate(vb, 6);
}

/* Exchange Halfword */
static int cpiXH(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {
    int32_t x; /* Temporary value */

    /* Perform operation */
    x = vb->cpu.registers[inst->register2];
    vb->cpu.registers[inst->register2] =
        (x & (0xFFFF << 16)) | ((x >> 16) & 0xFFFF);

    return cpoUpdate(vb, 1);
}

/* Exclusive Or */
static int cpiXOR(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Perform operation */
    vb->cpu.registers[inst->register2] ^= vb->cpu.registers[inst->register1];

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, vb->cpu.registers[inst->register2]);

    return cpoUpdate(vb, 1);
}

/* Exclusive Or Immediate */
static int cpiXORI(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Perform operation */
    vb->cpu.registers[inst->register2] ^= inst->immediate;

    /* Calculate status flags */
    vb->cpu.psw.ov = 0;
    cpoFlags(vb, vb->cpu.registers[inst->register2]);

    return cpoUpdate(vb, 1);
}



#endif /* VUEAPI */

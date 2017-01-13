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
 *                       Instruction Format Functions                        *
 *****************************************************************************/

/* Handler for illegal opcodes */
static void cpfIllegal(VUE_INSTRUCTION *inst) {

    /* Suppress unused parameter warning */
    inst = inst;

    /* Take no action */
}

/* Decoder for Format I */
static void cpfFormatI(VUE_INSTRUCTION *inst) {
    inst->register1 = inst->bits >> 0 & 31;
    inst->register2 = inst->bits >> 5 & 31;
}

/* Decoder for Format II */
static void cpfFormatII(VUE_INSTRUCTION *inst) {
    inst->immediate = inst->bits >> 0 & 31;
    inst->register2 = inst->bits >> 5 & 31;
    if (inst->sign_extend)
        inst->immediate = vueSignExtend(inst->immediate, 5);
}

/* Decoder for Format III */
static void cpfFormatIII(VUE_INSTRUCTION *inst) {
    inst->opcode       = 0x20;
    inst->displacement = inst->bits >> 0 & 0x01FF;
    inst->condition    = inst->bits >> 9 & 15;
    inst->displacement = vueSignExtend(inst->displacement, 9);
}

/* Decoder for Format IV */
static void cpfFormatIV(VUE_INSTRUCTION *inst) {
    inst->displacement = inst->bits & 0x03FFFFFF;
    inst->displacement = vueSignExtend(inst->displacement, 26);
}

/* Decoder for Format V */
static void cpfFormatV(VUE_INSTRUCTION *inst) {
    inst->immediate = inst->bits >>  0 & 0xFFFF;
    inst->register1 = inst->bits >> 16 & 31;
    inst->register2 = inst->bits >> 21 & 31;
    if (inst->sign_extend)
        inst->immediate = vueSignExtend(inst->immediate, 16);
}

/* Decoder for Format VI */
static void cpfFormatVI(VUE_INSTRUCTION *inst) {
    inst->displacement = inst->bits >>  0 & 0xFFFF;
    inst->register1    = inst->bits >> 16 & 31;
    inst->register2    = inst->bits >> 21 & 31;
    inst->displacement = vueSignExtend(inst->displacement, 16);
}

/* Decoder for Format VII */
static void cpfFormatVII(VUE_INSTRUCTION *inst) {
    inst->subopcode = inst->bits >>  0 & 63;
    inst->register1 = inst->bits >> 16 & 31;
    inst->register2 = inst->bits >> 21 & 31;
}



/*****************************************************************************
 *                       Common Instruction Functions                        *
 *****************************************************************************/

/* Addition processing */
static int32_t cpiAdd(VUE_CONTEXT *vb, int32_t left, int32_t right) {
    int32_t result; /* Operation output  */

    /* Perform the operation */
    result = left + right;

    /* Update CPU state */
    vb->cpu.psw.cy = (uint32_t) result < (uint32_t) left ? 1 : 0;
    vb->cpu.psw.ov = (~left ^ right & left ^ result) >> 31 & 1;
    vb->cpu.psw.s  = result >> 31 & 1;
    vb->cpu.psw.z  = result ? 0 : 1;

    return result;
}

/* Bitwise AND processing */
static int32_t cpiAnd(VUE_CONTEXT *vb, int32_t left, int32_t right) {
    int32_t result; /* Operation output  */

    /* Perform the operation */
    result = left & right;

    /* Update CPU state */
    vb->cpu.psw.ov = 0;
    vb->cpu.psw.s  = result >> 31 & 1;
    vb->cpu.psw.z  = result ? 0 : 1;

    return result;
}

/* Subtraction processing */
static int32_t cpiSubtract(VUE_CONTEXT *vb, int32_t left, int32_t right) {
    int32_t result; /* Operation output  */

    /* Perform the operation */
    result = left - right;

    /* Update CPU state */
    vb->cpu.psw.cy = (uint32_t) left < (uint32_t) right ? 1 : 0;
    vb->cpu.psw.ov = (left ^ right & left ^ result) >> 31 & 1;
    vb->cpu.psw.s  = result >> 31 & 1;
    vb->cpu.psw.z  = result ? 0 : 1;

    return result;
}



/*****************************************************************************
 *                           Instruction Functions                           *
 *****************************************************************************/

/* Forward references */
static int cpuRaiseException(VUE_CONTEXT *, uint16_t, VUE_INSTRUCTION *);

/* Illegal opcode */
static int cpiIllegal(VUE_CONTEXT *vb){

    /* Raise an exception */
    return cpuRaiseException(vb, 0xFF90, &vb->instruction);
}

/* ADD - Add Immediate */
static int cpiADD_IMM(VUE_CONTEXT *vb) {

    /* Call the common addition processor */
    vb->cpu.registers[vb->instruction.register2] = cpiAdd(vb,
        vb->cpu.registers[vb->instruction.register2],
        vb->instruction.immediate
    );

    vb->cpu.cycles += 1; /* Update emulation state */
    return 0;            /* No emulation break was requested */
}

/* ADD - Add Register */
static int cpiADD_REG(VUE_CONTEXT *vb) {

    /* Call the common addition processor */
    vb->cpu.registers[vb->instruction.register2] = cpiAdd(vb,
        vb->cpu.registers[vb->instruction.register2],
        vb->cpu.registers[vb->instruction.register1]
    );

    vb->cpu.cycles += 1; /* Update emulation state */
    return 0;            /* No emulation break was requested */
}

/* ADDI - Add Immediate */
static int cpiADDI(VUE_CONTEXT *vb) {

    /* Call the common addition processor */
    vb->cpu.registers[vb->instruction.register2] = cpiAdd(vb,
        vb->cpu.registers[vb->instruction.register2],
        vb->instruction.immediate
    );

    vb->cpu.cycles += 1; /* Update emulation state */
    return 0;            /* No emulation break was requested */
}

/* AND - And */
static int cpiAND(VUE_CONTEXT *vb) {

    /* Call the common AND processor */
    vb->cpu.registers[vb->instruction.register2] = cpiAnd(vb,
        vb->cpu.registers[vb->instruction.register2],
        vb->cpu.registers[vb->instruction.register1]
    );

    vb->cpu.cycles += 1; /* Update emulation state */
    return 0;            /* No emulation break was requested */
}

/* ANDI - And Immediate */
static int cpiANDI(VUE_CONTEXT *vb) {

    /* Call the common AND processor */
    vb->cpu.registers[vb->instruction.register2] = cpiAnd(vb,
        vb->cpu.registers[vb->instruction.register2],
        vb->instruction.immediate
    );

    vb->cpu.cycles += 1; /* Update emulation state */
    return 0;            /* No emulation break was requested */
}

/* BCOND - Branch on Condition */
static int cpiBCOND(VUE_CONTEXT *vb) {

    /* The branch was taken */
    if (vueCheckCondition(vb->instruction.condition)) {
        vb->cpu.pc = vb->cpu.pc + vb->instruction.displacement - 2;
        vb->cpu.cycles += 3;
    }

    /* The branch was not taken */
    else vb->cpu.cycles += 1;

    /* No emulation break was requested */
    return 0;
}

/* CAXI - Compare and Exchange Interlocked */
static int cpiCAXI(VUE_CONTEXT *vb) {
    VUE_ACCESS access;     /* Bus access descriptor */
    int        break_code; /* Application-supplied emulation break code */

    /* Read the lock value from memory */
    access.format  = VUE_32;
    access.address = vb->instruction.address;
    break_code     = cpuRead(vb, &access);

    /* An application break was requested */
    if (break_code)
        return break_code;

    /* Perform a comparison */
    cpiSubtract(vb,
        vb->cpu.registers[vb->instruction.register2],
        access.value
    );

    /* Exchange lock values */
    if (vb->cpu.psw.z) {

         /* Write the exchange value to memory */
         access.value = vue->cpu.registers[30];
         break_code   = cpuWrite(vb, &access);

         /* An emulation break was requested */
         if (break_code)
             return break_code;
    }

    vb->cpu.cycles += 26; /* Update emulation state */
    return 0;             /* No emulation break was requested */
}

/* CLI - Clear Interrupt Disable Flag */
static int cpiCLI(VUE_CONTEXT *vb) {
    vb->cpu.psw.id   = 0; /* Enable interrupts */
    vb->cpu.cycles += 12; /* Update emulation state */
    return 0;             /* No emulation break was requested */
}

/* CMP - Compare Immediate */
static int cpiCMP_IMM(VUE_CONTEXT *vb) {

    /* Call the common subtraction processor */
    vb->cpu.registers[vb->instruction.register2] = cpiSubtract(vb,
        vb->cpu.registers[vb->instruction.register2],
        vb->instruction.immediate
    );

    vb->cpu.cycles += 1; /* Update emulation state */
    return 0;            /* No emulation break was requested */
}

/* CMP - Compare Register */
static int cpiCMP_REG(VUE_CONTEXT *vb) {

    /* Call the common subtraction processor */
    vb->cpu.registers[vb->instruction.register2] = cpiSubtract(vb,
        vb->cpu.registers[vb->instruction.register2],
        vb->cpu.registers[vb->instruction.register1]
    );

    vb->cpu.cycles += 1; /* Update emulation state */
    return 0;            /* No emulation break was requested */
}

/* DIV - Divide */
static int cpiDIV(VUE_CONTEXT *vb) {
    int     break_code; /* Application-supplied emulation break code */
    int32_t left;       /* Left-hand operand */
    int32_t result;     /* Operation output */
    int32_t right;      /* Right-hand operand */

    /* Resolve the operands */
    left = vb->cpu.registers[vb->instruction.reg2];
    right = vb->cpu.registers[vb->instruction.reg1];

    /* Zero division occurred */
    if (right == 0)
        return cpuRaiseException(vb, 0xFF80, &vb->instruction);

    /* Perform special-case division */
    if (left == -0x800000000 && right == -1) {
        result = left;
        vb->cpu.ov = 1;
        vb->cpu.registers[30] = 0;
    }

    /* Perform regular division */
    else {
        result = left / right;
        vb->cpu.ov = 0;
        vb->cpu.registers[30] = left % right;
    }

    /* Update emulation state */
    vb->cpu.registers[vb->instruction.register2] = result;
    vb->cpu.psw.s = result >> 31 & 1;
    vb->cpu.psw.z = result ? 0 : 1;
    vb->cpu.cycles += 38;

    /* No emulation break was requested */
    return 0;
}



#endif /* VUEAPI */

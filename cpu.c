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
 *                                   Types                                   *
 *****************************************************************************/

/* Pipeline stage handler pointer */
typedef int (*STAGEDEF)(VUE_CONTEXT *, VUE_INSTRUCTION *, uint32_t);



/*****************************************************************************
 *                           Non-Library Functions                           *
 *****************************************************************************/

/* Enter exception processing */
/* RESEARCH: Is ECR.FECC cleared when a regular exception occurs? */
/* RESEARCH: How many cycles does this take? */
static int cpuRaiseException(VUE_CONTEXT *vb, uint16_t code,
    VUE_INSTRUCTION *inst) {
    VUE_ACCESS access;     /* Write access descriptor                   */
    int        break_code; /* Application-supplied emulation break code */
    uint32_t   psw;        /* Current value in PSW                      */

    /* Call the application-supplied exception handler if available */
    if (vb->debug.onexception) {

        /* Call the exception handler */
        break_code = vb->debug.onexception(vb, code, inst);

        /* An emulation break was requested */
        if (break_code)
            return break_code;
    }

    /* Retrieve the current PSW value */
    psw = vueGetSystemRegister(vb, VUE_PSW);

    /* Fatal exception processing */
    if (vb->cpu.psw.np) {
        access.format = VUE_32;

        /* Write exception code to debug memory */
        access.address = 0x00000000;
        access.value   = 0xFFFF0000 | code;
        break_code     = cpuWrite(vb, &access);
        if (break_code)
            return break_code;

        /* Write PSW to debug memory */
        access.address = 0x00000004;
        access.value   = psw;
        break_code     = cpuWrite(vb, &access);
        if (break_code)
            return break_code;

        /* Write PC to debug memory */
        access.address = 0x00000008;
        access.value   = vb->cpu.pc;
        break_code     = cpuWrite(vb, &access);
        if (break_code)
            return break_code;

        /* Halt the CPU until reset */
        vb->cpu.halt = 1;
        return 0;
    }

    /* Duplexed exception processing */
    if (vb->cpu.psw.ep) {
        vb->cpu.fepc   = vb->cpu.pc;
        vb->cpu.fepsw  = psw;
        vb->cpu.ecr    = vb->cpu.ecr & 0x0000FFFF | (uint32_t) code << 16;
        vb->cpu.psw.np = 1;
        vb->cpu.pc     = 0xFFFFFFD0;
    }

    /* Regular exception processing */
    else if (vb->cpu.psw.ep) {
        vb->cpu.eipc   = vb->cpu.pc;
        vb->cpu.eipsw  = psw;
        vb->cpu.ecr    = vb->cpu.ecr & 0xFFFF0000 | code;
        vb->cpu.psw.ep = 1;
        vb->cpu.pc     = 0xFFFF0000 |
            ((code == 0xFF70) ? 0xFF60 : code & 0xFFF0);
    }

    /* Common exception processing */
    vb->cpu.psw.id = 1;
    vb->cpu.psw.ae = 0;

    /* Update emulation state */
    /* vb->cpu.cycles += 0; */

    /* No emulation break was requested */
    return 0;
}

/* Perform instruction cache commands */
/* RESEARCH: Can ICC and ICD/ICR be performed simultaneously? */
/* RESEARCH: How many cycles does this take? */
static uint32_t cpuCacheControl(VUE_CONTEXT *vb, uint32_t value) {
    uint32_t cec; /* Cache Entry Count  */
    uint32_t cen; /* Cache Entry Number */
    int      x;   /* Iterator           */

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

    /* Update emulation state */
    /* vb->cpu.cycles += 0; */

    /* Update and return the system register value */
    return vb->cpu.chcw = value & ICE;
}

/* Decode a fetched instruction */
static void cpuDecode(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint32_t pc) {

    /* Decode the instruction operands */
    FMTDEFS[inst->format](inst);

    /* Additional processing */
    switch (inst->instruction) {
        case VUE_BCOND:
            inst->true = vueCheckCondition(vb, inst->condition);
            break;
        case CPI_BITSTRING:
            inst->instruction = BITSTRINGDEFS[inst->subopcode];
            break;
        case CPI_FLOATENDO:
            inst->instruction = FLOATENDODEFS[inst->subopcode];
            break;
        default:;
    }
}

/* Execute an instruction */
static int cpuExecute(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint32_t pc) {
    int break_code; /* Application-supplied emulation break code */

    /* Decode the instruction */
    cpuDecode(vb, inst, 0);

    /* Execute the appropriate instruction handler */
    break_code = INSTDEFS[inst->instruction](vb);

    /* Reset r0 in case the instruction changed it */
    vb->cpu.registers[0] = 0;

    /* Update emulation state */
    vb->cpu.stage = VUE_INTERRUPT;

    /* Relay the given break code back to the caller */
    return break_code;
}

/* Fetch the first 16 bits of an instruction */
static int cpuFetch16(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint32_t pc) {
    VUE_ACCESS  access;     /* Read access descriptor                    */
    int         break_code; /* Application-supplied emulation break code */
    OPDEF      *opdef;      /* Opcode translation descriptor             */

    /* Read the bits from the memory bus */
    access.address = pc;
    access.format  = VUE_U16;
    break_code     = cpuRead(vb, &access);

    /* An emulation break was requested */
    if (break_code)
        return break_code;

    /* Process the opcode translation from the loaded bits */
    inst->bits        = access.value;
    inst->opcode      = access.value >> 10 & 63;
    opdef             = (OPDEF *) &OPDEFS[inst->opcode];
    inst->format      = opdef->format;
    inst->instruction = opdef->instruction;
    inst->size        = (opdef->format < 4) ? 2 : 4;

    /* The fetch was requested externally */
    if (inst != &vb->instruction)
        return 0;
    
    /* Update emulation state */
    vb->cpu.stage = (opdef->format < 4) ? VUE_EXECUTE : VUE_FETCH32;

    /* No emulation break was requested */
    return 0;
}

/* Fetch the second 16 bits of a 32-bit instruction */
static int cpuFetch32(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint32_t pc) {
    VUE_ACCESS access;     /* Read access descriptor                    */
    int        break_code; /* Application-supplied emulation break code */

    /* Read the bits from the memory bus */
    access.address = pc + 2;
    access.format  = VUE_U16;
    break_code     = cpuRead(vb, &access);

    /* An emulation break was requested */
    if (break_code)
        return break_code;

    /* Update instruction information */
    inst->bits = inst->bits << 16 | access.value;

    /* The fetch was requested externally */
    if (inst != &vb->instruction)
        return 0;

    /* Update emulation state */
    vb->cpu.stage = VUE_EXECUTE;

    /* No emulation break was requested */
    return 0;
}

/* Pipeline stage handler for interrupt checks */
static int cpuInterrupt(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst, uint32_t pc) {
    int break_code; /* Application-supplied emulation break code */
    int level;      /* Interrupt level                           */

    /* Interrupt code lookup table */
    static const uint16_t CODES[] = {
        0xFE00, /* Game pad  */
        0xFE10, /* Timer     */
        0xFE20, /* Cartridge */
        0xFE30, /* Link      */
        0xFE40  /* VIP       */
    };

    /* Ignore interrupts entirely */
    if (
        vb->cpu.psw.np ||
        vb->cpu.psw.ep ||
        vb->cpu.psw.id
    ) return 0;

    /* Determine whether an interrupt occurred */
    level = -1;
    /*      if (vipInterrupt(vb)) level = 4; */
    /* else if (lnkInterrupt(vb)) level = 3; */
    /* else if (tmrInterrupt(vb)) level = 1; */
    /* else if (padInterrupt(vb)) level = 0; */

    /* No exception, or exception was masked */
    if (level == -1 || level > vb->cpu.psw.i)
        return 0;

    /* Raise an exception */
    break_code = cpuRaiseException(vb, CODES[level], NULL);

    /* An emulation break was requested */
    if (break_code)
        return break_code;

    /* CPU configuration */
    vb->cpu.halt  = 0;
    vb->cpu.psw.i = level + 1;

    /* No interrupt occurred */
    return 0;
}



/*****************************************************************************
 *                             Library Functions                             *
 *****************************************************************************/

/* One emulation iteration for CPU operations */
static int cpuEmulate(VUE_CONTEXT *vb) {
    int break_code; /* Application-supplied emulation break code */

    /* Pipeline stage handler lookup table */
    static const STAGEDEF STAGES[] =
        { &cpuInterrupt, &cpuFetch16, &cpuFetch32, &cpuExecute };

    /* Keep processing until CPU time elapses */
    for (vb->cpu.cycles = 0; vb->cpu.cycles == 0 && !vb->cpu.halt;) {

        /* Process the pipeline stage */
        break_code = STAGES[vb->cpu.stage](vb, &vb->instruction, vb->cpu.pc);

        /* An emulation break was requested */
        if (break_code)
            return break_code;
    }

    /* No emulation break was requested */
    return 0;
}

/* Perform a read on the CPU bus                                   */
/* Returns non-zero if the application requests an emulation break */
static int cpuRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Call the application-supplied access handler if available */
    if (vb->debug.onread)
        return vb->debug.onread(vb, access);

    /* Call the bus handler directly */
    vb->cpu.cycles += busRead(vb, access);

    /* No emulation break was requested */
    return 0;
}

/* Perform a write on the CPU bus                                  */
/* Returns non-zero if the application requests an emulation break */
static int cpuWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {
    int32_t cycles; /* Number of cycles taken by access operation */

    /* Call the application-supplied access handler if available */
    if (vb->debug.onwrite)
        return vb->debug.onwrite(vb, access);

    /* Call the bus handler directly */
    vb->cpu.cycles += busWrite(vb, access);

    /* No emulation break was requested */
    return 0;
}



#endif /* VUEAPI */

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

/* Format decoder callback pointer */
typedef void (*FMTPROC)(VUE_INSTRUCTION *);



/*****************************************************************************
 *                             Utility Functions                             *
 *****************************************************************************/

/* Perform instruction cache commands */
/* RESEARCH: Can ICC and ICD/ICR be performed simultaneously? */
/* RESEARCH: How many cycles do these operations take? */
static int cpuCacheControl(VUE_CONTEXT *vb, uint32_t value, int internal) {
    int32_t cec; /* Cache Entry Count */
    int32_t cen; /* Cache Entry Number */
    int     x;   /* Iterator */

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
    if ((value & ICD & ICR) == ICD) {
        /* dump_to_address(value & 0xFFFFFF00) */
    }

    /* Restore instruction cache from memory */
    if ((value & ICD & ICR) == ICR) {
        /* restore_from_address(value & 0xFFFFFF00) */
    }

    /* Suppress unused parameter warning */
    internal = internal;

    /* Update emulation state */
    vb->cpu.chcw = value & ICE;

    return 0;
}

/* Determine whether a particular status condition is met */
static int cpuCheckCondition(VUE_CONTEXT *vb, int id) {

    /* Select operation by condition ID */
    switch (id) {
        case VUE_V:  return vb->cpu.psw.ov;
        case VUE_C:  return vb->cpu.psw.cy;
        case VUE_Z:  return vb->cpu.psw.z;
        case VUE_NH: return vb->cpu.psw.cy | vb->cpu.psw.z;
        case VUE_N:  return vb->cpu.psw.s;
        case VUE_T:  return 1;
        case VUE_LT: return vb->cpu.psw.s ^ vb->cpu.psw.ov;
        case VUE_LE: return (vb->cpu.psw.s ^ vb->cpu.psw.ov) | vb->cpu.psw.z;
        case VUE_NV: return vb->cpu.psw.ov ^ 1;
        case VUE_NC: return vb->cpu.psw.cy ^ 1;
        case VUE_NZ: return vb->cpu.psw.z ^ 1;
        case VUE_H:  return (vb->cpu.psw.cy | vb->cpu.psw.z) ^ 1;
        case VUE_P:  return vb->cpu.psw.s ^ 1;
        case VUE_F:  return 0;
        case VUE_GE: return vb->cpu.psw.s ^ vb->cpu.psw.ov ^ 1;
        case VUE_GT: return ((vb->cpu.psw.s^vb->cpu.psw.ov) | vb->cpu.psw.z)^1;
        default:;
    }

    /* Invalid condition ID */
    return 0;
}

/* Set a value in a system register */
static int cpuLDSR(VUE_CONTEXT *vb, int id, uint32_t value, int internal) {

    /* Select the system register by its ID */
    switch (id) {
        case VUE_EIPC:  vb->cpu.eipc  = value & (uint32_t) -2; break;
        case VUE_EIPSW: vb->cpu.eipsw = value;                 break;
        case VUE_FEPC:  vb->cpu.fepc  = value & (uint32_t) -2; break;
        case VUE_FEPSW: vb->cpu.fepsw = value;                 break;
        case VUE_ADTRE: vb->cpu.adtre = value & (uint32_t) -2; break;
        case VUE_SR29:  vb->cpu.sr29  = value;                 break;
        case VUE_SR31:  vb->cpu.sr31  = value & 1;             break;
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
            break;

        /* Instruction cache dump and restore operations access the bus */
        case VUE_CHCW:
            return cpuCacheControl(vb, value, internal);

        default:;
    }

    return 0;
}

/* Retrieve the value in a system register */
static uint32_t cpuSTSR(VUE_CONTEXT *vb, int id) {

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
                ((uint32_t) vb->cpu.psw.z   <<  0) |
                ((uint32_t) vb->cpu.psw.s   <<  1) |
                ((uint32_t) vb->cpu.psw.ov  <<  2) |
                ((uint32_t) vb->cpu.psw.cy  <<  3) |
                ((uint32_t) vb->cpu.psw.fpr <<  4) |
                ((uint32_t) vb->cpu.psw.fud <<  5) |
                ((uint32_t) vb->cpu.psw.fov <<  6) |
                ((uint32_t) vb->cpu.psw.fzd <<  7) |
                ((uint32_t) vb->cpu.psw.fiv <<  8) |
                ((uint32_t) vb->cpu.psw.fro <<  9) |
                ((uint32_t) vb->cpu.psw.id  << 12) |
                ((uint32_t) vb->cpu.psw.ae  << 13) |
                ((uint32_t) vb->cpu.psw.ep  << 14) |
                ((uint32_t) vb->cpu.psw.np  << 15) |
                (((uint32_t) vb->cpu.psw.i & 15) << 16)
            ;
        default:;
    }

    /* An invalid system register ID was specified */
    return 0;
}



/*****************************************************************************
 *                            Pipeline Functions                             *
 *****************************************************************************/

/* Decode a fetched instruction */
static void cpuDecode(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst) {

    /* Instruction format handler lookup table */
    static const FMTPROC FORMATS[] = {
        &cpfIllegal,  &cpfFormatI, &cpfFormatII, &cpfFormatIII,
        &cpfFormatIV, &cpfFormatV, &cpfFormatVI, &cpfFormatVII,
    };

    /* Decode the binary operands of the instruction */
    FORMATS[inst->format](inst);

    /* Additional processing by format */
    switch (inst->format) {
        case 3: case 4: /* Relative branch */
            inst->address = (vb->cpu.pc + inst->displacement) & (int32_t) -2;
            break;
        case 6: /* Load/store */
            inst->address = inst->displacement +
                (uint32_t) vb->cpu.registers[inst->register1];
            break;
        default:;
    }

    /* Additional processing by instruction */
    switch (inst->instruction) {
        case VUE_BCOND:
            inst->is_true = cpuCheckCondition(vb, inst->condition);
            break;
        case CPI_BITSTRING:
            inst->instruction = BITSTRINGDEFS[inst->subopcode];
            break;
        case CPI_FLOATENDO:
            inst->instruction = FLOATENDODEFS[inst->subopcode];
        default:;
    }
}

/* Execute a fetched instruction */
static int cpuExecute(VUE_CONTEXT *vb) {
    int break_code; /* Emulation break request */

    /* Decode the instruction */
    cpuDecode(vb, &vb->instruction);

    /* Call the application-supplied debug callback if available */
    if (vb->debug.onexecute != NULL) {
        break_code = vb->debug.onexecute(vb, &vb->instruction);
        if (break_code) return break_code;
    }

    /* Execute the instruction */
    break_code = INSTDEFS[vb->instruction.instruction](vb, &vb->instruction);

    /* Reset r0 in case it got changed */
    vb->cpu.registers[0] = 0;

    /* An emulation break was requested */
    if (break_code) return break_code;

    /* Update emulation state */
    vb->cpu.stage = VUE_INTERRUPT;

    return 0;
}

/* Fetch the first 16 bits of an instruction */
static int cpuFetch16(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst,
    uint32_t address) {
    VUE_ACCESS  access;     /* Bus access descriptor */
    int         break_code; /* Emulation break request */
    OPDEF      *def;        /* Opcode-level instruction definition */

    /* Configure the access */
    access.cycles  = 0;
    access.format  = VUE_U16;
    access.address = address;

    /* A library-internal access was requested */
    if (inst == &vb->instruction) {
        break_code = cpuRead(vb, &access);
        if (break_code) return break_code;
    }

    /* An external access was requested */
    else busRead(vb, &access);

    /* Parse the opcode from the loaded bits */
    inst->opcode = (access.value >> 10) & 0x3F;
    def = (OPDEF *) &OPDEFS[inst->opcode];

    /* Configure instruction descriptor */
    inst->bits        = access.value;
    inst->format      = def->format;
    inst->instruction = def->instruction;
    inst->sign_extend = def->sign_extend;
    inst->size        = (inst->format < 4) ? 2 : 4;

    /* Update emulation state */
    if (inst == &vb->instruction) {
        vb->cpu.stage  = (inst->format < 4) ? VUE_EXECUTE : VUE_FETCH32;
        vb->cycles    += access.cycles;
    }

    return 0;
}

/* Fetch the second 16 bits of an instruction */
static int cpuFetch32(VUE_CONTEXT *vb, VUE_INSTRUCTION *inst,
    uint32_t address) {
    VUE_ACCESS access;     /* Bus access descriptor */
    int        break_code; /* Emulation break request */

    /* Configure the access */
    access.cycles  = 0;
    access.format  = VUE_U16;
    access.address = address + 2;

    /* A library-internal access was requested */
    if (inst == &vb->instruction) {
        break_code = cpuRead(vb, &access);
        if (break_code) return break_code;
    }

    /* An external access was requested */
    else busRead(vb, &access);

    /* Configure instruction descriptor */
    inst->bits = (inst->bits << 16) | access.value;

    /* Update emulation state */
    if (inst == &vb->instruction) {
        vb->cpu.stage  = VUE_EXECUTE;
        vb->cycles    += access.cycles;
    }

    return 0;
}

/* Check for and process interrupts */
static int cpuInterrupt(VUE_CONTEXT *vb) {
    int break_code; /* Emulation break request */
    int level;      /* Interrupt level */

    /* Check for the interrupt with the highest priority */
    for (level = 4; level >= 0; level--)
        if (vb->cpu.irq[level]) break;

    /* Determine whether interrupts should be processed */
    if (
        !vb->cpu.psw.np &&
        !vb->cpu.psw.ep &&
        !vb->cpu.psw.id &&
        vb->cpu.psw.i <= level
    ) {

        /* Raise an exception */
        break_code = cpuRaiseException(vb, 0xFE00 | (level << 4));
        if (break_code) return break_code;
    }

    /* Update emulation state */
    if (!vb->cpu.halt)
        vb->cpu.stage = VUE_FETCH;

    return 0;
}



/*****************************************************************************
 *                             Library Functions                             *
 *****************************************************************************/

/* Emulate the CPU for one emulation step */
static int cpuEmulate(VUE_CONTEXT *vb) {
    switch (vb->cpu.stage) {
        case VUE_FETCH:     return cpuFetch16(vb, &vb->instruction,vb->cpu.pc);
        case VUE_FETCH32:   return cpuFetch32(vb, &vb->instruction,vb->cpu.pc);
        case VUE_EXECUTE:   return cpuExecute(vb);
        case VUE_INTERRUPT: return cpuInterrupt(vb);
        default:;
    }
    return 0;
}

/* Raise an exception */
/* RESEARCH: Is ECR.FECC cleared on regular exception? */
/* RESEARCH: How many cycles does this take? */
static int cpuRaiseException(VUE_CONTEXT *vb, uint16_t code) {
    VUE_ACCESS access;     /* Bus access descriptor */
    int        break_code; /* Emulation break request */

    /* Call the application-supplied debug callback if available */
    if (vb->debug.onexception != NULL) {
        break_code = vb->debug.onexception(vb, code);
        if (break_code) return break_code;
    }

    /* A fatal exception occurred */
    if (vb->cpu.psw.np) {

        /* Write the debug information to memory */
        access.cycles = 0;
        access.format = VUE_FATAL;
        access.value  = ((int32_t) 0xFFFF << 16) | code;
        break_code = cpuWrite(vb, &access);
        if (break_code) return break_code;

        /* Update emulation state */
        vb->cpu.halt  = 1;
        vb->cycles   += access.cycles;
        return 0;
    }

    /* A duplexed exception occurred */
    if (vb->cpu.psw.ep) {
        vb->cpu.fepc    = vb->cpu.pc;
        vb->cpu.fepsw   = cpuSTSR(vb, VUE_PSW);
        vb->cpu.ecr    |= (uint32_t) code << 16;
        vb->cpu.psw.np  = 1;
        vb->cpu.pc      = ((uint32_t) 0xFFFF << 16) | 0xFFD0;
    }

    /* A regular exception occurred */
    else {
        vb->cpu.eipc   = vb->cpu.pc;
        vb->cpu.eipsw  = cpuSTSR(vb, VUE_PSW);
        vb->cpu.ecr    = code;
        vb->cpu.psw.ep = 1;
        vb->cpu.pc     = ((uint32_t) 0xFFFF << 16) |
            ((code == 0xFF70) ? 0xFF60 : code & 0xFFF0);
    }

    /* An interrupt occurred */
    if (vueIsInterrupt(code)) {
        vb->cpu.psw.i = ((code >> 4) & 15) + 1;
        vb->cpu.halt  = 0;
    }

    /* Common processing */
    vb->cpu.psw.id = 1;
    vb->cpu.psw.ae = 0;

    return 0;
}

/* Read a value from the CPU bus */
static int cpuRead(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the application-supplied debug callback if available */
    if (vb->debug.onread != NULL)
        return vb->debug.onread(vb, access);

    /* Use the library routine */
    busRead(vb, access);
    return 0;
}

/* Write a value to the CPU bus */
static int cpuWrite(VUE_CONTEXT *vb, VUE_ACCESS *access) {

    /* Use the application-supplied debug callback if available */
    if (vb->debug.onwrite != NULL)
        return vb->debug.onwrite(vb, access);

    /* Use the library routine */
    busWrite(vb, access);
    return 0;
}



#endif /* VUE_API */

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
 *                           Instruction Functions                           *
 *****************************************************************************/

/* Forward references */
static int cpuRaiseException(VUE_CONTEXT *, uint16_t, VUE_INSTRUCTION *);

/* Illegal opcode */
static int cpiIllegal(VUE_CONTEXT *vb){

    /* Raise an exception */
    return cpuRaiseException(vb, 0xFF90, &vb->instruction);
}



#endif /* VUEAPI */

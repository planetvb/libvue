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
 *                                 Constants                                 *
 *****************************************************************************/

/* Non-library opcode IDs */
#define CPI_BITSTRING 77
#define CPI_FLOATENDO 78



/*****************************************************************************
 *                                   Types                                   *
 *****************************************************************************/

/* Instruction execute handler pointer */
typedef int (*INSTPROC)(VUE_CONTEXT *, VUE_INSTRUCTION *);

/* Opcode translation descriptor */
typedef struct {
    uint8_t format;      /* Instruction encoding format */
    uint8_t sign_extend; /* Immediate operands are sign-extended */
    uint8_t instruction; /* libvue instruction ID */
} OPDEF;



/*****************************************************************************
 *                               Lookup Tables                               *
 *****************************************************************************/

/* Opcode-level instruction data */
static const OPDEF OPDEFS[] = {
    { 1, 0, VUE_MOV_REG   },
    { 1, 0, VUE_ADD_REG   },
    { 1, 0, VUE_SUB       },
    { 1, 0, VUE_CMP_REG   },
    { 1, 0, VUE_SHL_REG   },
    { 1, 0, VUE_SHR_REG   },
    { 1, 0, VUE_JMP       },
    { 1, 0, VUE_SAR_REG   },
    { 1, 0, VUE_MUL       },
    { 1, 0, VUE_DIV       },
    { 1, 0, VUE_MULU      },
    { 1, 0, VUE_DIVU      },
    { 1, 0, VUE_OR        },
    { 1, 0, VUE_AND       },
    { 1, 0, VUE_XOR       },
    { 1, 0, VUE_NOT       },
    { 2, 1, VUE_MOV_IMM   },
    { 2, 1, VUE_ADD_IMM   },
    { 2, 0, VUE_SETF      },
    { 2, 1, VUE_CMP_IMM   },
    { 2, 0, VUE_SHL_IMM   },
    { 2, 0, VUE_SHR_IMM   },
    { 2, 0, VUE_CLI       },
    { 2, 0, VUE_SAR_IMM   },
    { 2, 0, VUE_TRAP      },
    { 2, 0, VUE_RETI      },
    { 2, 0, VUE_HALT      },
    { 2, 0, VUE_ILLEGAL   },
    { 2, 0, VUE_LDSR      },
    { 2, 0, VUE_STSR      },
    { 2, 0, VUE_SEI       },
    { 2, 0, CPI_BITSTRING },
    { 3, 0, VUE_BCOND     },
    { 3, 0, VUE_BCOND     },
    { 3, 0, VUE_BCOND     },
    { 3, 0, VUE_BCOND     },
    { 3, 0, VUE_BCOND     },
    { 3, 0, VUE_BCOND     },
    { 3, 0, VUE_BCOND     },
    { 3, 0, VUE_BCOND     },
    { 5, 1, VUE_MOVEA     },
    { 5, 1, VUE_ADDI      },
    { 4, 0, VUE_JR        },
    { 4, 0, VUE_JAL       },
    { 5, 0, VUE_ORI       },
    { 5, 0, VUE_ANDI      },
    { 5, 0, VUE_XORI      },
    { 5, 0, VUE_MOVHI     },
    { 6, 0, VUE_LD_B      },
    { 6, 0, VUE_LD_H      },
    { 0, 0, VUE_ILLEGAL   },
    { 6, 0, VUE_LD_W      },
    { 6, 0, VUE_ST_B      },
    { 6, 0, VUE_ST_H      },
    { 0, 0, VUE_ILLEGAL   },
    { 6, 0, VUE_ST_W      },
    { 6, 0, VUE_IN_B      },
    { 6, 0, VUE_IN_H      },
    { 6, 0, VUE_CAXI      },
    { 6, 0, VUE_IN_W      },
    { 6, 0, VUE_OUT_B     },
    { 6, 0, VUE_OUT_H     },
    { 7, 0, CPI_FLOATENDO },
    { 6, 0, VUE_OUT_W     }
};

/* Bit string instruction subopcode translation */
static const uint8_t BITSTRINGDEFS[] = {
    VUE_SCH0BSU,
    VUE_SCH0BSD,
    VUE_SCH1BSU,
    VUE_SCH1BSD,
    VUE_ILLEGAL,
    VUE_ILLEGAL,
    VUE_ILLEGAL,
    VUE_ILLEGAL,
    VUE_ORBSU,
    VUE_ANDBSU,
    VUE_XORBSU,
    VUE_MOVBSU,
    VUE_ORNBSU,
    VUE_ANDNBSU,
    VUE_XORNBSU,
    VUE_NOTBSU
};

/* Floating-point/Nintendo instruction subupcode translation */
static const uint8_t FLOATENDODEFS[] = {
    VUE_CMPF_S,
    VUE_ILLEGAL,
    VUE_CVT_WS,
    VUE_CVT_SW,
    VUE_ADDF_S,
    VUE_SUBF_S,
    VUE_MULF_S,
    VUE_DIVF_S,
    VUE_XB,
    VUE_XH,
    VUE_REV,
    VUE_TRNC_SW,
    VUE_MPYHW
};

/* Instruction execute handlers */
static const INSTPROC INSTDEFS[] = {
    &cpiIllegal,
    &cpiADD_IMM,
    &cpiADD_REG,
    &cpiIllegal, /* &cpiADDF_S, */
    &cpiADDI,
    &cpiAND,
    &cpiIllegal, /* &cpiANDBSU, */
    &cpiANDI,
    &cpiIllegal, /* &cpiANDNBSU, */
    &cpiBCOND,
    &cpiCAXI,
    &cpiCLI,
    &cpiCMP_IMM,
    &cpiCMP_REG,
    &cpiIllegal, /* &cpiCMPF_S, */
    &cpiIllegal, /* &cpiCVT_SW, */
    &cpiIllegal, /* &cpiCVT_WS, */
    &cpiDIV,
    &cpiIllegal, /* &cpiDIVF_S, */
    &cpiDIVU,
    &cpiHALT,
    &cpiIN_B,
    &cpiIN_H,
    &cpiIN_W,
    &cpiJAL,
    &cpiJMP,
    &cpiJR,
    &cpiLD_B,
    &cpiLD_H,
    &cpiLD_W,
    &cpiLDSR,
    &cpiMOV_IMM,
    &cpiMOV_REG,
    &cpiIllegal, /* &cpiMOVBSU, */
    &cpiMOVEA,
    &cpiMOVHI,
    &cpiMPYHW,
    &cpiMUL,
    &cpiIllegal, /* &cpiMULF_S, */
    &cpiMULU,
    &cpiNOT,
    &cpiIllegal, /* &cpiNOTBSU, */
    &cpiOR,
    &cpiIllegal, /* &cpiORBSU, */
    &cpiORI,
    &cpiIllegal, /* &cpiORNBSU, */
    &cpiOUT_B,
    &cpiOUT_H,
    &cpiOUT_W,
    &cpiRETI,
    &cpiREV,
    &cpiSAR_IMM,
    &cpiSAR_REG,
    &cpiIllegal, /* &cpiSCH0BSD, */
    &cpiIllegal, /* &cpiSCH0BSU, */
    &cpiIllegal, /* &cpiSCH1BSD, */
    &cpiIllegal, /* &cpiSCH1BSU, */
    &cpiSEI,
    &cpiSETF,
    &cpiSHL_IMM,
    &cpiSHL_REG,
    &cpiSHR_IMM,
    &cpiSHR_REG,
    &cpiST_B,
    &cpiST_H,
    &cpiST_W,
    &cpiSTSR,
    &cpiSUB,
    &cpiIllegal, /* &cpiSUBF_S, */
    &cpiTRAP,
    &cpiIllegal, /* &cpiTRNC_SW, */
    &cpiXB,
    &cpiXH,
    &cpiXOR,
    &cpiIllegal, /* &cpiXORBSU, */
    &cpiXORI,
    &cpiIllegal  /* &cpiXORNBSU */
};



#endif /* VUEAPI */

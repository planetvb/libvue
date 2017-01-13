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

/* Access modes */
#define VUE_DEBUG    0
#define VUE_INTERNAL 1

/* Condition IDs */
#define VUE_C   1
#define VUE_E   2
#define VUE_F  13
#define VUE_GE 14
#define VUE_GT 15
#define VUE_H  11
#define VUE_L   1
#define VUE_LE  7
#define VUE_LT  6
#define VUE_N   4
#define VUE_NC  9
#define VUE_NE 10
#define VUE_NH  3
#define VUE_NL  9
#define VUE_NV  8
#define VUE_NZ 10
#define VUE_P  12
#define VUE_T   5
#define VUE_V   0
#define VUE_Z   2

/* Instruction IDs */
#define VUE_ILLEGAL  0
#define VUE_ADD_IMM  1
#define VUE_ADD_REG  2
#define VUE_ADDF_S   3
#define VUE_ADDI     4
#define VUE_AND      5
#define VUE_ANDBSU   6
#define VUE_ANDI     7
#define VUE_ANDNBSU  8
#define VUE_BCOND    9
#define VUE_CAXI    10
#define VUE_CLI     11
#define VUE_CMP_IMM 12
#define VUE_CMP_REG 13
#define VUE_CMPF_S  14
#define VUE_CVT_SW  15
#define VUE_CVT_WS  16
#define VUE_DIV     17
#define VUE_DIVF_S  18
#define VUE_DIVU    19
#define VUE_HALT    20
#define VUE_IN_B    21
#define VUE_IN_H    22
#define VUE_IN_W    23
#define VUE_JAL     24
#define VUE_JMP     25
#define VUE_JR      26
#define VUE_LD_B    27
#define VUE_LD_H    28
#define VUE_LD_W    29
#define VUE_LDSR    30
#define VUE_MOV_IMM 31
#define VUE_MOV_REG 32
#define VUE_MOVBSU  33
#define VUE_MOVEA   34
#define VUE_MOVHI   35
#define VUE_MPYHW   36
#define VUE_MUL     37
#define VUE_MULF_S  38
#define VUE_MULU    39
#define VUE_NOT     40
#define VUE_NOTBSU  41
#define VUE_OR      42
#define VUE_ORBSU   43
#define VUE_ORI     44
#define VUE_ORNBSU  45
#define VUE_OUT_B   46
#define VUE_OUT_H   47
#define VUE_OUT_W   48
#define VUE_RETI    49
#define VUE_REV     50
#define VUE_SAR_IMM 51
#define VUE_SAR_REG 52
#define VUE_SCH0BSD 53
#define VUE_SCH0BSU 54
#define VUE_SCH1BSD 55
#define VUE_SCH1BSU 56
#define VUE_SEI     57
#define VUE_SETF    58
#define VUE_SHL_IMM 59
#define VUE_SHL_REG 60
#define VUE_SHR_IMM 61
#define VUE_SHR_REG 62
#define VUE_ST_B    63
#define VUE_ST_H    64
#define VUE_ST_W    65
#define VUE_STSR    66
#define VUE_SUB     67
#define VUE_SUBF_S  68
#define VUE_TRAP    69
#define VUE_TRNC_SW 70
#define VUE_XB      71
#define VUE_XH      72
#define VUE_XOR     73
#define VUE_XORBSU  74
#define VUE_XORI    75
#define VUE_XORNBSU 76

/* CPU pipeline stages */
#define VUE_EXECUTE   3
#define VUE_FETCH     1
#define VUE_FETCH16   1
#define VUE_FETCH32   2
#define VUE_INTERRUPT 0

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
typedef struct VUE_ACCESS      VUE_ACCESS;
typedef struct VUE_CONTEXT     VUE_CONTEXT;
typedef struct VUE_INSTRUCTION VUE_INSTRUCTION;

/* Debugging callback function pointers */
typedef int (*VUE_ACCESSPROC   )(VUE_CONTEXT *, VUE_ACCESS *);
typedef int (*VUE_EXCEPTIONPROC)(VUE_CONTEXT *, uint16_t, VUE_INSTRUCTION *);
typedef int (*VUE_EXECUTEPROC  )(VUE_CONTEXT *, VUE_INSTRUCTION *);

/* Descriptor for bus accesses */
struct VUE_ACCESS {
    uint32_t address; /* Bus address to access */
    int32_t  value;   /* Value to write, or receives value read */
    int8_t   format;  /* Data size/format */
};

/* Descriptor for CPU instructions */
struct VUE_INSTRUCTION {
    uint32_t address;      /* Effective address */
    uint32_t bits;         /* Instruction binary data */
    int32_t  displacement; /* Displacement distance */
    int32_t  immediate;    /* Immediate data */
    uint8_t  condition;    /* Condition ID */
    uint8_t  format;       /* Binary format */
    uint8_t  instruction;  /* libvue instruction ID */
    uint8_t  opcode;       /* Instruction ID */
    uint8_t  register1;    /* Right-hand operand */
    uint8_t  register2;    /* Left-hand operand, destination */
    uint8_t  sign_extend;  /* Immediate data is sign-extended */
    uint8_t  size;         /* Binary size, in bytes */
    uint8_t  subopcode;    /* Extended instruction ID */
    uint8_t  true;         /* Condition status */
};

/* Top-level emulation state context */
struct VUE_CONTEXT {

    /* Debug settings */
    struct {
        VUE_EXCEPTIONPROC onexception; /* Exception */
        VUE_EXECUTEPROC   onexecute;   /* Execute */
        VUE_ACCESSPROC    onread;      /* Read access */
        VUE_ACCESSPROC    onwrite;     /* Write access */
    } debug;

    /* Cartridge RAM state information */
    struct {
        uint8_t  *ram;      /* Cartridge RAM data */
        uint32_t  ram_size; /* Size, in bytes, of RAM data */
        uint8_t  *rom;      /* Cartridge ROM data */
        uint32_t  rom_size; /* Size, in bytes, of ROM data */
    } cart;

    /* CPU state information */
    struct {

        /* System registers */
        uint32_t adtre; /* Address Trap Register for Execution */
        uint32_t chcw;  /* Cache Control Word */
        uint32_t ecr;   /* Exception Cause Register */
        uint32_t eipc;  /* Exception/Interrupt restore PC */
        uint32_t eipsw; /* Exception/Interrupt restore PSW */
        uint32_t fepc;  /* Duplexed Exception restore PC */
        uint32_t fepsw; /* Duplexed Exception restore PSW */
        uint32_t sr29;  /* "System Register 29" */
        uint32_t sr31;  /* "System Register 31" */

        /* Program Status Word */
        struct {
            uint8_t ae;  /* Address trap enable */
            uint8_t cy;  /* Carry */
            uint8_t ep;  /* Exception pending */
            uint8_t fiv; /* Floating-point invalid operation */
            uint8_t fov; /* Floating-point overflow */
            uint8_t fpr; /* Floating-point precision degradation */
            uint8_t fro; /* Floating-point reserved operand */
            uint8_t fud; /* Floating-point underflow */
            uint8_t fzd; /* Floating-point zero division */
            uint8_t i;   /* Interrupt masking level */
            uint8_t id;  /* Interrupt disable */
            uint8_t np;  /* Duplexed exception ("NMI") pending */
            uint8_t ov;  /* Overflow */
            uint8_t s;   /* Sign */
            uint8_t z;   /* Zero */
        } psw;

        /* Program registers */
        uint32_t pc;            /* Program counter */
        int32_t  registers[32]; /* Program registers */

        /* CPU control */
        int32_t cycles; /* Cycles for current emulation step */
        uint8_t halt;   /* Halting status */
        uint8_t stage;  /* Current pipeline stage */

    } cpu;

    /* WRAM state information */
    uint8_t wram[0x10000];

    /* Library state fields */
    VUE_INSTRUCTION instruction; /* Working data for CPU instructions */

};



/*****************************************************************************
 *                            Function Prototypes                            *
 *****************************************************************************/

VUEAPI int      vueCheckCondition   (VUE_CONTEXT *vb, int id);
VUEAPI int      vueEmulate          (VUE_CONTEXT *vb, int32_t *cycles);
VUEAPI uint32_t vueGetSystemRegister(VUE_CONTEXT *vb, int id);
VUEAPI void     vueRead             (VUE_CONTEXT *vb, VUE_ACCESS *access, int mode);
VUEAPI uint32_t vueSetSystemRegister(VUE_CONTEXT *vb, int id, uint32_t value);
VUEAPI void     vueWrite            (VUE_CONTEXT *vb, VUE_ACCESS *access, int mode);



#ifdef __cplusplus
}
#endif

#endif /* __VUE_H__ */

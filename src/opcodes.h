/**
 * @file opcodes.h
 * @brief Tabela de opcodes da JVM: mnemônico + formato dos operandos.
 *
 * Em vez de um switch gigante, cada opcode tem uma entrada na tabela dizendo
 * o nome e o "tipo" de operando. O disassembler (bytecode.c) usa o tipo
 * para saber quantos bytes ler e como exibir.
 */
#ifndef OPCODES_H
#define OPCODES_H

#include "types.h"

typedef enum {
    OPK_NONE,             /* sem operandos                       (1 byte no total)  ex.: iadd, iconst_0 */
    OPK_LOCAL,            /* u1 índice de variável local         (2)  ex.: iload 4, astore 2, ret */
    OPK_BYTE,             /* s1 imediato                         (2)  bipush */
    OPK_SHORT,            /* s2 imediato                         (3)  sipush */
    OPK_CP_U1,            /* u1 índice do pool                   (2)  ldc */
    OPK_CP_U2,            /* u2 índice do pool                   (3)  ldc_w, getfield, invokevirtual, new... */
    OPK_BRANCH2,          /* s2 offset de desvio                 (3)  ifeq, goto, jsr... */
    OPK_BRANCH4,          /* s4 offset de desvio                 (5)  goto_w, jsr_w */
    OPK_IINC,             /* u1 índice local + s1 constante      (3)  iinc */
    OPK_NEWARRAY,         /* u1 atype                            (2)  newarray */
    OPK_INVOKEINTERFACE,  /* u2 cp + u1 count + u1 zero          (5)  invokeinterface */
    OPK_INVOKEDYNAMIC,    /* u2 cp + u1 zero + u1 zero           (5)  invokedynamic */
    OPK_MULTIANEWARRAY,   /* u2 cp + u1 dimensões                (4)  multianewarray */
    OPK_TABLESWITCH,      /* tamanho variável (padding)               tableswitch */
    OPK_LOOKUPSWITCH,     /* tamanho variável (padding)               lookupswitch */
    OPK_WIDE              /* modifica a instrução seguinte            wide */
} OperandKind;

typedef struct {
    const char *mnemonic;   /* NULL = opcode não mapeado / inválido */
    OperandKind kind;
} OpcodeInfo;

extern const OpcodeInfo OPCODES[256];

#endif

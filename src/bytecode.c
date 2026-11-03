/**
 * @file bytecode.c
 * @brief Disassembler de bytecode orientado pela tabela de opcodes.
 */
#include "bytecode.h"
#include "constant_pool.h"
#include "opcodes.h"
#include "util.h"

#include <stdio.h>

/* Contexto da instrução sendo decodificada. */
typedef struct {
    const ClassFile *cf;
    const u1        *code;
    u4               len;
    u4               pc;     /* endereço do opcode */
    int              depth;
} Insn;

/* Leituras relativas ao pc, com checagem de limites.
 * `has(in, n)` verifica se existem os bytes pc+1 .. pc+n. */
static int has(const Insn *in, u4 n)        { return (u4)(in->len - in->pc) > n; }
static u1  at_u1(const Insn *in, u4 off)    { return in->code[in->pc + off]; }
static u2  at_u2(const Insn *in, u4 off)    { return (u2)((at_u1(in, off) << 8) | at_u1(in, off + 1)); }
static int32_t at_s4(const Insn *in, u4 off)
{
    return (int32_t)(((u4)at_u1(in, off) << 24) | ((u4)at_u1(in, off + 1) << 16) |
                     ((u4)at_u1(in, off + 2) << 8) | (u4)at_u1(in, off + 3));
}

static void print_cp_operand(const ClassFile *cf, u2 idx)
{
    char desc[1024];
    cp_describe(cf, idx, desc, sizeof desc);
    printf(" #%u <%s>", idx, desc);
}

static const char *newarray_type_name(u1 atype)
{
    switch (atype) {
    case 4:  return "boolean";
    case 5:  return "char";
    case 6:  return "float";
    case 7:  return "double";
    case 8:  return "byte";
    case 9:  return "short";
    case 10: return "int";
    case 11: return "long";
    default: return "<tipo inválido>";
    }
}

/* ---------------------------------------------------------------------- */
/* Instruções de tamanho variável                                         */
/*                                                                        */
/* Cada função imprime os operandos (a partir do mnemônico, que já foi    */
/* impresso) e retorna o TAMANHO TOTAL da instrução em bytes, ou 0 se a   */
/* instrução estiver malformada / não implementada (o disassembler então  */
/* interrompe a listagem desse método, pois não sabe onde está a próxima).*/
/* ---------------------------------------------------------------------- */

/*
 * TODO(Pessoa B): tableswitch (JVMS §6.5 tableswitch). Formato:
 *   opcode
 *   0-3 bytes de padding, de forma que `default` comece num endereço
 *       múltiplo de 4 CONTANDO A PARTIR DO INÍCIO DO CODE (não do arquivo!)
 *   s4 default, s4 low, s4 high
 *   (high - low + 1) x s4 offsets
 * Todos os offsets são relativos ao pc do opcode. Exibir como o jclasslib:
 *   tableswitch 1 to 3
 *       1: 28 (+26)
 *       2: 31 (+29)
 *       3: 34 (+32)
 *       default: 37 (+35)
 * Use at_s4(in, off) e has(in, n) para ler com segurança.
 * Teste com examples/tableswitch.class.
 */
static u4 op_tableswitch(const Insn *in)
{
    (void)in;
    printf(" <tableswitch não implementado>");
    return 0;
}

/*
 * TODO(Pessoa B): lookupswitch (JVMS §6.5 lookupswitch). Formato:
 *   opcode, 0-3 bytes de padding (mesma regra do tableswitch),
 *   s4 default, s4 npairs, npairs x (s4 match, s4 offset)
 * Exibir:
 *   lookupswitch 3
 *       10: 36 (+35)
 *       20: 39 (+38)
 *       default: 42 (+41)
 * Teste com examples/lookupswitch.class.
 */
static u4 op_lookupswitch(const Insn *in)
{
    (void)in;
    printf(" <lookupswitch não implementado>");
    return 0;
}

/*
 * TODO(Pessoa B): wide (JVMS §6.5 wide). Formato:
 *   wide <opcode> u2 índice                    (4 bytes)  para iload..aload, istore..astore, ret
 *   wide iinc     u2 índice  s2 constante      (6 bytes)
 * Exibir, por exemplo: "wide iload 300"  ou  "wide iinc 300 by 1000".
 */
static u4 op_wide(const Insn *in)
{
    (void)in;
    printf(" <wide não implementado>");
    return 0;
}

/* ---------------------------------------------------------------------- */
/* Operandos de tamanho fixo                                              */
/* ---------------------------------------------------------------------- */

/* Retorna o tamanho total da instrução, ou 0 em caso de erro. */
static u4 print_operands(const Insn *in, OperandKind kind)
{
    switch (kind) {
    case OPK_NONE:
        return 1;

    case OPK_LOCAL:
        if (!has(in, 1)) return 0;
        printf(" %u", at_u1(in, 1));
        return 2;

    case OPK_BYTE:
        if (!has(in, 1)) return 0;
        printf(" %d", (int8_t)at_u1(in, 1));
        return 2;

    case OPK_SHORT:
        if (!has(in, 2)) return 0;
        printf(" %d", (int16_t)at_u2(in, 1));
        return 3;

    case OPK_CP_U1:
        if (!has(in, 1)) return 0;
        print_cp_operand(in->cf, at_u1(in, 1));
        return 2;

    case OPK_CP_U2:
        if (!has(in, 2)) return 0;
        print_cp_operand(in->cf, at_u2(in, 1));
        return 3;

    case OPK_BRANCH2: {
        if (!has(in, 2)) return 0;
        int32_t off = (int16_t)at_u2(in, 1);
        printf(" %ld (%+ld)", (long)in->pc + off, (long)off);
        return 3;
    }

    case OPK_BRANCH4: {
        if (!has(in, 4)) return 0;
        int32_t off = at_s4(in, 1);
        printf(" %ld (%+ld)", (long)in->pc + off, (long)off);
        return 5;
    }

    case OPK_IINC:
        if (!has(in, 2)) return 0;
        printf(" %u by %d", at_u1(in, 1), (int8_t)at_u1(in, 2));
        return 3;

    case OPK_NEWARRAY:
        if (!has(in, 1)) return 0;
        printf(" %u (%s)", at_u1(in, 1), newarray_type_name(at_u1(in, 1)));
        return 2;

    case OPK_INVOKEINTERFACE:
        if (!has(in, 4)) return 0;
        print_cp_operand(in->cf, at_u2(in, 1));
        printf(" count %u", at_u1(in, 3));
        return 5;

    case OPK_INVOKEDYNAMIC:
        if (!has(in, 4)) return 0;
        print_cp_operand(in->cf, at_u2(in, 1));
        return 5;

    case OPK_MULTIANEWARRAY:
        if (!has(in, 3)) return 0;
        print_cp_operand(in->cf, at_u2(in, 1));
        printf(" dim %u", at_u1(in, 3));
        return 4;

    case OPK_TABLESWITCH:  return op_tableswitch(in);
    case OPK_LOOKUPSWITCH: return op_lookupswitch(in);
    case OPK_WIDE:         return op_wide(in);
    }
    return 0;
}

void print_bytecode(const ClassFile *cf, const u1 *code, u4 code_length, int depth)
{
    Insn in = { cf, code, code_length, 0, depth };

    while (in.pc < code_length) {
        u1 op = code[in.pc];
        const OpcodeInfo *info = &OPCODES[op];

        print_indent(depth);
        printf("%5u  ", (unsigned)in.pc);

        if (!info->mnemonic) {
            printf("<opcode 0x%02X não mapeado> -- listagem interrompida\n", op);
            return;
        }

        printf("%s", info->mnemonic);
        u4 size = print_operands(&in, info->kind);
        printf("\n");

        if (size == 0) {
            print_indent(depth);
            printf("       <instrução malformada ou não implementada> -- listagem interrompida\n");
            return;
        }
        in.pc += size;
    }
}

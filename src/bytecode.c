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
/* instrução estiver malformada ou truncada (o disassembler então         */
/* interrompe a listagem desse método, pois não sabe onde está a próxima).*/
/* ---------------------------------------------------------------------- */

/*
 * Os switches têm 0-3 bytes de padding logo após o opcode, de forma que o
 * primeiro operando (default) comece num endereço múltiplo de 4 CONTANDO A
 * PARTIR DO INÍCIO DO CODE (não do arquivo). Retorna o deslocamento, a partir
 * do opcode, onde começa o default.
 */
static u4 switch_operands_offset(const Insn *in)
{
    return 1 + (3 - in->pc % 4);
}

/* Uma linha "chave: destino (+offset)" de um switch, alinhada abaixo do mnemônico. */
static void print_switch_case(const Insn *in, const char *key, int32_t off)
{
    printf("\n");
    print_indent(in->depth);
    printf("%7s    %s: %ld (%+ld)", "", key, (long)in->pc + off, (long)off);
}

/*
 * tableswitch (JVMS §6.5): padding, s4 default, s4 low, s4 high e
 * (high - low + 1) offsets. Os offsets são relativos ao pc do opcode.
 *   tableswitch 1 to 3
 *       1: 28 (+26)
 *       ...
 *       default: 37 (+35)
 */
static u4 op_tableswitch(const Insn *in)
{
    u4 base = switch_operands_offset(in);
    if (!has(in, base + 11)) return 0;

    int32_t def = at_s4(in, base);
    int32_t low = at_s4(in, base + 4);
    int32_t high = at_s4(in, base + 8);
    if (high < low) {
        printf(" <faixa inválida: %ld to %ld>", (long)low, (long)high);
        return 0;
    }

    /* Confere o tamanho da tabela antes de multiplicar (evita overflow). */
    int64_t count = (int64_t)high - low + 1;
    if (count > (int64_t)(in->len / 4)) return 0;
    u4 table = base + 12;
    if (!has(in, table + 4 * (u4)count - 1)) return 0;

    printf(" %ld to %ld", (long)low, (long)high);
    for (u4 i = 0; i < (u4)count; i++) {
        char key[16];
        snprintf(key, sizeof key, "%ld", (long)((int64_t)low + i));
        print_switch_case(in, key, at_s4(in, table + 4 * i));
    }
    print_switch_case(in, "default", def);
    return table + 4 * (u4)count;
}

/*
 * lookupswitch (JVMS §6.5): padding, s4 default, s4 npairs e
 * npairs pares (s4 match, s4 offset), ordenados por match.
 *   lookupswitch 3
 *       10: 36 (+35)
 *       ...
 *       default: 42 (+41)
 */
static u4 op_lookupswitch(const Insn *in)
{
    u4 base = switch_operands_offset(in);
    if (!has(in, base + 7)) return 0;

    int32_t def = at_s4(in, base);
    int32_t npairs = at_s4(in, base + 4);
    if (npairs < 0 || (u4)npairs > in->len / 8) {
        printf(" <npairs inválido: %ld>", (long)npairs);
        return 0;
    }
    u4 pairs = base + 8;
    if (npairs > 0 && !has(in, pairs + 8 * (u4)npairs - 1)) return 0;

    printf(" %ld", (long)npairs);
    for (u4 i = 0; i < (u4)npairs; i++) {
        char key[16];
        snprintf(key, sizeof key, "%ld", (long)at_s4(in, pairs + 8 * i));
        print_switch_case(in, key, at_s4(in, pairs + 8 * i + 4));
    }
    print_switch_case(in, "default", def);
    return pairs + 8 * (u4)npairs;
}

/*
 * wide (JVMS §6.5): estende o índice de variável local para u2.
 *   wide <iload..aload | istore..astore | ret> u2 índice          (4 bytes)
 *   wide iinc u2 índice s2 constante                              (6 bytes)
 * Exibe "wide iload 300" ou "wide iinc 300 by 1000".
 */
static u4 op_wide(const Insn *in)
{
    if (!has(in, 3)) return 0;
    u1 op = at_u1(in, 1);

    if (op == 0x84) { /* iinc */
        if (!has(in, 5)) return 0;
        printf(" iinc %u by %d", at_u2(in, 2), (int16_t)at_u2(in, 4));
        return 6;
    }

    /* As instruções que aceitam wide são exatamente as de operando OPK_LOCAL. */
    const OpcodeInfo *info = &OPCODES[op];
    if (info->mnemonic && info->kind == OPK_LOCAL) {
        printf(" %s %u", info->mnemonic, at_u2(in, 2));
        return 4;
    }

    printf(" <opcode 0x%02X não pode ser modificado por wide>", op);
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
            /* 0xcb - 0xfd não são definidos pela JVMS. */
            printf("<opcode 0x%02X inválido> -- listagem interrompida\n", op);
            return;
        }

        printf("%s", info->mnemonic);
        u4 size = print_operands(&in, info->kind);
        printf("\n");

        if (size == 0) {
            print_indent(depth);
            printf("       <instrução malformada ou truncada> -- listagem interrompida\n");
            return;
        }
        in.pc += size;
    }
}

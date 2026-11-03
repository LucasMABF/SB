/**
 * @file opcodes.c
 * @brief Tabela de opcodes (JVMS capítulo 6 e §7 "Opcode Mnemonics by Opcode").
 *
 * Usa inicializadores designados do C99: [opcode] = {"mnemonico", TIPO}.
 * Entradas omitidas ficam {NULL, OPK_NONE} e o disassembler as trata como
 * "opcode não mapeado".
 */
#include "opcodes.h"

const OpcodeInfo OPCODES[256] = {
    /* ---------------- Constantes (0x00 - 0x14) ---------------- */
    [0x00] = {"nop",         OPK_NONE},
    [0x01] = {"aconst_null", OPK_NONE},
    [0x02] = {"iconst_m1",   OPK_NONE},
    [0x03] = {"iconst_0",    OPK_NONE},
    [0x04] = {"iconst_1",    OPK_NONE},
    [0x05] = {"iconst_2",    OPK_NONE},
    [0x06] = {"iconst_3",    OPK_NONE},
    [0x07] = {"iconst_4",    OPK_NONE},
    [0x08] = {"iconst_5",    OPK_NONE},
    [0x09] = {"lconst_0",    OPK_NONE},
    [0x0a] = {"lconst_1",    OPK_NONE},
    [0x0b] = {"fconst_0",    OPK_NONE},
    [0x0c] = {"fconst_1",    OPK_NONE},
    [0x0d] = {"fconst_2",    OPK_NONE},
    [0x0e] = {"dconst_0",    OPK_NONE},
    [0x0f] = {"dconst_1",    OPK_NONE},
    [0x10] = {"bipush",      OPK_BYTE},
    [0x11] = {"sipush",      OPK_SHORT},
    [0x12] = {"ldc",         OPK_CP_U1},
    [0x13] = {"ldc_w",       OPK_CP_U2},
    [0x14] = {"ldc2_w",      OPK_CP_U2},

    /* ---------------- Loads (0x15 - 0x35) ---------------- */
    [0x15] = {"iload",       OPK_LOCAL},
    [0x16] = {"lload",       OPK_LOCAL},
    [0x17] = {"fload",       OPK_LOCAL},
    [0x18] = {"dload",       OPK_LOCAL},
    [0x19] = {"aload",       OPK_LOCAL},
    [0x1a] = {"iload_0",     OPK_NONE},
    [0x1b] = {"iload_1",     OPK_NONE},
    [0x1c] = {"iload_2",     OPK_NONE},
    [0x1d] = {"iload_3",     OPK_NONE},
    [0x1e] = {"lload_0",     OPK_NONE},
    [0x1f] = {"lload_1",     OPK_NONE},
    [0x20] = {"lload_2",     OPK_NONE},
    [0x21] = {"lload_3",     OPK_NONE},
    [0x22] = {"fload_0",     OPK_NONE},
    [0x23] = {"fload_1",     OPK_NONE},
    [0x24] = {"fload_2",     OPK_NONE},
    [0x25] = {"fload_3",     OPK_NONE},
    [0x26] = {"dload_0",     OPK_NONE},
    [0x27] = {"dload_1",     OPK_NONE},
    [0x28] = {"dload_2",     OPK_NONE},
    [0x29] = {"dload_3",     OPK_NONE},
    [0x2a] = {"aload_0",     OPK_NONE},
    [0x2b] = {"aload_1",     OPK_NONE},
    [0x2c] = {"aload_2",     OPK_NONE},
    [0x2d] = {"aload_3",     OPK_NONE},
    [0x2e] = {"iaload",      OPK_NONE},
    [0x2f] = {"laload",      OPK_NONE},
    [0x30] = {"faload",      OPK_NONE},
    [0x31] = {"daload",      OPK_NONE},
    [0x32] = {"aaload",      OPK_NONE},
    [0x33] = {"baload",      OPK_NONE},
    [0x34] = {"caload",      OPK_NONE},
    [0x35] = {"saload",      OPK_NONE},

    /*
     * TODO(Pessoa B): completar a tabela. Faltam:
     *   0x36 - 0x56  Stores     (istore ... sastore)        -> istore..astore são OPK_LOCAL
     *   0x57 - 0x5f  Stack      (pop, pop2, dup ..., swap)
     *   0x60 - 0x84  Math       (iadd ... lxor, iinc)       -> iinc (0x84) é OPK_IINC
     *   0x85 - 0x93  Conversões (i2l ... i2s)
     *   0x94 - 0xa6  Comparações (lcmp ... if_acmpne)       -> ifeq..if_acmpne são OPK_BRANCH2
     *   0xa7 - 0xb1  Controle   (goto, jsr, ret, tableswitch, lookupswitch, ireturn ... return)
     *                           -> goto/jsr OPK_BRANCH2, ret OPK_LOCAL,
     *                              tableswitch OPK_TABLESWITCH, lookupswitch OPK_LOOKUPSWITCH
     *   0xc6 - 0xc9  Extended   (ifnull, ifnonnull -> OPK_BRANCH2; goto_w, jsr_w -> OPK_BRANCH4)
     *   0xca breakpoint, 0xfe impdep1, 0xff impdep2 (reservados, OPK_NONE)
     *
     * Fonte: JVMS SE 8, capítulo 7 "Opcode Mnemonics by Opcode"
     *   https://docs.oracle.com/javase/specs/jvms/se8/html/jvms-7.html
     * Confira os operandos de cada instrução no capítulo 6.
     */

    /* ---------------- References (0xb2 - 0xc3) ---------------- */
    [0xb2] = {"getstatic",       OPK_CP_U2},
    [0xb3] = {"putstatic",       OPK_CP_U2},
    [0xb4] = {"getfield",        OPK_CP_U2},
    [0xb5] = {"putfield",        OPK_CP_U2},
    [0xb6] = {"invokevirtual",   OPK_CP_U2},
    [0xb7] = {"invokespecial",   OPK_CP_U2},
    [0xb8] = {"invokestatic",    OPK_CP_U2},
    [0xb9] = {"invokeinterface", OPK_INVOKEINTERFACE},
    [0xba] = {"invokedynamic",   OPK_INVOKEDYNAMIC},
    [0xbb] = {"new",             OPK_CP_U2},
    [0xbc] = {"newarray",        OPK_NEWARRAY},
    [0xbd] = {"anewarray",       OPK_CP_U2},
    [0xbe] = {"arraylength",     OPK_NONE},
    [0xbf] = {"athrow",          OPK_NONE},
    [0xc0] = {"checkcast",       OPK_CP_U2},
    [0xc1] = {"instanceof",      OPK_CP_U2},
    [0xc2] = {"monitorenter",    OPK_NONE},
    [0xc3] = {"monitorexit",     OPK_NONE},

    /* ---------------- Extended (0xc4 - 0xc5) ---------------- */
    [0xc4] = {"wide",            OPK_WIDE},
    [0xc5] = {"multianewarray",  OPK_MULTIANEWARRAY},
};

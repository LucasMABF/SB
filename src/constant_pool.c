/**
 * @file constant_pool.c
 * @brief Acesso, resolução e exibição do pool de constantes.
 */
#include "constant_pool.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#define INVALID "<inválido>"

const char *cp_tag_name(u1 tag)
{
    switch (tag) {
    case CONSTANT_Utf8:               return "CONSTANT_Utf8_info";
    case CONSTANT_Integer:            return "CONSTANT_Integer_info";
    case CONSTANT_Float:              return "CONSTANT_Float_info";
    case CONSTANT_Long:               return "CONSTANT_Long_info";
    case CONSTANT_Double:             return "CONSTANT_Double_info";
    case CONSTANT_Class:              return "CONSTANT_Class_info";
    case CONSTANT_String:             return "CONSTANT_String_info";
    case CONSTANT_Fieldref:           return "CONSTANT_Fieldref_info";
    case CONSTANT_Methodref:          return "CONSTANT_Methodref_info";
    case CONSTANT_InterfaceMethodref: return "CONSTANT_InterfaceMethodref_info";
    case CONSTANT_NameAndType:        return "CONSTANT_NameAndType_info";
    case CONSTANT_MethodHandle:       return "CONSTANT_MethodHandle_info";
    case CONSTANT_MethodType:         return "CONSTANT_MethodType_info";
    case CONSTANT_Dynamic:            return "CONSTANT_Dynamic_info";
    case CONSTANT_InvokeDynamic:      return "CONSTANT_InvokeDynamic_info";
    case CONSTANT_Module:             return "CONSTANT_Module_info";
    case CONSTANT_Package:            return "CONSTANT_Package_info";
    default:                          return "CONSTANT_<desconhecida>";
    }
}

const cp_info *cp_get(const ClassFile *cf, u2 idx, u1 expected_tag)
{
    if (idx == 0 || idx >= cf->constant_pool_count) return NULL;
    const cp_info *e = &cf->constant_pool[idx];
    if (e->tag == 0) return NULL;
    if (expected_tag != 0 && e->tag != expected_tag) return NULL;
    return e;
}

const char *cp_utf8(const ClassFile *cf, u2 idx)
{
    const cp_info *e = cp_get(cf, idx, CONSTANT_Utf8);
    return e ? (const char *)e->info.Utf8.bytes : INVALID;
}

const char *cp_class_name(const ClassFile *cf, u2 idx)
{
    const cp_info *e = cp_get(cf, idx, CONSTANT_Class);
    return e ? cp_utf8(cf, e->info.Class.name_index) : INVALID;
}

/* Os valores numéricos são armazenados em big-endian no arquivo (já convertidos
 * para inteiros nativos no parse). Para float/double reinterpretamos os bits
 * com memcpy, que é a forma portável de fazer isso em C. */

int32_t cp_int_value(const cp_info *e)
{
    return (int32_t)e->info.Integer.bytes;
}

float cp_float_value(const cp_info *e)
{
    float f;
    u4 bits = e->info.Integer.bytes;
    memcpy(&f, &bits, sizeof f);
    return f;
}

static uint64_t cp_u8_value(const cp_info *e)
{
    return ((uint64_t)e->info.Long.high_bytes << 32) | e->info.Long.low_bytes;
}

int64_t cp_long_value(const cp_info *e)
{
    return (int64_t)cp_u8_value(e);
}

double cp_double_value(const cp_info *e)
{
    double d;
    uint64_t bits = cp_u8_value(e);
    memcpy(&d, &bits, sizeof d);
    return d;
}

static const char *method_handle_kind_name(u1 kind)
{
    static const char *names[] = {
        NULL, "REF_getField", "REF_getStatic", "REF_putField", "REF_putStatic",
        "REF_invokeVirtual", "REF_invokeStatic", "REF_invokeSpecial",
        "REF_newInvokeSpecial", "REF_invokeInterface"
    };
    return (kind >= 1 && kind <= 9) ? names[kind] : "REF_<inválido>";
}

/* `depth` evita recursão infinita em arquivos malformados (ex.: MethodHandle
 * apontando para outro MethodHandle). */
static void describe_rec(const ClassFile *cf, u2 idx, char *buf, size_t n, int depth)
{
    const cp_info *e = cp_get(cf, idx, 0);
    if (!e || depth > 3) {
        snprintf(buf, n, INVALID);
        return;
    }

    switch (e->tag) {
    case CONSTANT_Utf8:
        snprintf(buf, n, "%s", (const char *)e->info.Utf8.bytes);
        break;
    case CONSTANT_Integer:
        snprintf(buf, n, "%ld", (long)cp_int_value(e));
        break;
    case CONSTANT_Float:
        snprintf(buf, n, "%g", (double)cp_float_value(e));
        break;
    case CONSTANT_Long:
        snprintf(buf, n, "%lld", (long long)cp_long_value(e));
        break;
    case CONSTANT_Double:
        snprintf(buf, n, "%.15g", cp_double_value(e));
        break;
    case CONSTANT_Class:
    case CONSTANT_Module:
    case CONSTANT_Package:
        snprintf(buf, n, "%s", cp_utf8(cf, e->info.Class.name_index));
        break;
    case CONSTANT_String:
        snprintf(buf, n, "\"%s\"", cp_utf8(cf, e->info.String.string_index));
        break;
    case CONSTANT_NameAndType:
        snprintf(buf, n, "%s : %s", cp_utf8(cf, e->info.NameAndType.name_index),
                 cp_utf8(cf, e->info.NameAndType.descriptor_index));
        break;
    case CONSTANT_Fieldref:
    case CONSTANT_Methodref:
    case CONSTANT_InterfaceMethodref: {
        const cp_info *nat = cp_get(cf, e->info.Ref.name_and_type_index, CONSTANT_NameAndType);
        snprintf(buf, n, "%s.%s : %s", cp_class_name(cf, e->info.Ref.class_index),
                 nat ? cp_utf8(cf, nat->info.NameAndType.name_index) : INVALID,
                 nat ? cp_utf8(cf, nat->info.NameAndType.descriptor_index) : INVALID);
        break;
    }
    case CONSTANT_MethodHandle: {
        char inner[512];
        describe_rec(cf, e->info.MethodHandle.reference_index, inner, sizeof inner, depth + 1);
        snprintf(buf, n, "%s %s", method_handle_kind_name(e->info.MethodHandle.reference_kind), inner);
        break;
    }
    case CONSTANT_MethodType:
        snprintf(buf, n, "%s", cp_utf8(cf, e->info.MethodType.descriptor_index));
        break;
    case CONSTANT_Dynamic:
    case CONSTANT_InvokeDynamic: {
        const cp_info *nat = cp_get(cf, e->info.Dynamic.name_and_type_index, CONSTANT_NameAndType);
        snprintf(buf, n, "#%u %s : %s", e->info.Dynamic.bootstrap_method_attr_index,
                 nat ? cp_utf8(cf, nat->info.NameAndType.name_index) : INVALID,
                 nat ? cp_utf8(cf, nat->info.NameAndType.descriptor_index) : INVALID);
        break;
    }
    default:
        snprintf(buf, n, INVALID);
    }
}

void cp_describe(const ClassFile *cf, u2 idx, char *buf, size_t buf_len)
{
    describe_rec(cf, idx, buf, buf_len, 0);
}

void print_cp_ref(const ClassFile *cf, const char *label, u2 idx, int depth)
{
    char desc[1024];
    cp_describe(cf, idx, desc, sizeof desc);
    print_indent(depth);
    printf("%-*s cp #%u <%s>\n", LABEL_WIDTH, label, idx, desc);
}

/* ---------------------------------------------------------------------- */
/* Exibição de cada entrada (formato inspirado no jclasslib)              */
/* ---------------------------------------------------------------------- */

/* Imprime "Label: valor" sem referência ao pool. */
static void print_kv(int depth, const char *label, const char *fmt_value)
{
    print_indent(depth);
    printf("%-*s %s\n", LABEL_WIDTH, label, fmt_value);
}

/*
 * TODO(Pessoa A): exibição correta de Modified UTF-8 (JVMS §4.4.7).
 *  - O byte nulo é codificado como C0 80; caracteres fora do BMP viram
 *    pares de surrogates (6 bytes: ED Ax xx ED Bx xx).
 *  - Hoje imprimimos os bytes direto, o que funciona para ASCII e para a
 *    maior parte dos acentos (que são iguais em UTF-8 e Modified UTF-8).
 *  - Também seria bom escapar \n, \t etc. para não quebrar o layout
 *    (ex.: uma String "linha\n" hoje gera uma quebra de linha na saída).
 */
static void print_utf8_value(const cp_info *e)
{
    fwrite(e->info.Utf8.bytes, 1, e->info.Utf8.length, stdout);
}

static void print_cp_entry(const ClassFile *cf, u2 i, int depth)
{
    const cp_info *e = &cf->constant_pool[i];
    char buf[1024];

    switch (e->tag) {
    case CONSTANT_Utf8:
        print_indent(depth);
        printf("%-*s %u\n", LABEL_WIDTH, "Length of byte array:", e->info.Utf8.length);
        print_indent(depth);
        printf("%-*s ", LABEL_WIDTH, "String:");
        print_utf8_value(e);
        printf("\n");
        break;

    case CONSTANT_Class:
        print_cp_ref(cf, "Class name:", e->info.Class.name_index, depth);
        break;

    case CONSTANT_Fieldref:
    case CONSTANT_Methodref:
    case CONSTANT_InterfaceMethodref:
        print_cp_ref(cf, "Class name:", e->info.Ref.class_index, depth);
        print_cp_ref(cf, "Name and type:", e->info.Ref.name_and_type_index, depth);
        break;

    /*
     * TODO(Pessoa A): implementar a exibição detalhada dos casos abaixo,
     * seguindo o padrão dos casos acima e o que o jclasslib mostra:
     *
     *  String       -> "String:" cp #string_index <valor>
     *  Integer      -> "Bytes:" 0x%08X  e  "Integer:" valor decimal
     *  Float        -> "Bytes:" 0x%08X  e  "Float:" valor (use cp_float_value)
     *  Long         -> "High bytes:" 0x%08X, "Low bytes:" 0x%08X, "Long:" valor
     *  Double       -> "High bytes:", "Low bytes:", "Double:" valor
     *  NameAndType  -> "Name:" cp #..  e  "Descriptor:" cp #..
     *  MethodHandle -> "Reference kind:" (nome do kind)  e  "Reference index:" cp #..
     *  MethodType   -> "Descriptor:" cp #..
     *  InvokeDynamic/Dynamic -> "Bootstrap method:" índice  e  "Name and type:" cp #..
     *  Module/Package -> "Name:" cp #..
     *
     *  Atenção para float/double especiais: NaN, +Infinity, -Infinity
     *  (o jclasslib exibe esses nomes; printf("%g") imprime "nan"/"inf").
     *
     * Enquanto não estiver implementado, cai no caso genérico abaixo.
     */
    case CONSTANT_String:
    case CONSTANT_Integer:
    case CONSTANT_Float:
    case CONSTANT_Long:
    case CONSTANT_Double:
    case CONSTANT_NameAndType:
    case CONSTANT_MethodHandle:
    case CONSTANT_MethodType:
    case CONSTANT_Dynamic:
    case CONSTANT_InvokeDynamic:
    case CONSTANT_Module:
    case CONSTANT_Package:
    default:
        cp_describe(cf, i, buf, sizeof buf);
        print_kv(depth, "Valor:", buf);
        print_kv(depth, "", "(exibição detalhada não implementada)");
        break;
    }
}

void print_constant_pool(const ClassFile *cf)
{
    print_section("CONSTANT POOL");
    printf("Constant pool count: %u\n\n", cf->constant_pool_count);

    for (u2 i = 1; i < cf->constant_pool_count; i++) {
        const cp_info *e = &cf->constant_pool[i];
        if (e->tag == 0) {
            /* Segunda metade de um Long/Double. */
            printf("[%3u] (large numeric continued)\n", i);
            continue;
        }
        printf("[%3u] %s\n", i, cp_tag_name(e->tag));
        print_cp_entry(cf, i, 3);
    }
}

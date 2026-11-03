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

/* ---------------------------------------------------------------------- */
/* Valores numéricos                                                      */
/*                                                                        */
/* No parse os bytes já foram montados em inteiros (big-endian -> nativo).*/
/* Para float/double reinterpretamos os bits com memcpy, que é a forma    */
/* portável de fazer isso em C (um cast converteria o VALOR, não os bits).*/
/* ---------------------------------------------------------------------- */

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

/*
 * Formata float/double com `digits` algarismos significativos (7 bastam para
 * float, 15 para double). Valores inteiros ganham ".0" ("3.0", não "3"),
 * para não serem confundidos com int. NaN e infinito saem como "nan"/"inf".
 */
static void format_fp(double v, int digits, char *buf, size_t n)
{
    if (v > -1e15 && v < 1e15 && v == (double)(long long)v)
        snprintf(buf, n, "%.1f", v);
    else
        snprintf(buf, n, "%.*g", digits, v);
}

/* ---------------------------------------------------------------------- */
/* Strings (CONSTANT_Utf8)                                                */
/* ---------------------------------------------------------------------- */

/*
 * Copia uma string Utf8 do pool para `buf`, trocando caracteres de controle
 * por escapes (\n, \t, \r) para não quebrar o layout da saída.
 *
 * O formato é "Modified UTF-8" (JVMS §4.4.7). Para caracteres comuns, incluindo
 * acentos, ele é idêntico ao UTF-8 e os bytes podem ser impressos direto. A
 * diferença que tratamos é o caractere nulo, gravado como C0 80 (o byte 00
 * nunca aparece). Caracteres fora do BMP (ex.: emoji) não são convertidos.
 */
static void escape_utf8(const u1 *s, u2 len, char *buf, size_t n)
{
    size_t k = 0;
    for (u2 i = 0; i < len && k + 3 < n; i++) {
        switch (s[i]) {
        case '\n': buf[k++] = '\\'; buf[k++] = 'n'; break;
        case '\t': buf[k++] = '\\'; buf[k++] = 't'; break;
        case '\r': buf[k++] = '\\'; buf[k++] = 'r'; break;
        case 0xC0:
            if (i + 1 < len && s[i + 1] == 0x80) {    /* nulo: C0 80 */
                buf[k++] = '\\'; buf[k++] = '0';
                i++;
                break;
            }
            buf[k++] = (char)s[i];
            break;
        default:
            buf[k++] = (char)s[i];
        }
    }
    if (n > 0) buf[k] = '\0';
}

/* ---------------------------------------------------------------------- */
/* Resolução: o valor que o jclasslib mostra entre < >                    */
/* ---------------------------------------------------------------------- */

/* Field/Method/InterfaceMethodref -> "classe.nome : descritor" */
static void describe_ref(const ClassFile *cf, const cp_info *e, char *buf, size_t n)
{
    const cp_info *nat = cp_get(cf, e->info.Ref.name_and_type_index, CONSTANT_NameAndType);
    snprintf(buf, n, "%s.%s : %s", cp_class_name(cf, e->info.Ref.class_index),
             nat ? cp_utf8(cf, nat->info.NameAndType.name_index) : INVALID,
             nat ? cp_utf8(cf, nat->info.NameAndType.descriptor_index) : INVALID);
}

static int is_ref_tag(u1 tag)
{
    return tag == CONSTANT_Fieldref || tag == CONSTANT_Methodref ||
           tag == CONSTANT_InterfaceMethodref;
}

void cp_describe(const ClassFile *cf, u2 idx, char *buf, size_t n)
{
    const cp_info *e = cp_get(cf, idx, 0);
    if (!e) {
        snprintf(buf, n, INVALID);
        return;
    }

    switch (e->tag) {
    case CONSTANT_Utf8:
        escape_utf8(e->info.Utf8.bytes, e->info.Utf8.length, buf, n);
        break;
    case CONSTANT_Integer:
        snprintf(buf, n, "%ld", (long)cp_int_value(e));
        break;
    case CONSTANT_Float:
        format_fp(cp_float_value(e), 7, buf, n);
        break;
    case CONSTANT_Long:
        snprintf(buf, n, "%lld", (long long)cp_long_value(e));
        break;
    case CONSTANT_Double:
        format_fp(cp_double_value(e), 15, buf, n);
        break;
    case CONSTANT_Class:
    case CONSTANT_Module:
    case CONSTANT_Package:
        snprintf(buf, n, "%s", cp_utf8(cf, e->info.Class.name_index));
        break;
    case CONSTANT_String: {
        const cp_info *s = cp_get(cf, e->info.String.string_index, CONSTANT_Utf8);
        if (!s) {
            snprintf(buf, n, INVALID);
            break;
        }
        char text[1024];
        escape_utf8(s->info.Utf8.bytes, s->info.Utf8.length, text, sizeof text);
        snprintf(buf, n, "\"%s\"", text);
        break;
    }
    case CONSTANT_NameAndType:
        snprintf(buf, n, "%s : %s", cp_utf8(cf, e->info.NameAndType.name_index),
                 cp_utf8(cf, e->info.NameAndType.descriptor_index));
        break;
    case CONSTANT_Fieldref:
    case CONSTANT_Methodref:
    case CONSTANT_InterfaceMethodref:
        describe_ref(cf, e, buf, n);
        break;
    case CONSTANT_MethodHandle: {
        /* Aponta para um Field/Method/InterfaceMethodref. */
        const cp_info *ref = cp_get(cf, e->info.MethodHandle.reference_index, 0);
        if (ref && is_ref_tag(ref->tag))
            describe_ref(cf, ref, buf, n);
        else
            snprintf(buf, n, INVALID);
        break;
    }
    case CONSTANT_MethodType:
        snprintf(buf, n, "%s", cp_utf8(cf, e->info.MethodType.descriptor_index));
        break;
    case CONSTANT_Dynamic:
    case CONSTANT_InvokeDynamic: {
        const cp_info *nat = cp_get(cf, e->info.Dynamic.name_and_type_index, CONSTANT_NameAndType);
        snprintf(buf, n, "%s : %s",
                 nat ? cp_utf8(cf, nat->info.NameAndType.name_index) : INVALID,
                 nat ? cp_utf8(cf, nat->info.NameAndType.descriptor_index) : INVALID);
        break;
    }
    default:
        snprintf(buf, n, INVALID);
    }
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

/* Imprime "Label: valor" (valor já formatado). */
static void print_kv(int depth, const char *label, const char *value)
{
    print_indent(depth);
    printf("%-*s %s\n", LABEL_WIDTH, label, value);
}

static void print_hex_u4(int depth, const char *label, u4 value)
{
    print_indent(depth);
    printf("%-*s 0x%08X\n", LABEL_WIDTH, label, (unsigned)value);
}

static void print_cp_entry(const ClassFile *cf, u2 i, int depth)
{
    const cp_info *e = &cf->constant_pool[i];
    char value[1024];

    switch (e->tag) {
    case CONSTANT_Utf8:
        print_indent(depth);
        printf("%-*s %u\n", LABEL_WIDTH, "Length of byte array:", e->info.Utf8.length);
        cp_describe(cf, i, value, sizeof value);
        print_kv(depth, "String:", value);
        break;

    case CONSTANT_Class:
        print_cp_ref(cf, "Class name:", e->info.Class.name_index, depth);
        break;

    case CONSTANT_String:
        print_cp_ref(cf, "String:", e->info.String.string_index, depth);
        break;

    case CONSTANT_Fieldref:
    case CONSTANT_Methodref:
    case CONSTANT_InterfaceMethodref:
        print_cp_ref(cf, "Class name:", e->info.Ref.class_index, depth);
        print_cp_ref(cf, "Name and type:", e->info.Ref.name_and_type_index, depth);
        break;

    case CONSTANT_NameAndType:
        print_cp_ref(cf, "Name:", e->info.NameAndType.name_index, depth);
        print_cp_ref(cf, "Descriptor:", e->info.NameAndType.descriptor_index, depth);
        break;

    /* Integer/Float: os 4 bytes brutos e o valor. */
    case CONSTANT_Integer:
    case CONSTANT_Float:
        print_hex_u4(depth, "Bytes:", e->info.Integer.bytes);
        cp_describe(cf, i, value, sizeof value);
        print_kv(depth, e->tag == CONSTANT_Integer ? "Integer:" : "Float:", value);
        break;

    /* Long/Double: high e low bytes e o valor (ocupam dois índices do pool). */
    case CONSTANT_Long:
    case CONSTANT_Double:
        print_hex_u4(depth, "High bytes:", e->info.Long.high_bytes);
        print_hex_u4(depth, "Low bytes:", e->info.Long.low_bytes);
        cp_describe(cf, i, value, sizeof value);
        print_kv(depth, e->tag == CONSTANT_Long ? "Long:" : "Double:", value);
        break;

    /* Tags do Java 7+ (lambdas etc.): só os campos, sem detalhar. */
    case CONSTANT_MethodHandle:
        print_indent(depth);
        printf("%-*s %u\n", LABEL_WIDTH, "Reference kind:", e->info.MethodHandle.reference_kind);
        print_cp_ref(cf, "Reference index:", e->info.MethodHandle.reference_index, depth);
        break;

    case CONSTANT_MethodType:
        print_cp_ref(cf, "Descriptor:", e->info.MethodType.descriptor_index, depth);
        break;

    case CONSTANT_Dynamic:
    case CONSTANT_InvokeDynamic:
        print_indent(depth);
        printf("%-*s %u\n", LABEL_WIDTH, "Bootstrap method:", e->info.Dynamic.bootstrap_method_attr_index);
        print_cp_ref(cf, "Name and type:", e->info.Dynamic.name_and_type_index, depth);
        break;

    case CONSTANT_Module:
    case CONSTANT_Package:
        print_cp_ref(cf, "Name:", e->info.Class.name_index, depth);
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

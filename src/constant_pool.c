/**
 * @file constant_pool.c
 * @brief Acesso, resolução e exibição do pool de constantes.
 */
#include "constant_pool.h"
#include "reader.h"
#include "util.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

/*
 * Escreve um long em decimal sem usar "%lld": o msvcrt usado pelo MinGW
 * antigo não reconhece esse formato.
 */
static void format_long(int64_t v, char *buf, size_t buf_len)
{
    char tmp[24];
    size_t k = sizeof tmp - 1;
    uint64_t u = v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v;
    tmp[k] = '\0';
    do {
        tmp[--k] = (char)('0' + u % 10);
        u /= 10;
    } while (u != 0);
    if (v < 0) tmp[--k] = '-';
    snprintf(buf, buf_len, "%s", tmp + k);
}

/* ---------------------------------------------------------------------- */
/* float/double no formato do Java                                        */
/* ---------------------------------------------------------------------- */

/*
 * Mesma regra do Float/Double.toString: notação decimal para 1e-3 <= |v| < 1e7
 * e científica ("1.0E10") fora dessa faixa, sempre com ao menos um dígito
 * depois do ponto. Os dígitos vêm do menor "%.*e" que, lido de volta, dá
 * exatamente o mesmo valor (até 9 dígitos para float e 17 para double).
 */
static void format_java_fp(double v, int is_float, char *buf, size_t buf_len)
{
    if (isnan(v)) {
        snprintf(buf, buf_len, "NaN");
        return;
    }
    if (isinf(v)) {
        snprintf(buf, buf_len, v > 0 ? "Infinity" : "-Infinity");
        return;
    }
    if (v == 0) {
        snprintf(buf, buf_len, signbit(v) ? "-0.0" : "0.0");
        return;
    }

    char sci[48];
    int max_prec = is_float ? 9 : 17;
    for (int p = 1; p <= max_prec; p++) {
        snprintf(sci, sizeof sci, "%.*e", p - 1, v);
        if (is_float ? strtof(sci, NULL) == (float)v : strtod(sci, NULL) == v)
            break;
    }

    /* sci = "[-]d.ddde[+-]XX": separa sinal, dígitos e expoente. */
    const char *p = sci;
    int neg = (*p == '-');
    if (neg) p++;
    char digits[24];
    int nd = 0;
    for (; *p && *p != 'e'; p++)
        if (isdigit((unsigned char)*p) && nd < (int)sizeof digits - 1)
            digits[nd++] = *p;
    int exp = (*p == 'e') ? atoi(p + 1) : 0;
    while (nd > 1 && digits[nd - 1] == '0') nd--;

    char out[64];
    size_t k = 0;
    if (neg) out[k++] = '-';
    double a = v < 0 ? -v : v;
    if (a >= 1e-3 && a < 1e7) {
        if (exp >= 0) {
            for (int i = 0; i <= exp; i++) out[k++] = i < nd ? digits[i] : '0';
            out[k++] = '.';
            if (exp + 1 < nd)
                for (int i = exp + 1; i < nd; i++) out[k++] = digits[i];
            else
                out[k++] = '0';
        } else {
            out[k++] = '0';
            out[k++] = '.';
            for (int i = 0; i < -exp - 1; i++) out[k++] = '0';
            for (int i = 0; i < nd; i++) out[k++] = digits[i];
        }
        out[k] = '\0';
    } else {
        out[k++] = digits[0];
        out[k++] = '.';
        if (nd > 1)
            for (int i = 1; i < nd; i++) out[k++] = digits[i];
        else
            out[k++] = '0';
        snprintf(out + k, sizeof out - k, "E%d", exp);
    }
    snprintf(buf, buf_len, "%s", out);
}

void format_float(float f, char *buf, size_t buf_len)
{
    format_java_fp((double)f, 1, buf, buf_len);
}

void format_double(double d, char *buf, size_t buf_len)
{
    format_java_fp(d, 0, buf, buf_len);
}

/* ---------------------------------------------------------------------- */
/* Modified UTF-8 (JVMS §4.4.7)                                           */
/* ---------------------------------------------------------------------- */

typedef struct {
    char  *buf;
    size_t cap;   /* tamanho total de buf */
    size_t len;   /* bytes já escritos, sem o '\0' */
    int    full;  /* depois que algo não cabe, nada mais é escrito */
} StrOut;

/* Acrescenta s inteira ou nada (nunca corta um caractere UTF-8 no meio). */
static void out_str(StrOut *o, const char *s)
{
    size_t n = strlen(s);
    if (o->full || o->len + n >= o->cap) {
        o->full = 1;
        return;
    }
    memcpy(o->buf + o->len, s, n + 1);
    o->len += n;
}

/* Escreve um code point: caracteres de controle viram escapes, o resto vira UTF-8. */
static void out_code_point(StrOut *o, uint32_t cp)
{
    char tmp[8];
    switch (cp) {
    case 0x00: out_str(o, "\\0"); return;
    case '\b': out_str(o, "\\b"); return;
    case '\t': out_str(o, "\\t"); return;
    case '\n': out_str(o, "\\n"); return;
    case '\f': out_str(o, "\\f"); return;
    case '\r': out_str(o, "\\r"); return;
    }

    if (cp < 0x20 || cp == 0x7F || (cp >= 0xD800 && cp <= 0xDFFF)) {
        /* Demais controles e surrogates sem par (não existem em UTF-8 válido). */
        snprintf(tmp, sizeof tmp, "\\u%04X", (unsigned)cp);
    } else if (cp < 0x80) {
        tmp[0] = (char)cp;
        tmp[1] = '\0';
    } else if (cp < 0x800) {
        tmp[0] = (char)(0xC0 | (cp >> 6));
        tmp[1] = (char)(0x80 | (cp & 0x3F));
        tmp[2] = '\0';
    } else if (cp < 0x10000) {
        tmp[0] = (char)(0xE0 | (cp >> 12));
        tmp[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[2] = (char)(0x80 | (cp & 0x3F));
        tmp[3] = '\0';
    } else {
        tmp[0] = (char)(0xF0 | (cp >> 18));
        tmp[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        tmp[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[3] = (char)(0x80 | (cp & 0x3F));
        tmp[4] = '\0';
    }
    out_str(o, tmp);
}

/*
 * Decodifica um caractere Modified UTF-8 a partir de s[i].
 * Em Modified UTF-8 só existem sequências de 1, 2 e 3 bytes; o caractere
 * nulo é C0 80 (nunca 00) e caracteres fora do BMP são dois surrogates de
 * 3 bytes cada. Retorna quantos bytes foram consumidos, ou 0 se inválido.
 */
static size_t mutf8_next(const u1 *s, size_t len, size_t i, uint32_t *cp)
{
    if (i >= len) return 0;
    u1 b = s[i];
    if (b != 0 && b < 0x80) {
        *cp = b;
        return 1;
    }
    if ((b & 0xE0) == 0xC0 && i + 1 < len && (s[i + 1] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(b & 0x1F) << 6) | (s[i + 1] & 0x3F);
        return 2;
    }
    if ((b & 0xF0) == 0xE0 && i + 2 < len &&
        (s[i + 1] & 0xC0) == 0x80 && (s[i + 2] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(b & 0x0F) << 12) | ((uint32_t)(s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
        return 3;
    }
    return 0;
}

size_t mutf8_format(const u1 *s, size_t len, char *buf, size_t buf_len)
{
    StrOut o = { buf, buf_len, 0, buf_len == 0 };
    if (buf_len > 0) buf[0] = '\0';

    size_t units = 0;
    size_t i = 0;
    while (i < len) {
        uint32_t cp;
        size_t n = mutf8_next(s, len, i, &cp);
        if (n == 0) {
            char tmp[8];
            snprintf(tmp, sizeof tmp, "\\x%02X", s[i]);
            out_str(&o, tmp);
            units++;
            i++;
            continue;
        }

        if (cp >= 0xD800 && cp <= 0xDBFF) {
            uint32_t lo;
            size_t m = mutf8_next(s, len, i + n, &lo);
            if (m > 0 && lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                n += m;
                units++; /* o par ocupa duas unidades UTF-16 */
            }
        }

        out_code_point(&o, cp);
        units++;
        i += n;
    }
    return units;
}

/* ---------------------------------------------------------------------- */
/* Resolução (o valor que o jclasslib mostra entre < >)                   */
/* ---------------------------------------------------------------------- */

static const char *method_handle_kind_name(u1 kind)
{
    static const char *names[] = {
        NULL, "REF_getField", "REF_getStatic", "REF_putField", "REF_putStatic",
        "REF_invokeVirtual", "REF_invokeStatic", "REF_invokeSpecial",
        "REF_newInvokeSpecial", "REF_invokeInterface"
    };
    return (kind >= 1 && kind <= 9) ? names[kind] : "REF_<inválido>";
}

/*
 * Procura o atributo BootstrapMethods da classe e retorna o índice (no pool)
 * do MethodHandle do bootstrap method número `bsm`, ou 0 se não existir.
 */
static u2 bootstrap_method_ref(const ClassFile *cf, u2 bsm)
{
    for (u2 i = 0; i < cf->attributes_count; i++) {
        const attribute_info *a = &cf->attributes[i];
        if (strcmp(cp_utf8(cf, a->attribute_name_index), "BootstrapMethods") != 0)
            continue;

        ByteReader r;
        br_init(&r, a->info, a->attribute_length);
        u2 count = br_u2(&r);
        for (u2 k = 0; k < count && br_ok(&r); k++) {
            u2 ref = br_u2(&r);
            u2 nargs = br_u2(&r);
            if (k == bsm) return br_ok(&r) ? ref : 0;
            br_bytes(&r, (size_t)nargs * 2);
        }
        return 0;
    }
    return 0;
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

    char num[64];
    switch (e->tag) {
    case CONSTANT_Utf8:
        mutf8_format(e->info.Utf8.bytes, e->info.Utf8.length, buf, n);
        break;
    case CONSTANT_Integer:
        snprintf(buf, n, "%ld", (long)cp_int_value(e));
        break;
    case CONSTANT_Float:
        format_float(cp_float_value(e), num, sizeof num);
        snprintf(buf, n, "%s", num);
        break;
    case CONSTANT_Long:
        format_long(cp_long_value(e), buf, n);
        break;
    case CONSTANT_Double:
        format_double(cp_double_value(e), num, sizeof num);
        snprintf(buf, n, "%s", num);
        break;
    case CONSTANT_Class:
    case CONSTANT_Module:
    case CONSTANT_Package:
        snprintf(buf, n, "%s", cp_utf8(cf, e->info.Class.name_index));
        break;
    case CONSTANT_String: {
        const cp_info *s = cp_get(cf, e->info.String.string_index, CONSTANT_Utf8);
        if (!s || n < 3) {
            snprintf(buf, n, INVALID);
            break;
        }
        /* "texto" -- reserva espaço para as duas aspas */
        buf[0] = '"';
        mutf8_format(s->info.Utf8.bytes, s->info.Utf8.length, buf + 1, n - 2);
        strcat(buf, "\"");
        break;
    }
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

/* String de um CONSTANT_Utf8: tamanho em bytes, em caracteres e o texto. */
static void print_utf8_entry(const cp_info *e, int depth)
{
    size_t cap = (size_t)e->info.Utf8.length * 6 + 1;
    char *text = malloc(cap);
    if (!text) {
        print_kv(depth, "String:", "<memória insuficiente>");
        return;
    }
    size_t chars = mutf8_format(e->info.Utf8.bytes, e->info.Utf8.length, text, cap);

    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Length of byte array:", e->info.Utf8.length);
    print_indent(depth);
    printf("%-*s %lu\n", LABEL_WIDTH, "Length of string:", (unsigned long)chars);
    print_kv(depth, "String:", text);
    free(text);
}

static void print_cp_entry(const ClassFile *cf, u2 i, int depth)
{
    const cp_info *e = &cf->constant_pool[i];
    char buf[1024];

    switch (e->tag) {
    case CONSTANT_Utf8:
        print_utf8_entry(e, depth);
        break;

    case CONSTANT_Class:
        print_cp_ref(cf, "Class name:", e->info.Class.name_index, depth);
        break;

    case CONSTANT_Module:
    case CONSTANT_Package:
        print_cp_ref(cf, "Name:", e->info.Class.name_index, depth);
        break;

    case CONSTANT_Fieldref:
    case CONSTANT_Methodref:
    case CONSTANT_InterfaceMethodref:
        print_cp_ref(cf, "Class name:", e->info.Ref.class_index, depth);
        print_cp_ref(cf, "Name and type:", e->info.Ref.name_and_type_index, depth);
        break;

    case CONSTANT_String:
        print_cp_ref(cf, "String:", e->info.String.string_index, depth);
        break;

    case CONSTANT_Integer:
        print_hex_u4(depth, "Bytes:", e->info.Integer.bytes);
        snprintf(buf, sizeof buf, "%ld", (long)cp_int_value(e));
        print_kv(depth, "Integer:", buf);
        break;

    case CONSTANT_Float:
        print_hex_u4(depth, "Bytes:", e->info.Integer.bytes);
        format_float(cp_float_value(e), buf, sizeof buf);
        print_kv(depth, "Float:", buf);
        break;

    case CONSTANT_Long:
        print_hex_u4(depth, "High bytes:", e->info.Long.high_bytes);
        print_hex_u4(depth, "Low bytes:", e->info.Long.low_bytes);
        format_long(cp_long_value(e), buf, sizeof buf);
        print_kv(depth, "Long:", buf);
        break;

    case CONSTANT_Double:
        print_hex_u4(depth, "High bytes:", e->info.Long.high_bytes);
        print_hex_u4(depth, "Low bytes:", e->info.Long.low_bytes);
        format_double(cp_double_value(e), buf, sizeof buf);
        print_kv(depth, "Double:", buf);
        break;

    case CONSTANT_NameAndType:
        print_cp_ref(cf, "Name:", e->info.NameAndType.name_index, depth);
        print_cp_ref(cf, "Descriptor:", e->info.NameAndType.descriptor_index, depth);
        break;

    case CONSTANT_MethodHandle:
        print_indent(depth);
        printf("%-*s %u <%s>\n", LABEL_WIDTH, "Reference kind:", e->info.MethodHandle.reference_kind,
               method_handle_kind_name(e->info.MethodHandle.reference_kind));
        print_cp_ref(cf, "Reference index:", e->info.MethodHandle.reference_index, depth);
        break;

    case CONSTANT_MethodType:
        print_cp_ref(cf, "Descriptor:", e->info.MethodType.descriptor_index, depth);
        break;

    case CONSTANT_Dynamic:
    case CONSTANT_InvokeDynamic: {
        u2 bsm = e->info.Dynamic.bootstrap_method_attr_index;
        u2 ref = bootstrap_method_ref(cf, bsm);
        print_indent(depth);
        if (ref) {
            cp_describe(cf, ref, buf, sizeof buf);
            printf("%-*s %u <%s>\n", LABEL_WIDTH, "Bootstrap method:", bsm, buf);
        } else {
            printf("%-*s %u <não encontrado em BootstrapMethods>\n", LABEL_WIDTH, "Bootstrap method:", bsm);
        }
        print_cp_ref(cf, "Name and type:", e->info.Dynamic.name_and_type_index, depth);
        break;
    }

    default:
        print_kv(depth, "Valor:", "<tag desconhecida>");
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

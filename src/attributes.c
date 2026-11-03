/**
 * @file attributes.c
 * @brief Decodificação e exibição de atributos (JVMS §4.7).
 *
 * Os atributos ficam guardados como bytes brutos no ClassFile. Aqui cada um é
 * decodificado pelo nome: se houver um decodificador na tabela ATTR_DECODERS,
 * ele lê os bytes com um ByteReader e imprime os campos; senão, o conteúdo é
 * mostrado em hexadecimal. Assim um atributo desconhecido nunca quebra a saída.
 */
#include "attributes.h"
#include "bytecode.h"
#include "constant_pool.h"
#include "reader.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

/*
 * Atributos podem se aninhar (Code contém atributos, que podem ser outro Code).
 * Um arquivo malformado poderia aninhar milhares de níveis e estourar a pilha;
 * a partir desta profundidade o conteúdo não é mais decodificado.
 */
#define MAX_ATTR_DEPTH 40

/* Cada decodificador recebe um leitor posicionado no início do campo `info`. */
typedef void (*AttrDecoder)(const ClassFile *cf, ByteReader *r, int depth);

static void print_attribute(const ClassFile *cf, u2 index, u2 name_index,
                            const u1 *info, u4 length, int depth);

/* ---------------------------------------------------------------------- */
/* Atributos com um único índice para o pool                              */
/* ---------------------------------------------------------------------- */

/* SourceFile (§4.7.10): u2 sourcefile_index */
static void decode_source_file(const ClassFile *cf, ByteReader *r, int depth)
{
    print_cp_ref(cf, "Source file name:", br_u2(r), depth);
}

/* ConstantValue (§4.7.2): u2 constantvalue_index */
static void decode_constant_value(const ClassFile *cf, ByteReader *r, int depth)
{
    print_cp_ref(cf, "Constant value index:", br_u2(r), depth);
}

/* Signature (§4.7.9): u2 signature_index */
static void decode_signature(const ClassFile *cf, ByteReader *r, int depth)
{
    print_cp_ref(cf, "Signature index:", br_u2(r), depth);
}

/* ---------------------------------------------------------------------- */
/* Exceptions (§4.7.5): u2 number_of_exceptions; u2 exception_index[n]    */
/* ---------------------------------------------------------------------- */

static void decode_exceptions(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 n = br_u2(r);
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Number of exceptions:", n);
    for (u2 i = 0; i < n && br_ok(r); i++)
        print_cp_ref(cf, "Exception:", br_u2(r), depth + 1);
}

/* ---------------------------------------------------------------------- */
/* Tabelas de depuração (ficam dentro do Code)                            */
/* ---------------------------------------------------------------------- */

/* LineNumberTable (§4.7.12): u2 length; length x { u2 start_pc; u2 line_number } */
static void decode_line_number_table(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf;
    u2 n = br_u2(r);
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Line number table length:", n);
    if (n > 0) {
        print_indent(depth + 1);
        printf("%-4s %-9s %s\n", "Nr.", "Start PC", "Line number");
    }
    for (u2 i = 0; i < n; i++) {
        u2 start_pc = br_u2(r);
        u2 line = br_u2(r);
        if (!br_ok(r)) return;
        print_indent(depth + 1);
        printf("%-4u %-9u %u\n", i, start_pc, line);
    }
}

/*
 * LocalVariableTable (§4.7.13): u2 length; length x {
 *   u2 start_pc; u2 length; u2 name_index; u2 descriptor_index; u2 index }
 */
static void decode_local_variable_table(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 n = br_u2(r);
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Local variable table length:", n);
    if (n > 0) {
        print_indent(depth + 1);
        printf("%-4s %-9s %-7s %-6s %-26s %s\n", "Nr.", "Start PC", "Length", "Index", "Name", "Descriptor");
    }
    for (u2 i = 0; i < n; i++) {
        u2 start_pc = br_u2(r);
        u2 length = br_u2(r);
        u2 name = br_u2(r);
        u2 descriptor = br_u2(r);
        u2 index = br_u2(r);
        if (!br_ok(r)) return;

        /* Nome e descritor com o índice do pool, ex.: "cp #12 <args>". */
        char name_col[64];
        snprintf(name_col, sizeof name_col, "cp #%u <%s>", name, cp_utf8(cf, name));
        print_indent(depth + 1);
        printf("%-4u %-9u %-7u %-6u %-26s cp #%u <%s>\n", i, start_pc, length, index,
               name_col, descriptor, cp_utf8(cf, descriptor));
    }
}

/* ---------------------------------------------------------------------- */
/* Code (§4.7.3)                                                          */
/* ---------------------------------------------------------------------- */

/*
 * u2 max_stack; u2 max_locals; u4 code_length; u1 code[code_length];
 * u2 exception_table_length; { u2 start_pc, end_pc, handler_pc, catch_type }[];
 * u2 attributes_count; attribute_info attributes[];
 */
static void decode_code(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 max_stack = br_u2(r);
    u2 max_locals = br_u2(r);
    u4 code_length = br_u4(r);
    const u1 *code = br_bytes(r, code_length);
    if (!br_ok(r)) return;

    print_indent(depth); printf("%-*s %u\n", LABEL_WIDTH, "Max stack:", max_stack);
    print_indent(depth); printf("%-*s %u\n", LABEL_WIDTH, "Max locals:", max_locals);
    print_indent(depth); printf("%-*s %u\n", LABEL_WIDTH, "Code length:", (unsigned)code_length);
    print_indent(depth); printf("Bytecode:\n");
    print_bytecode(cf, code, code_length, depth + 1);

    u2 exc_len = br_u2(r);
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Exception table length:", exc_len);
    if (exc_len > 0) {
        print_indent(depth + 1);
        printf("%-4s %-8s %-8s %-8s %s\n", "Nr.", "Start PC", "End PC", "Handler", "Catch type");
    }
    for (u2 i = 0; i < exc_len; i++) {
        u2 start = br_u2(r), end = br_u2(r), handler = br_u2(r), catch_type = br_u2(r);
        if (!br_ok(r)) return;
        print_indent(depth + 1);
        printf("%-4u %-8u %-8u %-8u ", i, start, end, handler);
        if (catch_type == 0)
            printf("cp #0 <any>\n");   /* 0 = captura qualquer exceção (finally) */
        else
            printf("cp #%u <%s>\n", catch_type, cp_class_name(cf, catch_type));
    }

    /* Atributos internos do Code (LineNumberTable, LocalVariableTable, ...) */
    u2 attr_count = br_u2(r);
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Attributes count:", attr_count);
    for (u2 i = 0; i < attr_count; i++) {
        u2 name_index = br_u2(r);
        u4 len = br_u4(r);
        const u1 *info = br_bytes(r, len);
        if (!br_ok(r)) return;
        print_attribute(cf, i, name_index, info, len, depth);
    }
}

/* ---------------------------------------------------------------------- */
/* Despacho por nome                                                      */
/* ---------------------------------------------------------------------- */

static const struct {
    const char *name;
    AttrDecoder fn;
} ATTR_DECODERS[] = {
    {"Code",               decode_code},
    {"SourceFile",         decode_source_file},
    {"ConstantValue",      decode_constant_value},
    {"Signature",          decode_signature},
    {"Exceptions",         decode_exceptions},
    {"LineNumberTable",    decode_line_number_table},
    {"LocalVariableTable", decode_local_variable_table},
    {NULL, NULL}
};

static AttrDecoder find_decoder(const char *name)
{
    for (int i = 0; ATTR_DECODERS[i].name; i++)
        if (strcmp(ATTR_DECODERS[i].name, name) == 0)
            return ATTR_DECODERS[i].fn;
    return NULL;
}

static void print_attribute(const ClassFile *cf, u2 index, u2 name_index,
                            const u1 *info, u4 length, int depth)
{
    print_indent(depth);
    printf("[%u] %s\n", index, cp_utf8(cf, name_index));
    print_cp_ref(cf, "Attribute name index:", name_index, depth + 2);
    print_indent(depth + 2);
    printf("%-*s %u\n", LABEL_WIDTH, "Attribute length:", (unsigned)length);

    if (depth > MAX_ATTR_DEPTH) {
        print_indent(depth + 2);
        printf("<aninhamento excessivo de atributos -- conteúdo omitido>\n");
        return;
    }

    AttrDecoder fn = find_decoder(cp_utf8(cf, name_index));
    if (!fn) {
        print_indent(depth + 2);
        printf("(atributo não decodificado) -- dump hexadecimal:\n");
        print_hexdump(info, length, depth + 3);
        return;
    }

    ByteReader r;
    br_init(&r, info, length);
    fn(cf, &r, depth + 2);
    if (!br_ok(&r)) {
        print_indent(depth + 2);
        printf("<atributo truncado/malformado>\n");
    } else if (br_remaining(&r) > 0) {
        print_indent(depth + 2);
        printf("<aviso: %lu byte(s) não lidos no fim do atributo>\n", (unsigned long)br_remaining(&r));
    }
}

void print_attributes(const ClassFile *cf, u2 count, const attribute_info *attrs, int depth)
{
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Attributes count:", count);
    for (u2 i = 0; i < count; i++)
        print_attribute(cf, i, attrs[i].attribute_name_index, attrs[i].info,
                        attrs[i].attribute_length, depth);
}

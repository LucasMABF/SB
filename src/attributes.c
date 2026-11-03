/**
 * @file attributes.c
 * @brief Despacho de atributos por nome + decodificadores de Code e SourceFile.
 */
#include "attributes.h"
#include "attributes_internal.h"
#include "bytecode.h"
#include "constant_pool.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

static const struct {
    const char *name;
    AttrDecoder fn;
} ATTR_DECODERS[] = {
    {"Code",                   decode_code},
    {"SourceFile",             decode_source_file},
    {"ConstantValue",          decode_constant_value},
    {"Exceptions",             decode_exceptions},
    {"LineNumberTable",        decode_line_number_table},
    {"LocalVariableTable",     decode_local_variable_table},
    {"LocalVariableTypeTable", decode_local_variable_table}, /* mesmo formato */
    {"InnerClasses",           decode_inner_classes},
    {"Signature",              decode_signature},
    {"Deprecated",             decode_empty},
    {"Synthetic",              decode_empty},
    {"StackMapTable",          decode_stack_map_table},
    {"BootstrapMethods",       decode_bootstrap_methods},
    {"EnclosingMethod",        decode_enclosing_method},
    {NULL, NULL}
};

static AttrDecoder find_decoder(const char *name)
{
    for (int i = 0; ATTR_DECODERS[i].name; i++)
        if (strcmp(ATTR_DECODERS[i].name, name) == 0)
            return ATTR_DECODERS[i].fn;
    return NULL;
}

void print_attribute_raw(const ClassFile *cf, u2 index, u2 name_index,
                         const u1 *info, u4 length, int depth)
{
    const char *name = cp_utf8(cf, name_index);

    print_indent(depth);
    printf("[%u] %s\n", index, name);
    print_cp_ref(cf, "Attribute name index:", name_index, depth + 2);
    print_indent(depth + 2);
    printf("%-*s %u\n", LABEL_WIDTH, "Attribute length:", (unsigned)length);

    AttrDecoder fn = find_decoder(name);
    ByteReader r;
    br_init(&r, info, length);

    if (fn && fn(cf, &r, depth + 2)) {
        if (!br_ok(&r)) {
            print_indent(depth + 2);
            printf("<atributo truncado/malformado>\n");
        } else if (br_remaining(&r) > 0) {
            print_indent(depth + 2);
            printf("<aviso: %zu byte(s) não lidos no fim do atributo>\n", br_remaining(&r));
        }
        return;
    }

    print_indent(depth + 2);
    printf("%s -- dump hexadecimal:\n",
           fn ? "(decodificador não implementado)" : "(atributo desconhecido)");
    print_hexdump(info, length, depth + 3);
}

void print_attributes(const ClassFile *cf, u2 count, const attribute_info *attrs, int depth)
{
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Attributes count:", count);
    for (u2 i = 0; i < count; i++)
        print_attribute_raw(cf, i, attrs[i].attribute_name_index, attrs[i].info,
                            attrs[i].attribute_length, depth);
}

/* ---------------------------------------------------------------------- */
/* SourceFile (JVMS §4.7.10) -- exemplo de decodificador simples          */
/* ---------------------------------------------------------------------- */

int decode_source_file(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 idx = br_u2(r);
    print_cp_ref(cf, "Source file name:", idx, depth);
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Code (JVMS §4.7.3) -- exemplo de decodificador com tabela e aninhamento*/
/* ---------------------------------------------------------------------- */

int decode_code(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 max_stack = br_u2(r);
    u2 max_locals = br_u2(r);
    u4 code_length = br_u4(r);
    const u1 *code = br_bytes(r, code_length);
    if (!br_ok(r)) return 1;

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
        if (!br_ok(r)) return 1;
        print_indent(depth + 1);
        printf("%-4u %-8u %-8u %-8u ", i, start, end, handler);
        if (catch_type == 0)
            printf("cp #0 <any>\n");
        else
            printf("cp #%u <%s>\n", catch_type, cp_class_name(cf, catch_type));
    }

    /* Atributos aninhados (LineNumberTable, LocalVariableTable, StackMapTable...) */
    u2 attr_count = br_u2(r);
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, "Attributes count:", attr_count);
    for (u2 i = 0; i < attr_count; i++) {
        u2 name_index = br_u2(r);
        u4 len = br_u4(r);
        const u1 *info = br_bytes(r, len);
        if (!br_ok(r)) return 1;
        print_attribute_raw(cf, i, name_index, info, len, depth);
    }
    return 1;
}

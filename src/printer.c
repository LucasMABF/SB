/**
 * @file printer.c
 * @brief Exibição das seções do .class.
 */
#include "printer.h"
#include "access_flags.h"
#include "attributes.h"
#include "constant_pool.h"
#include "util.h"

#include <stdio.h>

/*
 * Nome da versão do Java para major_version, como o jclasslib faz
 * ("Major version: 52 [1.8]"). JVMS §4.1, tabela 4.1-A:
 *   45 -> 1.1 ... 52 -> 1.8 (nomenclatura antiga "1.x"),
 *   53 -> 9, 54 -> 10, ... (a partir do Java 9: versão = major - 44).
 */
static const char *java_version_name(u2 major, char *buf, size_t buf_len)
{
    if (major < 45)
        snprintf(buf, buf_len, "inválida");
    else if (major <= 52)
        snprintf(buf, buf_len, "1.%u", major - 44u);
    else
        snprintf(buf, buf_len, "%u", major - 44u);
    return buf;
}

static void print_u_line(int depth, const char *label, unsigned value)
{
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, label, value);
}

static void print_flags_line(int depth, u2 flags, FlagContext ctx)
{
    char buf[256];
    flags_to_string(flags, ctx, buf, sizeof buf);
    print_indent(depth);
    printf("%-*s 0x%04X [%s]\n", LABEL_WIDTH, "Access flags:", flags, buf);
}

void print_general_info(const ClassFile *cf)
{
    char version[16];
    print_section("GENERAL INFORMATION");
    printf("%-*s 0x%08X\n", LABEL_WIDTH, "Magic number:", (unsigned)cf->magic);
    print_u_line(0, "Minor version:", cf->minor_version);
    printf("%-*s %u [%s]\n", LABEL_WIDTH, "Major version:", cf->major_version,
           java_version_name(cf->major_version, version, sizeof version));
    print_u_line(0, "Constant pool count:", cf->constant_pool_count);
    print_flags_line(0, cf->access_flags, FLAGS_CLASS);
    print_cp_ref(cf, "This class:", cf->this_class, 0);
    if (cf->super_class == 0)
        printf("%-*s cp #0 <nenhuma (java/lang/Object)>\n", LABEL_WIDTH, "Super class:");
    else
        print_cp_ref(cf, "Super class:", cf->super_class, 0);
    print_u_line(0, "Interfaces count:", cf->interfaces_count);
    print_u_line(0, "Fields count:", cf->fields_count);
    print_u_line(0, "Methods count:", cf->methods_count);
    print_u_line(0, "Attributes count:", cf->attributes_count);
}

void print_interfaces(const ClassFile *cf)
{
    print_section("INTERFACES");
    printf("Interfaces count: %u\n", cf->interfaces_count);
    for (u2 i = 0; i < cf->interfaces_count; i++) {
        char label[32];
        snprintf(label, sizeof label, "[%u] Interface:", i);
        print_cp_ref(cf, label, cf->interfaces[i], 0);
    }
}

static void print_member(const ClassFile *cf, const member_info *m, u2 i, FlagContext ctx)
{
    printf("\n[%u] %s\n", i, cp_utf8(cf, m->name_index));
    print_cp_ref(cf, "Name:", m->name_index, 2);
    print_cp_ref(cf, "Descriptor:", m->descriptor_index, 2);
    print_flags_line(2, m->access_flags, ctx);
    print_attributes(cf, m->attributes_count, m->attributes, 2);
}

void print_fields(const ClassFile *cf)
{
    print_section("FIELDS");
    printf("Fields count: %u\n", cf->fields_count);
    for (u2 i = 0; i < cf->fields_count; i++)
        print_member(cf, &cf->fields[i], i, FLAGS_FIELD);
}

void print_methods(const ClassFile *cf)
{
    print_section("METHODS");
    printf("Methods count: %u\n", cf->methods_count);
    for (u2 i = 0; i < cf->methods_count; i++)
        print_member(cf, &cf->methods[i], i, FLAGS_METHOD);
}

void print_class_attributes(const ClassFile *cf)
{
    print_section("CLASS ATTRIBUTES");
    print_attributes(cf, cf->attributes_count, cf->attributes, 0);
}

void print_class(const ClassFile *cf)
{
    print_general_info(cf);
    print_constant_pool(cf);
    print_interfaces(cf);
    print_fields(cf);
    print_methods(cf);
    print_class_attributes(cf);
}

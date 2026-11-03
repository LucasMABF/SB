/**
 * @file classfile.c
 * @brief Parser do formato .class (JVMS §4.1).
 */
#include "classfile.h"
#include "reader.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLASS_MAGIC 0xCAFEBABEu

static void set_err(char *err, size_t len, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, len, fmt, ap);
    va_end(ap);
}

/* ---------------------------------------------------------------------- */
/* Pool de constantes                                                     */
/* ---------------------------------------------------------------------- */

static int parse_constant_pool(ByteReader *r, ClassFile *cf, char *err, size_t err_len)
{
    u2 count = cf->constant_pool_count;
    if (count == 0) {
        set_err(err, err_len, "constant_pool_count igual a 0 (mínimo é 1)");
        return 0;
    }

    cf->constant_pool = calloc(count, sizeof(cp_info));
    if (!cf->constant_pool) {
        set_err(err, err_len, "memória insuficiente");
        return 0;
    }

    for (u2 i = 1; i < count; i++) {
        cp_info *e = &cf->constant_pool[i];
        e->tag = br_u1(r);

        switch (e->tag) {
        case CONSTANT_Utf8: {
            u2 len = br_u2(r);
            const u1 *src = br_bytes(r, len);
            if (!src) break;
            e->info.Utf8.length = len;
            e->info.Utf8.bytes = malloc((size_t)len + 1);
            if (!e->info.Utf8.bytes) {
                set_err(err, err_len, "memória insuficiente");
                return 0;
            }
            memcpy(e->info.Utf8.bytes, src, len);
            e->info.Utf8.bytes[len] = '\0';
            break;
        }
        case CONSTANT_Integer:
        case CONSTANT_Float:
            e->info.Integer.bytes = br_u4(r);
            break;
        case CONSTANT_Long:
        case CONSTANT_Double:
            e->info.Long.high_bytes = br_u4(r);
            e->info.Long.low_bytes = br_u4(r);
            /* JVMS §4.4.5: Long/Double ocupam dois índices; o seguinte é inutilizável. */
            if (i + 1 >= count) {
                set_err(err, err_len, "constante de 8 bytes no último índice do pool (#%u)", i);
                return 0;
            }
            i++; /* cf->constant_pool[i].tag continua 0 (calloc) */
            break;
        case CONSTANT_Class:
        case CONSTANT_Module:
        case CONSTANT_Package:
            e->info.Class.name_index = br_u2(r);
            break;
        case CONSTANT_String:
            e->info.String.string_index = br_u2(r);
            break;
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
            e->info.Ref.class_index = br_u2(r);
            e->info.Ref.name_and_type_index = br_u2(r);
            break;
        case CONSTANT_NameAndType:
            e->info.NameAndType.name_index = br_u2(r);
            e->info.NameAndType.descriptor_index = br_u2(r);
            break;
        case CONSTANT_MethodHandle:
            e->info.MethodHandle.reference_kind = br_u1(r);
            e->info.MethodHandle.reference_index = br_u2(r);
            break;
        case CONSTANT_MethodType:
            e->info.MethodType.descriptor_index = br_u2(r);
            break;
        case CONSTANT_Dynamic:
        case CONSTANT_InvokeDynamic:
            e->info.Dynamic.bootstrap_method_attr_index = br_u2(r);
            e->info.Dynamic.name_and_type_index = br_u2(r);
            break;
        default:
            if (br_ok(r)) {
                set_err(err, err_len, "tag desconhecida %u no pool de constantes, índice #%u", e->tag, i);
                return 0;
            }
        }

        if (!br_ok(r)) {
            set_err(err, err_len, "arquivo truncado dentro do pool de constantes (índice #%u)", i);
            return 0;
        }
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Atributos, fields e methods                                            */
/* ---------------------------------------------------------------------- */

static int parse_attributes(ByteReader *r, u2 count, attribute_info **out)
{
    *out = NULL;
    if (count == 0) return 1;

    attribute_info *attrs = calloc(count, sizeof(attribute_info));
    if (!attrs) return 0;
    *out = attrs; /* atribuído logo para que classfile_free libere em caso de erro */

    for (u2 i = 0; i < count; i++) {
        attrs[i].attribute_name_index = br_u2(r);
        attrs[i].attribute_length = br_u4(r);
        const u1 *src = br_bytes(r, attrs[i].attribute_length);
        if (!src) return 0;
        if (attrs[i].attribute_length > 0) {
            attrs[i].info = malloc(attrs[i].attribute_length);
            if (!attrs[i].info) return 0;
            memcpy(attrs[i].info, src, attrs[i].attribute_length);
        }
    }
    return 1;
}

static int parse_members(ByteReader *r, u2 count, member_info **out)
{
    *out = NULL;
    if (count == 0) return 1;

    member_info *m = calloc(count, sizeof(member_info));
    if (!m) return 0;
    *out = m;

    for (u2 i = 0; i < count; i++) {
        m[i].access_flags = br_u2(r);
        m[i].name_index = br_u2(r);
        m[i].descriptor_index = br_u2(r);
        m[i].attributes_count = br_u2(r);
        if (!br_ok(r)) return 0;
        if (!parse_attributes(r, m[i].attributes_count, &m[i].attributes)) return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* API pública                                                            */
/* ---------------------------------------------------------------------- */

ClassFile *classfile_load(const char *path, char *err, size_t err_len)
{
    size_t size;
    u1 *data = read_whole_file(path, &size);
    if (!data) {
        set_err(err, err_len, "não foi possível ler o arquivo '%s'", path);
        return NULL;
    }

    ClassFile *cf = calloc(1, sizeof(ClassFile));
    if (!cf) {
        free(data);
        set_err(err, err_len, "memória insuficiente");
        return NULL;
    }

    ByteReader r;
    br_init(&r, data, size);

    cf->magic = br_u4(&r);
    if (!br_ok(&r) || cf->magic != CLASS_MAGIC) {
        set_err(err, err_len, "magic number inválido (0x%08X, esperado 0xCAFEBABE): não é um arquivo .class",
                (unsigned)cf->magic);
        goto fail;
    }

    cf->minor_version = br_u2(&r);
    cf->major_version = br_u2(&r);
    cf->constant_pool_count = br_u2(&r);
    if (!br_ok(&r)) {
        set_err(err, err_len, "arquivo truncado no cabeçalho");
        goto fail;
    }

    if (!parse_constant_pool(&r, cf, err, err_len)) goto fail;

    cf->access_flags = br_u2(&r);
    cf->this_class = br_u2(&r);
    cf->super_class = br_u2(&r);
    cf->interfaces_count = br_u2(&r);
    if (!br_ok(&r)) {
        set_err(err, err_len, "arquivo truncado após o pool de constantes");
        goto fail;
    }

    if (cf->interfaces_count > 0) {
        cf->interfaces = calloc(cf->interfaces_count, sizeof(u2));
        if (!cf->interfaces) {
            set_err(err, err_len, "memória insuficiente");
            goto fail;
        }
        for (u2 i = 0; i < cf->interfaces_count; i++)
            cf->interfaces[i] = br_u2(&r);
    }

    cf->fields_count = br_u2(&r);
    if (!br_ok(&r) || !parse_members(&r, cf->fields_count, &cf->fields)) {
        set_err(err, err_len, "erro ao ler os fields (arquivo truncado?)");
        goto fail;
    }

    cf->methods_count = br_u2(&r);
    if (!br_ok(&r) || !parse_members(&r, cf->methods_count, &cf->methods)) {
        set_err(err, err_len, "erro ao ler os métodos (arquivo truncado?)");
        goto fail;
    }

    cf->attributes_count = br_u2(&r);
    if (!br_ok(&r) || !parse_attributes(&r, cf->attributes_count, &cf->attributes)) {
        set_err(err, err_len, "erro ao ler os atributos da classe (arquivo truncado?)");
        goto fail;
    }

    if (br_remaining(&r) > 0)
        fprintf(stderr, "Aviso: %lu byte(s) extra(s) no fim do arquivo foram ignorados\n",
                (unsigned long)br_remaining(&r));

    free(data);
    return cf;

fail:
    free(data);
    classfile_free(cf);
    return NULL;
}

static void free_attributes(attribute_info *attrs, u2 count)
{
    if (!attrs) return;
    for (u2 i = 0; i < count; i++)
        free(attrs[i].info);
    free(attrs);
}

static void free_members(member_info *m, u2 count)
{
    if (!m) return;
    for (u2 i = 0; i < count; i++)
        free_attributes(m[i].attributes, m[i].attributes_count);
    free(m);
}

void classfile_free(ClassFile *cf)
{
    if (!cf) return;
    if (cf->constant_pool) {
        for (u2 i = 1; i < cf->constant_pool_count; i++)
            if (cf->constant_pool[i].tag == CONSTANT_Utf8)
                free(cf->constant_pool[i].info.Utf8.bytes);
        free(cf->constant_pool);
    }
    free(cf->interfaces);
    free_members(cf->fields, cf->fields_count);
    free_members(cf->methods, cf->methods_count);
    free_attributes(cf->attributes, cf->attributes_count);
    free(cf);
}

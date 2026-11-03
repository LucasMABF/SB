/**
 * @file classfile.h
 * @brief Estruturas do arquivo .class (JVMS §4) e funções de carga/liberação.
 *
 * Decisão de projeto: os atributos são guardados como bytes brutos
 * (attribute_info.info). Eles são decodificados apenas na hora de exibir
 * (ver attributes.c). Isso mantém o parser pequeno e deixa cada
 * decodificador de atributo independente dos outros.
 */
#ifndef CLASSFILE_H
#define CLASSFILE_H

#include "types.h"

/* Tags do pool de constantes (JVMS §4.4, tabela 4.4-B) */
enum {
    CONSTANT_Utf8               = 1,
    CONSTANT_Integer            = 3,
    CONSTANT_Float              = 4,
    CONSTANT_Long               = 5,
    CONSTANT_Double             = 6,
    CONSTANT_Class              = 7,
    CONSTANT_String             = 8,
    CONSTANT_Fieldref           = 9,
    CONSTANT_Methodref          = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_NameAndType        = 12,
    CONSTANT_MethodHandle       = 15,
    CONSTANT_MethodType         = 16,
    CONSTANT_Dynamic            = 17,
    CONSTANT_InvokeDynamic      = 18,
    CONSTANT_Module             = 19,
    CONSTANT_Package            = 20
};

/**
 * Uma entrada do pool de constantes.
 * tag == 0 indica slot inutilizável: o índice 0 e o slot logo depois
 * de um Long/Double (que ocupam duas posições).
 */
typedef struct {
    u1 tag;
    union {
        struct { u2 name_index; } Class;                                /* também Module, Package */
        struct { u2 class_index; u2 name_and_type_index; } Ref;         /* Field/Method/InterfaceMethodref */
        struct { u2 string_index; } String;
        struct { u4 bytes; } Integer;                                   /* também Float */
        struct { u4 high_bytes; u4 low_bytes; } Long;                   /* também Double */
        struct { u2 name_index; u2 descriptor_index; } NameAndType;
        struct { u2 length; u1 *bytes; } Utf8;                          /* bytes terminados em '\0' */
        struct { u1 reference_kind; u2 reference_index; } MethodHandle;
        struct { u2 descriptor_index; } MethodType;
        struct { u2 bootstrap_method_attr_index; u2 name_and_type_index; } Dynamic; /* também InvokeDynamic */
    } info;
} cp_info;

typedef struct {
    u2  attribute_name_index;
    u4  attribute_length;
    u1 *info;                 /* attribute_length bytes brutos */
} attribute_info;

/* field_info e method_info têm o mesmo formato (JVMS §4.5 e §4.6). */
typedef struct {
    u2              access_flags;
    u2              name_index;
    u2              descriptor_index;
    u2              attributes_count;
    attribute_info *attributes;
} member_info;

typedef struct {
    u4              magic;
    u2              minor_version;
    u2              major_version;
    u2              constant_pool_count;
    cp_info        *constant_pool;     /* índices válidos: 1 .. constant_pool_count-1 */
    u2              access_flags;
    u2              this_class;
    u2              super_class;
    u2              interfaces_count;
    u2             *interfaces;
    u2              fields_count;
    member_info    *fields;
    u2              methods_count;
    member_info    *methods;
    u2              attributes_count;
    attribute_info *attributes;
} ClassFile;

/**
 * Lê e faz o parse de um arquivo .class.
 * @param err  buffer que recebe a mensagem de erro em caso de falha.
 * @return ClassFile alocado, ou NULL em caso de erro.
 */
ClassFile *classfile_load(const char *path, char *err, size_t err_len);

/**
 * Faz o parse de um .class que já está em memória (usado por classfile_load
 * e pelo harness do libFuzzer). O buffer não é modificado nem guardado.
 */
ClassFile *classfile_parse(const u1 *data, size_t size, char *err, size_t err_len);

void classfile_free(ClassFile *cf);

#endif

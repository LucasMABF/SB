/**
 * @file attributes_internal.h
 * @brief Interface comum dos decodificadores de atributos.
 *
 * Cada decodificador recebe um ByteReader posicionado no início do campo
 * `info` do atributo (logo depois de attribute_length) e imprime o conteúdo.
 *
 * Retorno:
 *   1 -> decodificou (mesmo que parcialmente; se o leitor marcar erro, o
 *        chamador avisa que o atributo está truncado)
 *   0 -> não implementado / não reconhecido: o chamador exibe um dump hex.
 *
 * Para adicionar um atributo novo: implemente a função em attr_decoders.c e
 * registre o nome na tabela ATTR_DECODERS em attributes.c.
 */
#ifndef ATTRIBUTES_INTERNAL_H
#define ATTRIBUTES_INTERNAL_H

#include "classfile.h"
#include "reader.h"

typedef int (*AttrDecoder)(const ClassFile *cf, ByteReader *r, int depth);

/* attributes.c */
int decode_code(const ClassFile *cf, ByteReader *r, int depth);
int decode_source_file(const ClassFile *cf, ByteReader *r, int depth);

/* attr_decoders.c (TODO: Pessoa C) */
int decode_constant_value(const ClassFile *cf, ByteReader *r, int depth);
int decode_exceptions(const ClassFile *cf, ByteReader *r, int depth);
int decode_line_number_table(const ClassFile *cf, ByteReader *r, int depth);
int decode_local_variable_table(const ClassFile *cf, ByteReader *r, int depth);
int decode_inner_classes(const ClassFile *cf, ByteReader *r, int depth);
int decode_signature(const ClassFile *cf, ByteReader *r, int depth);
int decode_empty(const ClassFile *cf, ByteReader *r, int depth);
int decode_stack_map_table(const ClassFile *cf, ByteReader *r, int depth);
int decode_bootstrap_methods(const ClassFile *cf, ByteReader *r, int depth);
int decode_enclosing_method(const ClassFile *cf, ByteReader *r, int depth);

#endif

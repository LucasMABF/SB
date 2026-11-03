/**
 * @file attributes.h
 * @brief Decodificação e exibição de atributos (JVMS §4.7).
 */
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "classfile.h"

/** Imprime "Attributes count: N" seguido de cada atributo. */
void print_attributes(const ClassFile *cf, u2 count, const attribute_info *attrs, int depth);

/**
 * Imprime um atributo a partir dos seus bytes brutos. Usado tanto para os
 * atributos já separados pelo parser quanto para atributos aninhados
 * (ex.: LineNumberTable dentro de Code).
 */
void print_attribute_raw(const ClassFile *cf, u2 index, u2 name_index,
                         const u1 *info, u4 length, int depth);

#endif

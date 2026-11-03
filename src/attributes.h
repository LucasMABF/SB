/**
 * @file attributes.h
 * @brief Decodificação e exibição de atributos (JVMS §4.7).
 */
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "classfile.h"

/**
 * Imprime "Attributes count: N" seguido de cada atributo. Os atributos
 * conhecidos (Code, SourceFile, ConstantValue, Signature, Exceptions,
 * LineNumberTable, LocalVariableTable) são decodificados; os demais aparecem
 * em hexadecimal.
 */
void print_attributes(const ClassFile *cf, u2 count, const attribute_info *attrs, int depth);

#endif

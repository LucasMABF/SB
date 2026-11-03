/**
 * @file printer.h
 * @brief Exibição das seções do .class (visão geral, interfaces, fields, métodos, atributos).
 */
#ifndef PRINTER_H
#define PRINTER_H

#include "classfile.h"

/** Exibe o arquivo .class completo, seção por seção. */
void print_class(const ClassFile *cf);

void print_general_info(const ClassFile *cf);
void print_interfaces(const ClassFile *cf);
void print_fields(const ClassFile *cf);
void print_methods(const ClassFile *cf);
void print_class_attributes(const ClassFile *cf);

#endif

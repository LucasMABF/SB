/**
 * @file printer.h
 * @brief Exibição das seções do .class.
 */
#ifndef PRINTER_H
#define PRINTER_H

#include "classfile.h"

/**
 * Exibe o arquivo .class completo, seção por seção: informações gerais,
 * pool de constantes, interfaces, fields, métodos e atributos da classe.
 */
void print_class(const ClassFile *cf);

#endif

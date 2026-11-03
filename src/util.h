/**
 * @file util.h
 * @brief Funções auxiliares de formatação da saída.
 */
#ifndef UTIL_H
#define UTIL_H

#include "types.h"

/** Largura da coluna de rótulos ("Max stack:      ..."). */
#define LABEL_WIDTH 22

/** Imprime 2*depth espaços. */
void print_indent(int depth);

/** Imprime um título de seção. */
void print_section(const char *title);

/** Dump hexadecimal (16 bytes por linha), usado para atributos desconhecidos/não implementados. */
void print_hexdump(const u1 *data, u4 len, int depth);

#endif

/**
 * @file bytecode.h
 * @brief Disassembler: exibe as instruções de um atributo Code.
 */
#ifndef BYTECODE_H
#define BYTECODE_H

#include "classfile.h"

/**
 * Exibe o bytecode no formato:
 *     pc  mnemônico operandos
 * Operandos que apontam para o pool mostram o índice e o valor resolvido,
 * ex.:  "   1  invokespecial #1 <java/lang/Object.<init> : ()V>".
 */
void print_bytecode(const ClassFile *cf, const u1 *code, u4 code_length, int depth);

#endif

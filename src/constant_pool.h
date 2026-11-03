/**
 * @file constant_pool.h
 * @brief Acesso, resolução e exibição do pool de constantes.
 */
#ifndef CONSTANT_POOL_H
#define CONSTANT_POOL_H

#include "classfile.h"

/** Nome da tag no estilo do jclasslib, ex.: "CONSTANT_Methodref_info". */
const char *cp_tag_name(u1 tag);

/**
 * Retorna a entrada `idx` se ela existir e tiver a tag `expected_tag`
 * (passe 0 para aceitar qualquer tag). Caso contrário, retorna NULL.
 */
const cp_info *cp_get(const ClassFile *cf, u2 idx, u1 expected_tag);

/** String de um CONSTANT_Utf8 (ou "<inválido>" se o índice não for Utf8). */
const char *cp_utf8(const ClassFile *cf, u2 idx);

/** Nome da classe de um CONSTANT_Class (ou "<inválido>"). */
const char *cp_class_name(const ClassFile *cf, u2 idx);

/* Valores numéricos (a entrada precisa ter a tag correspondente). */
int32_t cp_int_value(const cp_info *e);
float   cp_float_value(const cp_info *e);
int64_t cp_long_value(const cp_info *e);
double  cp_double_value(const cp_info *e);

/**
 * Formata float/double como o Java (Float.toString / Double.toString):
 * "1.0", "3.1415", "1.0E10", "NaN", "Infinity", "-Infinity".
 * Usa o menor número de dígitos que reproduz exatamente o valor.
 */
void format_float(float f, char *buf, size_t buf_len);
void format_double(double d, char *buf, size_t buf_len);

/**
 * Converte uma string Modified UTF-8 (JVMS §4.4.7) para UTF-8 padrão,
 * escapando caracteres de controle (\n, \t, \0, \uXXXX...) para não quebrar
 * o layout da saída. Pares de surrogates viram um único caractere UTF-8.
 * O resultado é truncado em buf_len-1 bytes e sempre termina em '\0'.
 * Para nunca truncar, use buf_len >= 6 * len + 1.
 * @return número de caracteres em UTF-16 (o "Length of string" do jclasslib).
 */
size_t mutf8_format(const u1 *bytes, size_t len, char *buf, size_t buf_len);

/**
 * Escreve em `buf` a representação "resolvida" da constante `idx`, ou seja,
 * o valor que o jclasslib mostra entre < >. Exemplos:
 *   Class        -> java/lang/Object
 *   Methodref    -> java/io/PrintStream.println : (I)V
 *   NameAndType  -> <init> : ()V
 *   String       -> "Ola mundo"
 *   Integer      -> 42
 */
void cp_describe(const ClassFile *cf, u2 idx, char *buf, size_t buf_len);

/**
 * Imprime uma linha "Label: cp #idx <valor resolvido>" com a indentação dada.
 * Use em todo lugar que exibe um índice para o pool (fields, atributos...).
 */
void print_cp_ref(const ClassFile *cf, const char *label, u2 idx, int depth);

/** Imprime a seção completa do pool de constantes. */
void print_constant_pool(const ClassFile *cf);

#endif

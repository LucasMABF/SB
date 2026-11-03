/**
 * @file reader.h
 * @brief Leitura big-endian sobre um buffer em memória, com checagem de limites.
 *
 * O arquivo .class inteiro é carregado em memória e lido por um ByteReader.
 * O mesmo leitor é reutilizado para decodificar o conteúdo dos atributos
 * (que ficam guardados como bytes brutos no ClassFile).
 *
 * Se uma leitura passar do fim do buffer, `error` vira 1 e todas as leituras
 * seguintes retornam 0/NULL. Assim o código que usa o leitor pode ler tudo
 * em sequência e checar `br_ok()` uma única vez no final.
 */
#ifndef READER_H
#define READER_H

#include "types.h"

typedef struct {
    const u1 *data;
    size_t    size;
    size_t    pos;
    int       error;
} ByteReader;

void       br_init(ByteReader *r, const u1 *data, size_t size);
u1         br_u1(ByteReader *r);
u2         br_u2(ByteReader *r);
u4         br_u4(ByteReader *r);
/** Retorna um ponteiro para os próximos n bytes e avança; NULL se não houver bytes suficientes. */
const u1  *br_bytes(ByteReader *r, size_t n);
size_t     br_remaining(const ByteReader *r);
int        br_ok(const ByteReader *r);

/**
 * Lê um arquivo inteiro para a memória.
 * @return buffer alocado com malloc (o chamador libera) ou NULL em caso de erro.
 */
u1 *read_whole_file(const char *path, size_t *out_size);

#endif

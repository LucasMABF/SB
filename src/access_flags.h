/**
 * @file access_flags.h
 * @brief Decodificação das access_flags.
 *
 * O mesmo bit tem significados diferentes conforme o contexto
 * (ex.: 0x0020 é ACC_SUPER numa classe e ACC_SYNCHRONIZED num método),
 * por isso o contexto precisa ser informado.
 */
#ifndef ACCESS_FLAGS_H
#define ACCESS_FLAGS_H

#include "types.h"

typedef enum {
    FLAGS_CLASS,        /* JVMS tabela 4.1-B */
    FLAGS_FIELD,        /* JVMS tabela 4.5-A */
    FLAGS_METHOD,       /* JVMS tabela 4.6-A */
    FLAGS_INNER_CLASS   /* JVMS tabela 4.7.6-A (atributo InnerClasses) */
} FlagContext;

/** Escreve em buf algo como "public static final". */
void flags_to_string(u2 flags, FlagContext ctx, char *buf, size_t buf_len);

#endif

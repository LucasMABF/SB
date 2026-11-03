/**
 * @file util.c
 * @brief Funções auxiliares de formatação da saída.
 */
#include "util.h"

#include <stdio.h>

void print_indent(int depth)
{
    for (int i = 0; i < depth; i++)
        fputs("  ", stdout);
}

void print_section(const char *title)
{
    printf("\n==================================================================\n");
    printf("  %s\n", title);
    printf("==================================================================\n");
}

void print_hexdump(const u1 *data, u4 len, int depth)
{
    for (u4 i = 0; i < len; i += 16) {
        print_indent(depth);
        printf("%04X: ", (unsigned)i);
        for (u4 j = i; j < i + 16 && j < len; j++)
            printf(" %02X", data[j]);
        printf("\n");
    }
}

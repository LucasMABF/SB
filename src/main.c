/**
 * @file main.c
 * @brief Leitor-exibidor de arquivos .class.
 *
 * Uso: ./leitor-exibidor <arquivo.class>
 * A saída vai para stdout; para salvar em arquivo:  ./leitor-exibidor X.class > X.txt
 */
#include "classfile.h"
#include "printer.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.class>\n", argv[0]);
        return 1;
    }

    char err[512];
    ClassFile *cf = classfile_load(argv[1], err, sizeof err);
    if (!cf) {
        fprintf(stderr, "Erro ao ler '%s': %s\n", argv[1], err);
        return 1;
    }

    print_class(cf);
    classfile_free(cf);
    return 0;
}

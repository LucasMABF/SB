/**
 * @file fuzz_classfile.c
 * @brief Harness do libFuzzer: parse + exibição de um .class arbitrário.
 *
 * O libFuzzer gera entradas mutando os .class de exemplo; cada entrada passa
 * pelo parser e, se for aceita, por toda a exibição (pool, bytecode,
 * atributos). Com -fsanitize=address,undefined, qualquer leitura fora dos
 * limites, vazamento ou comportamento indefinido vira um crash reportado.
 *
 * Compilar/rodar: make fuzz  (precisa do clang)
 */
#include "classfile.h"
#include "printer.h"

#include <stdio.h>

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    /* A saída do exibidor não interessa ao fuzzer, só os crashes.
     * O stderr NÃO é redirecionado: é por ele que o ASan/libFuzzer reportam. */
    if (!freopen("/dev/null", "w", stdout)) return 1;
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char err[512];
    ClassFile *cf = classfile_parse(data, size, err, sizeof err);
    if (cf) {
        print_class(cf);
        classfile_free(cf);
    }
    return 0;
}

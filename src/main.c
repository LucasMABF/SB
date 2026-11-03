/**
 * @file main.c
 * @brief Leitor-exibidor de arquivos .class.
 *
 * Uso: ./leitor-exibidor [opções] <arquivo.class>
 *   -s <seção>    exibe só a seção indicada (pode repetir a opção):
 *                 geral, pool, interfaces, fields, metodos, atributos
 *   -i            modo interativo: menu para escolher a seção
 *   -o <arquivo>  grava a saída em <arquivo> em vez de exibir no terminal
 *   -h            ajuda
 * Sem opções, exibe todas as seções no terminal (stdout).
 */
#include "classfile.h"
#include "constant_pool.h"
#include "printer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {
    const char *name;   /* nome usado em -s */
    const char *alias;  /* nome alternativo (inglês) */
    const char *title;  /* texto do menu interativo */
    void (*print)(const ClassFile *cf);
} Section;

static const Section SECTIONS[] = {
    {"geral",      "general",    "Informações gerais",            print_general_info},
    {"pool",       "cp",         "Pool de constantes",            print_constant_pool},
    {"interfaces", "interface",  "Interfaces",                    print_interfaces},
    {"fields",     "campos",     "Fields (campos)",               print_fields},
    {"metodos",    "methods",    "Métodos (com bytecode)",        print_methods},
    {"atributos",  "attributes", "Atributos da classe",           print_class_attributes},
};
#define N_SECTIONS ((int)(sizeof SECTIONS / sizeof SECTIONS[0]))

static void usage(void)
{
    fprintf(stderr,
            "Uso: leitor-exibidor [opções] <arquivo.class>\n"
            "  -s <seção>    exibe só a seção indicada (pode repetir):\n"
            "                geral, pool, interfaces, fields, metodos, atributos\n"
            "  -i            modo interativo (menu de seções)\n"
            "  -o <arquivo>  grava a saída no arquivo em vez do terminal\n"
            "  -h            mostra esta ajuda\n"
            "Sem opções, exibe todas as seções.\n");
}

static int find_section(const char *name)
{
    for (int i = 0; i < N_SECTIONS; i++)
        if (strcmp(name, SECTIONS[i].name) == 0 || strcmp(name, SECTIONS[i].alias) == 0)
            return i;
    return -1;
}

/*
 * Avisa (sem impedir a exibição) quando o nome do arquivo não bate com
 * this_class. A JVM recusaria carregar uma classe assim (NoClassDefFoundError,
 * "wrong name"), mas para o exibidor isso é só informativo.
 */
static void check_class_name(const ClassFile *cf, const char *path)
{
    const char *base = path;
    for (const char *p = path; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    size_t len = strlen(base);
    if (len > 6 && strcmp(base + len - 6, ".class") == 0) len -= 6;

    const char *name = cp_class_name(cf, cf->this_class);
    const char *simple = strrchr(name, '/');
    simple = simple ? simple + 1 : name;

    if (strlen(simple) != len || strncmp(simple, base, len) != 0)
        fprintf(stderr, "Aviso: o arquivo '%s' contém a classe '%s' "
                        "(a JVM a recusaria com NoClassDefFoundError: wrong name)\n", path, name);
}

static void interactive_menu(const ClassFile *cf, const char *path)
{
    char line[64];
    for (;;) {
        printf("\n---------------- %s ----------------\n", path);
        for (int i = 0; i < N_SECTIONS; i++)
            printf("  %d. %s\n", i + 1, SECTIONS[i].title);
        printf("  %d. Todas as seções\n", N_SECTIONS + 1);
        printf("  0. Sair\n");
        printf("Opção: ");
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin)) break; /* EOF */
        char *end;
        long opt = strtol(line, &end, 10);
        if (end == line) {
            printf("Opção inválida.\n");
            continue;
        }
        if (opt == 0) break;
        if (opt >= 1 && opt <= N_SECTIONS) {
            SECTIONS[opt - 1].print(cf);
        } else if (opt == N_SECTIONS + 1) {
            print_class(cf);
        } else {
            printf("Opção inválida.\n");
        }
    }
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    /* A saída é UTF-8 (acentos, strings do pool); o console do Windows usa
     * outra página de código por padrão. */
    SetConsoleOutputCP(CP_UTF8);
#endif

    const char *path = NULL;
    const char *out_path = NULL;
    int interactive = 0;
    int selected[N_SECTIONS] = {0};
    int any_selected = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage();
            return 0;
        } else if (strcmp(a, "-i") == 0) {
            interactive = 1;
        } else if (strcmp(a, "-s") == 0 || strcmp(a, "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Erro: a opção %s precisa de um argumento\n", a);
                usage();
                return 1;
            }
            const char *value = argv[++i];
            if (a[1] == 'o') {
                out_path = value;
            } else {
                int s = find_section(value);
                if (s < 0) {
                    fprintf(stderr, "Erro: seção desconhecida '%s'\n", value);
                    usage();
                    return 1;
                }
                selected[s] = 1;
                any_selected = 1;
            }
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "Erro: opção desconhecida '%s'\n", a);
            usage();
            return 1;
        } else if (!path) {
            path = a;
        } else {
            fprintf(stderr, "Erro: mais de um arquivo informado\n");
            usage();
            return 1;
        }
    }

    if (!path) {
        usage();
        return 1;
    }
    if (interactive && (out_path || any_selected)) {
        fprintf(stderr, "Erro: -i não pode ser combinado com -o ou -s\n");
        return 1;
    }

    char err[512];
    ClassFile *cf = classfile_load(path, err, sizeof err);
    if (!cf) {
        fprintf(stderr, "Erro ao ler '%s': %s\n", path, err);
        return 1;
    }
    check_class_name(cf, path);

    if (out_path && !freopen(out_path, "w", stdout)) {
        fprintf(stderr, "Erro: não foi possível criar '%s'\n", out_path);
        classfile_free(cf);
        return 1;
    }

    if (interactive) {
        interactive_menu(cf, path);
    } else if (any_selected) {
        for (int i = 0; i < N_SECTIONS; i++)
            if (selected[i]) SECTIONS[i].print(cf);
    } else {
        print_class(cf);
    }

    classfile_free(cf);
    if (out_path) {
        fclose(stdout);
        fprintf(stderr, "Saída gravada em '%s'\n", out_path);
    }
    return 0;
}

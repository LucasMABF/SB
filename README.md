# Leitor-Exibidor de arquivos .class

Trabalho da disciplina de Software Básico (UnB). Lê um arquivo `.class` da JVM e
exibe suas estruturas, de forma similar ao jclasslib: informações gerais, pool de
constantes (com índices e valores resolvidos), interfaces, fields, métodos,
atributos e bytecode (mnemônicos, com os operandos que apontam para o pool
mostrando a constante referenciada).

## Compilação

Requisitos: GCC (C99) e GNU Make.

```sh
make
```

Opções de compilação utilizadas: `gcc -std=c99 -Wall -Wextra -pedantic -O2 -g`

Sem o Make (qualquer sistema com GCC):

```sh
gcc -std=c99 -Wall -Wextra -pedantic -O2 -g src/*.c -o leitor-exibidor
```

## Execução

```sh
./leitor-exibidor examples/Belote.class            # saída no terminal
./leitor-exibidor examples/Belote.class > out.txt  # saída em arquivo
```

Outros alvos:

```sh
make test   # roda em todos os exemplos de examples/, salva em out/ e mostra um resumo
make asan   # recompila com AddressSanitizer/UBSan (para depuração)
make clean
```

## Estrutura

| Arquivo | Responsabilidade |
|---|---|
| `src/main.c` | Ponto de entrada |
| `src/reader.[ch]` | Leitura big-endian com checagem de limites |
| `src/classfile.[ch]` | Estruturas do .class e parser |
| `src/constant_pool.[ch]` | Resolução e exibição do pool de constantes |
| `src/access_flags.[ch]` | Decodificação das access flags |
| `src/opcodes.[ch]` | Tabela de opcodes (mnemônico + tipo de operando) |
| `src/bytecode.[ch]` | Disassembler do atributo Code |
| `src/attributes.[ch]` | Despacho de atributos, Code e SourceFile |
| `src/attr_decoders.c` | Demais atributos (LineNumberTable, ConstantValue, ...) |
| `src/printer.[ch]` | Exibição das seções |

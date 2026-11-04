# Leitor-Exibidor de arquivos .class

Trabalho da disciplina de Software Básico (UnB). Lê um arquivo `.class` da JVM e
exibe suas estruturas, de forma similar ao jclasslib: informações gerais, pool de
constantes (com índices e valores resolvidos), interfaces, fields, métodos,
atributos e bytecode (mnemônicos, com os operandos que apontam para o pool
mostrando a constante referenciada).

## Compilação

Requisitos: GCC (C99) e GNU Make. Não há dependências externas.

```sh
make
```

Opções de compilação utilizadas: `gcc -std=c99 -Wall -Wextra -pedantic -O2 -g`
(compila sem nenhum warning).

Sem o Make (qualquer sistema com GCC):

```sh
gcc -std=c99 -Wall -Wextra -pedantic -O2 -g src/*.c -o leitor-exibidor
```

No Windows (MinGW), o mesmo comando gera `leitor-exibidor.exe`; com o Make do
MinGW, use `mingw32-make`.

## Execução

```sh
./leitor-exibidor examples/Belote.class            # saída no terminal
./leitor-exibidor examples/Belote.class > out.txt  # saída em arquivo
```

Código de saída: 0 em caso de sucesso, 1 em caso de erro (arquivo inexistente,
magic number inválido, arquivo truncado ou malformado).

## O que é exibido

- **Informações gerais**: magic, versões (com o nome da versão do Java, ex.: `52 [1.8]`),
  access flags decodificadas, this/super class e contadores.
- **Pool de constantes**: todas as entradas com índice e tipo; cada referência
  mostra o índice e o valor resolvido (`cp #12 <java/lang/Object>`). Integer,
  Float, Long e Double mostram os bytes e o valor; Long/Double ocupam dois slots.
  Strings com `\n`, `\t`, `\r` e o caractere nulo são exibidas com escape.
- **Interfaces, fields e métodos**: nome, descritor e access flags.
- **Bytecode**: os 202 opcodes da JVMS, com mnemônico e operandos. Operandos que
  apontam para o pool exibem a constante
  (`invokevirtual #4 <java/io/PrintStream.println : (I)V>`); desvios mostram o
  destino absoluto e o deslocamento (`ifeq 12 (+8)`). Inclui `tableswitch`,
  `lookupswitch` e `wide`.
- **Atributos decodificados**: Code (com a tabela de exceções), SourceFile,
  ConstantValue, Signature, Exceptions, LineNumberTable e LocalVariableTable.

### Não implementado

- Os demais atributos (ex.: StackMapTable, InnerClasses, BootstrapMethods,
  anotações) não são decodificados: aparecem como dump hexadecimal, com nome e
  tamanho. Dos exemplos da disciplina, só a StackMapTable aparece (em 2 métodos).
- Caracteres fora do BMP (ex.: emoji) em strings não são convertidos do
  formato Modified UTF-8.
- Interface gráfica em árvore como a do jclasslib: a exibição é em texto.

## Testes e analisadores

```sh
make test                # roda em examples/ e tests/classes/ e salva a saída em out/
make asan                # recompila com AddressSanitizer/UBSan
make cppcheck            # análise estática (falha se houver qualquer aviso)
make fuzz                # fuzzing com libFuzzer por 60 s (precisa do clang)
make fuzz FUZZ_TIME=600  # fuzzing por 10 min
make clean
```

- `examples/`: classes fornecidas na disciplina. O `cafebabe.class` não é um
  .class (é um executável do Windows) e deve ser rejeitado com erro.
- `tests/classes/` (fontes em `tests/java/`): classes extras com `wide`,
  exceções, lambdas e classes internas.
- O harness do libFuzzer (`fuzz/fuzz_classfile.c`) passa cada entrada gerada
  pelo parser e, se ela for aceita, por toda a exibição. As sementes são os
  `.class` de `examples/` e `tests/classes/`. Entradas que causam crash são
  salvas em `build/fuzz/`.

## Entrega

- `Leitor-Exibidor_SB_logo.pdf`: slides com exemplos das seções exibidas pelo leitor-exibidor.
- `docs/ATIVIDADES.md`: resumo das atividades de cada membro.
- `docs/ROTEIRO_VIDEO.md`: roteiro do vídeo de 5 minutos.

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
| `src/attributes.[ch]` | Decodificação dos atributos |
| `src/printer.[ch]` | Exibição das seções |
| `src/util.[ch]` | Indentação, títulos e dump hexadecimal |

Referência: JVMS SE 8, capítulos 4 e 6 (https://docs.oracle.com/javase/specs/jvms/se8/html/).

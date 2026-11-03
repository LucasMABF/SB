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
MinGW use `mingw32-make`. Testado com GCC 6.3 (MinGW) e GCC 16.1 (MinGW-w64).

## Execução

```sh
./leitor-exibidor examples/Belote.class            # todas as seções no terminal
./leitor-exibidor examples/Belote.class > out.txt  # saída em arquivo (redirecionamento)
./leitor-exibidor -o out.txt examples/Belote.class # saída em arquivo (opção -o)
./leitor-exibidor -s pool -s metodos examples/Belote.class  # só algumas seções
./leitor-exibidor -i examples/Belote.class         # menu interativo de seções
```

| Opção | Efeito |
|---|---|
| `-s <seção>` | Exibe só a seção indicada (pode repetir): `geral`, `pool`, `interfaces`, `fields`, `metodos`, `atributos` |
| `-o <arquivo>` | Grava a saída no arquivo em vez do terminal |
| `-i` | Modo interativo: menu para escolher a seção a exibir |
| `-h` | Ajuda |

Código de saída: 0 em caso de sucesso, 1 em caso de erro (arquivo inexistente,
magic number inválido, arquivo truncado/malformado...). Se o nome do arquivo não
bate com a classe declarada (`this_class`), um aviso é mostrado em stderr.

Outros alvos do Make:

```sh
make test   # roda em examples/ e tests/classes/, salva em out/ e mostra um resumo
make asan   # recompila com AddressSanitizer/UBSan (Linux; para depuração)
make clean
```

## O que é exibido

- **Informações gerais**: magic, versões (com o nome da versão do Java, ex.: `52 [1.8]`),
  access flags decodificadas, this/super class, contadores.
- **Pool de constantes**: todas as tags da JVMS (Utf8, Integer, Float, Long, Double,
  Class, String, Fieldref, Methodref, InterfaceMethodref, NameAndType, MethodHandle,
  MethodType, Dynamic, InvokeDynamic, Module, Package), cada referência com o índice
  e o valor resolvido (`cp #12 <java/lang/Object>`). Long/Double mostram high/low
  bytes e ocupam dois slots. Float/Double no formato do Java, incluindo `NaN`,
  `Infinity` e `-Infinity`. Strings Modified UTF-8 decodificadas (caractere nulo
  `C0 80`, pares de surrogates), com escape de `\n`, `\t` etc.
- **Interfaces, fields e métodos**: nome, descritor e access flags.
- **Bytecode**: todos os 202 opcodes da JVMS (mais os 3 reservados) com mnemônico e operandos. Operandos que
  apontam para o pool exibem a constante (`invokevirtual #4 <java/io/PrintStream.println : (I)V>`);
  desvios mostram o destino absoluto e o offset (`ifeq 12 (+8)`); `tableswitch` e
  `lookupswitch` (com o padding relativo ao início do código), `wide`, `iinc`,
  `newarray` (tipo), `multianewarray`, `invokeinterface`, `invokedynamic`.
- **Atributos**: Code (com tabela de exceções), ConstantValue, Exceptions,
  LineNumberTable, LocalVariableTable, LocalVariableTypeTable, SourceFile,
  InnerClasses, EnclosingMethod, Signature, Deprecated, Synthetic, StackMapTable
  (todos os tipos de frame), BootstrapMethods, MethodParameters, NestHost,
  NestMembers, PermittedSubclasses, Record, SourceDebugExtension,
  Runtime(In)VisibleAnnotations, Runtime(In)VisibleParameterAnnotations e
  AnnotationDefault.

### Não implementado

- Atributos `RuntimeVisibleTypeAnnotations`/`RuntimeInvisibleTypeAnnotations` e os
  de módulo (`Module`, `ModulePackages`, `ModuleMainClass`): aparecem como dump
  hexadecimal (não ocorrem nas classes de teste).
- Interface gráfica em árvore como a do jclasslib: a exibição é em texto.

## Testes

- `examples/`: classes fornecidas na disciplina (incluindo `cafebabe.class`, que
  não é um .class e deve gerar erro).
- `tests/java/` + `tests/classes/`: classes extras para cobrir o que os exemplos
  não cobrem (`wide`, NaN/Infinity, strings com escapes e emoji, lambdas,
  anotações, records, StackMapTable...). Para recompilar (JDK 17+):

  ```sh
  javac -encoding UTF-8 -g --release 8 -d tests/classes tests/java/Recursos.java tests/java/Largo.java
  javac -encoding UTF-8 -g -parameters --release 17 -d tests/classes tests/java/Moderno.java
  ```

A saída foi conferida com `javap -v -p` (JDK 21) para todas essas classes: mesmas
tags e valores no pool, mesmas instruções (pc, mnemônico, índice do pool) e mesmos
destinos de switch. O parser também foi testado com milhares de arquivos
truncados/corrompidos sem travar.

## Entrega

- `docs/ATIVIDADES.md`: resumo das atividades de cada membro.
- `docs/prints/`: prints da saída do leitor-exibidor (uma seção por imagem) usados nos slides.

## Estrutura

| Arquivo | Responsabilidade |
|---|---|
| `src/main.c` | Ponto de entrada e opções de linha de comando |
| `src/reader.[ch]` | Leitura big-endian com checagem de limites |
| `src/classfile.[ch]` | Estruturas do .class e parser |
| `src/constant_pool.[ch]` | Resolução e exibição do pool de constantes, Modified UTF-8, float/double |
| `src/access_flags.[ch]` | Decodificação das access flags |
| `src/opcodes.[ch]` | Tabela de opcodes (mnemônico + tipo de operando) |
| `src/bytecode.[ch]` | Disassembler do atributo Code |
| `src/attributes.[ch]` | Despacho de atributos, Code e SourceFile |
| `src/attr_decoders.c` | Demais atributos (LineNumberTable, StackMapTable, anotações, ...) |
| `src/printer.[ch]` | Exibição das seções |

Referência: JVMS SE 8, capítulos 4 e 6 (https://docs.oracle.com/javase/specs/jvms/se8/html/).

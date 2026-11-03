# Resumo das atividades por membro

Leitor-exibidor de arquivos .class — Software Básico (UnB).
Repositório: https://github.com/LucasMABF/SB

> Preencher os nomes e matrículas e ajustar a descrição ao que cada um fez de fato.

| Membro | Matrícula | Responsabilidade |
|---|---|---|
| Lucas | | Coordenação, esqueleto e parser |
| _(nome — Pessoa A)_ | | Pool de constantes |
| _(nome — Pessoa B)_ | | Bytecode |
| _(nome — Pessoa C)_ | | Atributos |
| _(nome — Pessoa D)_ | | Seções gerais, linha de comando e entrega |

## Lucas — coordenação, esqueleto e parser
- Arquitetura do projeto: separação em módulos (`reader`, `classfile`,
  `constant_pool`, `opcodes`/`bytecode`, `attributes`, `printer`).
- Leitor big-endian com checagem de limites (`reader.c`) e parser completo do
  formato .class (`classfile.c`), com mensagens de erro para arquivos inválidos
  (magic number errado, arquivo truncado, tag desconhecida).
- Makefile (`make`, `make test`, `make asan`) e script de testes
  (`scripts/run_examples.sh`).
- Decodificadores de Code e SourceFile e o despacho de atributos por nome.
- Integração, revisão e testes finais.

## Pessoa A — pool de constantes (`src/constant_pool.c`)
- Exibição detalhada de todas as tags do pool, no estilo do jclasslib, com o
  índice e o valor resolvido de cada referência.
- Long/Double com high/low bytes e o slot extra que eles ocupam.
- Float/Double no formato do Java, incluindo NaN, Infinity e -Infinity.
- Decodificação de Modified UTF-8 (caractere nulo `C0 80`, pares de surrogates)
  e escape de caracteres de controle.
- Resolução do bootstrap method nas entradas InvokeDynamic.

## Pessoa B — bytecode (`src/opcodes.c`, `src/bytecode.c`)
- Tabela com os 202 opcodes da JVMS (e os 3 reservados), cada um com o
  mnemônico e o formato dos operandos.
- Operandos que apontam para o pool exibem a constante referenciada.
- `tableswitch` e `lookupswitch` (padding relativo ao início do código) e `wide`.

## Pessoa C — atributos (`src/attr_decoders.c`)
- ConstantValue, Exceptions, LineNumberTable, LocalVariableTable,
  LocalVariableTypeTable, InnerClasses, Signature, EnclosingMethod,
  Deprecated, Synthetic.
- StackMapTable (todos os tipos de frame) e BootstrapMethods.
- Atributos de versões novas do Java: MethodParameters, NestHost, NestMembers,
  PermittedSubclasses, Record, anotações e AnnotationDefault.

## Pessoa D — seções gerais, linha de comando e entrega (`src/printer.c`, `src/main.c`)
- Nome da versão do Java a partir do major_version.
- Opções de linha de comando: seção específica (`-s`), saída em arquivo (`-o`)
  e menu interativo (`-i`).
- Classes de teste extras (`tests/java`) e comparação da saída com o `javap`.
- Slides com prints das seções e README.

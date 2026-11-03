# Resumo das atividades por membro

Leitor-exibidor de arquivos .class — Software Básico (UnB).
Repositório: https://github.com/LucasMABF/SB

| Membro | Matrícula | Responsabilidade |
|---|---|---|
| Lucas Mendes | 241020750 | Coordenação, esqueleto, parser, seções gerais, analisadores e entrega |
| Antonio Coelho | 241038100 | Pool de constantes |
| Gabriel Pinto | 241002331 | Bytecode |
| Leonardo Mileo | 241032690 | Atributos |

## Lucas Mendes — coordenação, parser, analisadores e entrega

Coordenação, esqueleto e parser:
- Arquitetura do projeto: separação em módulos (`reader`, `classfile`,
  `constant_pool`, `opcodes`/`bytecode`, `attributes`, `printer`, `util`).
- Leitor big-endian com checagem de limites (`reader.c`) e parser completo do
  formato .class (`classfile.c`), com mensagens de erro para arquivos inválidos
  (magic number errado, arquivo truncado, tag desconhecida).
- Makefile (`make`, `make test`, `make asan`, `make cppcheck`, `make fuzz`),
  script de testes (`scripts/run_examples.sh`) e harness do libFuzzer
  (`fuzz/fuzz_classfile.c`).
- Integração, simplificação do código, revisão e testes finais.

Seções gerais e entrega (`src/printer.c`, `src/main.c`):
- Exibição das seções e nome da versão do Java a partir do major_version.
- Classes de teste extras (`tests/java`).
- Slides e README.

## Antonio — pool de constantes (`src/constant_pool.c`)
- Exibição de todas as entradas do pool, no estilo do jclasslib, com o índice
  e o valor resolvido de cada referência.
- Integer, Float, Long e Double com os bytes e o valor; Long/Double ocupam
  dois slots.
- Escape de `\n`, `\t`, `\r` e do caractere nulo nas strings.

## Gabriel — bytecode (`src/opcodes.c`, `src/bytecode.c`)
- Tabela com os 202 opcodes da JVMS (e os 3 reservados), cada um com o
  mnemônico e o formato dos operandos.
- Operandos que apontam para o pool exibem a constante referenciada.
- `tableswitch` e `lookupswitch` (padding relativo ao início do código) e `wide`.

## Leonardo — atributos (`src/attributes.c`)
- Code (com a tabela de exceções), SourceFile, ConstantValue, Signature,
  Exceptions, LineNumberTable e LocalVariableTable.
- Dump hexadecimal, com nome e tamanho, dos atributos não decodificados.

# Resumo das atividades por membro

Leitor-exibidor de arquivos .class — Software Básico (UnB).
Repositório: https://github.com/LucasMABF/SB

| Membro | Matrícula | Responsabilidade |
|---|---|---|
| Lucas Mendes | 241020750 | Coordenação, leitura binária e parser |
| Antonio Coelho | 241038100 | Pool de constantes e access flags |
| Gabriel Pinto | 241002331 | Bytecode |
| Leonardo Mileo | 241032690 | Atributos e dump hexadecimal |
| Daniel Morais | 242042396 | Seções da saída e ponto de entrada |

## Lucas Mendes — coordenação, leitura binária e parser (`src/reader.c`, `src/classfile.c`)
- Arquitetura do projeto: separação em módulos (`reader`, `classfile`,
  `constant_pool`, `opcodes`/`bytecode`, `attributes`, `printer`, `util`).
- Leitor big-endian com checagem de limites (`reader.c`) e parser completo do
  formato .class (`classfile.c`), com mensagens de erro para arquivos inválidos
  (magic number errado, arquivo truncado, tag desconhecida).
- Makefile (`make`, `make test`, `make asan`, `make cppcheck`, `make fuzz`),
  script de testes (`scripts/run_examples.sh`), harness do libFuzzer
  (`fuzz/fuzz_classfile.c`) e classes de teste extras (`tests/java`).
- Integração, simplificação do código, revisão e testes finais.

## Antonio Coelho — pool de constantes e access flags (`src/constant_pool.c`, `src/access_flags.c`)
- Exibição de todas as entradas do pool, no estilo do jclasslib, com o índice
  e o valor resolvido de cada referência (ex.: Methodref → Class + NameAndType
  → Utf8).
- Integer, Float, Long e Double com os bytes e o valor; Float/Double montados a
  partir dos bits (IEEE 754); Long/Double ocupam dois slots.
- Escape de `\n`, `\t`, `\r` e do caractere nulo (`C0 80`) nas strings.
- Decodificação das access flags por contexto (classe, field, método, classe
  interna).

## Gabriel Pinto — bytecode (`src/opcodes.c`, `src/bytecode.c`)
- Tabela com os 202 opcodes da JVMS (e os 3 reservados), cada um com o
  mnemônico e o formato dos operandos.
- Operandos que apontam para o pool exibem a constante referenciada.
- `tableswitch` e `lookupswitch` (padding relativo ao início do código) e `wide`.

## Leonardo Mileo — atributos e dump hexadecimal (`src/attributes.c`, `src/util.c`)
- Code (com a tabela de exceções), SourceFile, ConstantValue, Signature,
  Exceptions, LineNumberTable e LocalVariableTable.
- Despacho dos atributos pelo nome e dump hexadecimal, com nome e tamanho, dos
  atributos não decodificados.

## Daniel Morais — seções da saída e ponto de entrada (`src/printer.c`, `src/main.c`)
- Exibição das seções: informações gerais, pool, interfaces, fields, métodos e
  atributos da classe.
- Nome da versão do Java a partir do major_version.
- Ponto de entrada: checagem dos argumentos, mensagem de erro e código de saída.
- Slides com exemplos das seções.

# Divisão de tarefas

O leitor-exibidor está **completo**: `make test` roda sem pendências em todos os
exemplos de `examples/` e nas classes extras de `tests/classes/`, e a saída foi
conferida com `javap -v -p`. Falta apenas o vídeo.

Referência: JVMS SE 8, capítulos 4 (formato .class) e 6/7 (instruções):
https://docs.oracle.com/javase/specs/jvms/se8/html/
Para comparar a saída: jclasslib (https://github.com/ingokegel/jclasslib) ou
`javap -v -p Arquivo.class` (vem com o JDK).

## Pessoa A: pool de constantes (`src/constant_pool.c`)
- [x] `print_cp_entry`: exibição detalhada de String, Integer, Float, Long,
      Double, NameAndType, MethodHandle, MethodType, InvokeDynamic, Module, Package
- [x] NaN / Infinity em float e double (formato do `Float/Double.toString`)
- [x] Modified UTF-8 (`C0 80`, surrogates) e escape de `\n`, `\t`
- [x] InvokeDynamic mostra o bootstrap method resolvido via BootstrapMethods

## Pessoa B: bytecode (`src/opcodes.c`, `src/bytecode.c`)
- [x] Tabela `OPCODES` completa (202 opcodes + 3 reservados)
- [x] `op_tableswitch` (padding relativo ao início do código)
- [x] `op_lookupswitch`
- [x] `op_wide` (testado com `tests/classes/Largo.class`)

## Pessoa C: atributos (`src/attr_decoders.c`)
- [x] LineNumberTable
- [x] LocalVariableTable / LocalVariableTypeTable
- [x] ConstantValue
- [x] Exceptions, InnerClasses, Signature, EnclosingMethod
- [x] StackMapTable, BootstrapMethods
- [x] Extras: MethodParameters, NestHost, NestMembers, PermittedSubclasses,
      Record, SourceDebugExtension, anotações e AnnotationDefault

## Pessoa D: seções gerais + entrega (`src/printer.c`, `src/main.c`, slides)
- [x] `java_version_name` (ex.: 52 → "1.8")
- [x] Opções `-s` (seção), `-o` (arquivo de saída) e `-i` (menu interativo)
- [x] Slides com prints das seções (General, Constant Pool, Fields,
      Methods/Bytecode, Attributes) e do erro do `cafebabe.class`
- [x] Resumo das atividades de cada membro (`docs/ATIVIDADES.md`, falta preencher os nomes)

## Coordenação (Lucas)
- [x] Esqueleto, parser, Makefile, script de testes
- [x] Integrar as contribuições; `make test` sem pendências; saída conferida com `javap`
- [ ] `make asan` em Linux (no Windows/MinGW o AddressSanitizer não está disponível;
      no lugar disso foi feito um teste com arquivos truncados/corrompidos)
- [ ] Roteiro e gravação do vídeo (5 min): o que foi feito, o que não foi, como compilar/rodar
- [ ] Empacotar a entrega (fontes + Makefile + README, vídeo, slides, resumo)

## Regras rápidas
- Cada pessoa mexe principalmente no(s) seu(s) arquivo(s), para evitar conflitos de merge.
- Antes de subir: `make` sem warnings e `make test` sem regressões.
- Commits pequenos e frequentes; um branch por pessoa e merge na `main`.

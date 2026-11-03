# Divisão de tarefas

O esqueleto já **compila e roda em todos os exemplos**. O parser do `.class`
está completo; o que falta está marcado com `TODO(Pessoa X)` no código. Toda
parte não implementada tem um *fallback* (dump hexadecimal ou valor genérico),
então ninguém bloqueia ninguém: cada um trabalha no seu arquivo em paralelo.

Antes de começar: `make test`. A coluna PENDÊNCIAS conta quantos trechos
ainda aparecem como "não implementado/não mapeado". A meta é zerar.

Referência: JVMS SE 8, capítulos 4 (formato .class) e 6/7 (instruções):
https://docs.oracle.com/javase/specs/jvms/se8/html/
Para comparar a saída: jclasslib (https://github.com/ingokegel/jclasslib) ou
`javap -v -p Arquivo.class` (vem com o JDK).

## Pessoa A: pool de constantes (`src/constant_pool.c`)
- [ ] `print_cp_entry`: exibição detalhada de String, Integer, Float, Long,
      Double, NameAndType, MethodHandle, MethodType, InvokeDynamic (ver TODO)
- [ ] NaN / Infinity em float e double
- [ ] `print_utf8_value`: Modified UTF-8 e escape de `\n`, `\t`
- Testar com: `Somar` (Double), `double_*`, `Belote`

## Pessoa B: bytecode (`src/opcodes.c`, `src/bytecode.c`)
- [ ] Completar a tabela `OPCODES` (stores, stack, math, conversões,
      comparações, controle, extended): a maior parte das pendências vem daqui
- [ ] `op_tableswitch` (testar com `tableswitch`)
- [ ] `op_lookupswitch` (testar com `lookupswitch`)
- [ ] `op_wide`
- Cuidado com o padding dos switches: é relativo ao início do código do método.

## Pessoa C: atributos (`src/attr_decoders.c`)
- [ ] LineNumberTable (aparece em todos os métodos)
- [ ] LocalVariableTable / LocalVariableTypeTable
- [ ] ConstantValue (testar com `Somar`)
- [ ] Exceptions, InnerClasses, Signature, EnclosingMethod
- [ ] (opcional) StackMapTable, BootstrapMethods

## Pessoa D: seções gerais + entrega (`src/printer.c`, slides)
- [ ] `java_version_name` (ex.: 52 → "1.8")
- [ ] Slides: um print por seção (General, Constant Pool, Fields, Methods/Bytecode,
      Attributes) para pelo menos 2 ou 3 classes de exemplo, incluindo o erro do
      `cafebabe.class`
- [ ] Resumo das atividades de cada membro

## Coordenação (Lucas)
- [x] Esqueleto, parser, Makefile, script de testes
- [ ] Revisar e integrar as contribuições; rodar `make asan` + `make test` no final
- [ ] Roteiro e gravação do vídeo (5 min): o que foi feito, o que não foi, como compilar/rodar
- [ ] Empacotar a entrega (fontes + Makefile + README, vídeo, slides, resumo)

## Regras rápidas
- Cada pessoa mexe principalmente no(s) seu(s) arquivo(s), para evitar conflitos de merge.
- Antes de subir: `make` sem warnings e `make test` sem regressões.
- Commits pequenos e frequentes; um branch por pessoa e merge na `main`.

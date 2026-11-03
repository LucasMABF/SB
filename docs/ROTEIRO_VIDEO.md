# Roteiro do vídeo (5 minutos)

Leitor-exibidor de arquivos .class (Software Básico, UnB).

O vídeo tem que cobrir três pontos: **o que foi implementado**, **o que não foi
implementado** e **como compilar e rodar**. Cada membro apresenta a parte que
fez. O Lucas Mendes, que fez a coordenação, o parser e a linha de comando, fica
com a abertura, a compilação e os testes; o encerramento fica com o Antonio,
para equilibrar o tempo de fala.
Os tempos somam 5:00. Ensaiem com cronômetro: é fácil passar do tempo nas
partes de demonstração.

Antes de gravar:
- Terminal com fonte grande (≥ 16 pt) e janela larga (~120 colunas), para as
  linhas do bytecode não quebrarem.
- Começar com `make clean`, para a compilação aparecer do zero.
- Deixar os comandos abaixo prontos para copiar e colar, sem digitar ao vivo.
- No Windows, trocar `make` por `mingw32-make` e `./leitor-exibidor` por
  `.\leitor-exibidor.exe`.

| Tempo | Bloco | Quem |
|---|---|---|
| 0:00–0:30 | Abertura | Lucas Mendes |
| 0:30–1:10 | Compilar e rodar | Lucas Mendes |
| 1:10–1:50 | Informações gerais e pool de constantes | Antonio |
| 1:50–2:50 | Métodos e bytecode | Gabriel |
| 2:50–3:40 | Atributos | Leonardo |
| 3:40–4:20 | Robustez e testes | Lucas Mendes |
| 4:20–5:00 | O que não foi implementado e encerramento | Antonio |

---

## 0:00–0:30 — Abertura (Lucas Mendes)

**Tela:** README aberto no editor, ou slide de título com os nomes.

> "Olá, somos o grupo [...]: Lucas Mendes, Antonio
> Coelho, Gabriel Pinto e Leonardo Mileo.
> Implementamos em C99 um leitor-exibidor de arquivos .class da JVM. Ele lê o
> arquivo binário e mostra as estruturas no estilo do jclasslib: informações
> gerais, pool de constantes, interfaces, fields, métodos com o bytecode
> desmontado, e os atributos. Vamos mostrar como compilar, o que foi feito e o
> que ficou de fora."

## 0:30–1:10 — Compilar e rodar (Lucas Mendes)

**Tela:** terminal.

```sh
make clean
make
```

> "A única dependência é o GCC. Compilamos com `-std=c99 -Wall -Wextra
> -pedantic -O2 -g`, sem nenhum warning. Sem o Make, basta um comando:"

```sh
gcc -std=c99 -Wall -Wextra -pedantic -O2 -g src/*.c -o leitor-exibidor
```

> "Para rodar, passamos o .class. Sem opções, ele mostra todas as seções. Com
> `-s` mostra só uma seção, com `-o` grava em arquivo e com `-i` abre um menu
> interativo."

```sh
./leitor-exibidor -h
./leitor-exibidor -i examples/Belote.class
```

**Mostrar:** o menu do `-i` e uma escolha de seção (ver `docs/prints/16_menu_interativo.png`).

## 1:10–1:50 — Informações gerais e pool de constantes (Antonio)

```sh
./leitor-exibidor -s geral -s pool examples/Somar.class
```

> "Nas informações gerais aparecem o magic number, a versão (52 é o Java 8) e as
> access flags decodificadas. No pool, cada entrada tem o índice e o tipo, e toda
> referência mostra o índice e o valor resolvido: a classe `Somar` aponta para
> `cp #12`, que é o Utf8 `Somar`. Double e Long mostram high bytes e low bytes e
> ocupam dois slots: aqui a entrada 6 é o double 3.1415, e a 7 é o slot extra."

**Se sobrar tempo** (5 s): mostrar `docs/prints/10_pool_constantes_especiais.png`
(NaN, Infinity) e `11_pool_strings_utf8.png` (Modified UTF-8).

## 1:50–2:50 — Métodos e bytecode (Gabriel)

```sh
./leitor-exibidor -s metodos examples/Belote.class
```

**Rolar até o método `main`.**

> "Cada instrução aparece com o pc, o mnemônico e os operandos. Os operandos que
> apontam para o pool mostram a constante: `invokespecial #44` mostra
> `Belote.<init> : ()V`, e `anewarray #36` mostra a classe `Carta`. Os desvios
> mostram o destino absoluto e o deslocamento relativo, como `if_icmpge 48 (+25)`.
> A tabela cobre os 202 opcodes da especificação."

```sh
./leitor-exibidor -s metodos examples/tableswitch.class
```

> "O tableswitch e o lookupswitch têm tamanho variável e padding de alinhamento
> a 4 bytes, contado a partir do início do código. Mostramos cada caso com o
> destino. O `wide` também está implementado."

## 2:50–3:40 — Atributos (Leonardo)

Os exemplos da disciplina têm poucos atributos (o Belote só tem Code,
LineNumberTable e SourceFile). Por isso esta parte usa a nossa classe extra
`Recursos`, que tem exceções, lambdas e classes internas.

```sh
./leitor-exibidor -s metodos tests/classes/Recursos.class
```

> "Dentro de cada método, o atributo Code traz max stack, max locals, a tabela
> de exceções e os atributos internos: LineNumberTable, LocalVariableTable e
> StackMapTable, com todos os tipos de frame."

```sh
./leitor-exibidor -s atributos tests/classes/Recursos.class
```

> "Na classe aparecem Signature, SourceFile, InnerClasses e BootstrapMethods,
> que é o atributo usado pelas lambdas. Também tratamos atributos de versões
> mais novas do Java, como Record, NestMembers e anotações."

**Mostrar:** `docs/prints/12_excecoes_stackmap.png` e `15_bootstrap_methods.png`.

## 3:40–4:20 — Robustez e testes (Lucas Mendes)

```sh
./leitor-exibidor examples/cafebabe.class
make test
```

> "O `cafebabe.class` não é um .class de verdade: o magic number está errado. O
> programa avisa e sai com código 1, sem travar. Arquivos truncados também geram
> mensagem de erro. O `make test` roda todos os exemplos da disciplina e as
> nossas classes extras e mostra que não há pendências. Conferimos a saída com o
> `javap -v` do JDK: os mesmos valores no pool, as mesmas instruções e os mesmos
> destinos de switch."

## 4:20–5:00 — O que não foi implementado e encerramento (Antonio)

**Tela:** seção "Não implementado" do README.

> "O que não implementamos: os atributos RuntimeVisibleTypeAnnotations e
> RuntimeInvisibleTypeAnnotations e os atributos de módulo (Module,
> ModulePackages, ModuleMainClass). Se aparecerem, o programa mostra o conteúdo
> em hexadecimal, sem quebrar. Eles não ocorrem nas classes de teste. Também não
> fizemos interface gráfica em árvore como a do jclasslib: a saída é em texto,
> com o menu interativo como alternativa. Os prints de cada seção estão nos
> slides e a divisão de tarefas está no resumo de atividades. Obrigado!"

---

## Checklist de gravação
- [ ] Nome do grupo e dos membros na abertura
- [ ] Mostrou a compilação **e** as opções de compilação
- [ ] Mostrou um operando do bytecode resolvido no pool (ex.: `invokevirtual #N <...>`)
- [ ] Mostrou o pool com índices e valores resolvidos
- [ ] Mencionou explicitamente o que **não** foi implementado
- [ ] Duração total ≤ 5:00

# Roteiro do vídeo (5 minutos)

Leitor-exibidor de arquivos .class (Software Básico, UnB).

O vídeo tem que cobrir três pontos: **o que foi implementado**, **o que não foi
implementado** e **como compilar e rodar**. Cada membro apresenta a parte que
fez, na ordem dos slides (`Leitor-Exibidor_SB_logo.pdf`). Os tempos somam 5:00.
Ensaiem com cronômetro: é fácil passar do tempo nas partes de demonstração.

Antes de gravar:
- Terminal com fonte grande (≥ 16 pt) e janela larga (~120 colunas), para as
  linhas do bytecode não quebrarem.
- Começar com `make clean`, para a compilação aparecer do zero.
- O programa sempre mostra todas as seções. Para as demonstrações, gerar a
  saída em arquivo antes de gravar e abrir no editor, que permite buscar o
  trecho (`Ctrl+F`) em vez de rolar o terminal. `make test` gera todas as
  saídas em `out/`.
- No Windows, trocar `make` por `mingw32-make` e `./leitor-exibidor` por
  `.\leitor-exibidor.exe`.

| Tempo | Bloco | Slides | Quem |
|---|---|---|---|
| 0:00–0:25 | Abertura | 1 | Lucas Mendes |
| 0:25–0:55 | Compilar e rodar | 2 | Lucas Mendes |
| 0:55–1:35 | Organização do código, seções da saída e ponto de entrada | 3, 4, 6 | Daniel Morais |
| 1:35–2:25 | Pool de constantes e access flags | 5, 13 | Antonio Coelho |
| 2:25–3:25 | Bytecode | 7, 8, 9, 15 | Gabriel Pinto |
| 3:25–4:05 | Atributos e dump hexadecimal | 10, 14 | Leonardo Mileo |
| 4:05–4:35 | Parser e robustez | 11, 12 | Lucas Mendes |
| 4:35–5:00 | O que não foi implementado e encerramento | 16, 17 | Daniel Morais |

---

## 0:00–0:25 — Abertura (Lucas Mendes) · slide 1

> "Olá, somos o grupo [...]: Antonio Coelho, Daniel Morais, Gabriel Pinto,
> Leonardo Mileo e Lucas Mendes. Implementamos em C99 um leitor-exibidor de
> arquivos .class da JVM: ele lê o arquivo binário e mostra as estruturas no
> estilo do jclasslib, com o bytecode desmontado e os valores do pool de
> constantes resolvidos."

## 0:25–0:55 — Compilar e rodar (Lucas Mendes) · slide 2

```sh
make clean
make
./leitor-exibidor examples/Somar.class
```

> "A única dependência é o GCC. Compilamos com `-std=c99 -Wall -Wextra
> -pedantic -O2 -g`, sem nenhum warning; sem o Make, é um único comando `gcc`.
> Para rodar, passamos o .class como único argumento; a saída pode ser
> redirecionada para um arquivo."

## 0:55–1:35 — Organização, seções e ponto de entrada (Daniel Morais) · slides 3, 4 e 6

**Tela:** slide 3, depois a saída do `Somar.class` no terminal (é curta).

> "O código é dividido em módulos: leitura binária, parser, pool, bytecode,
> atributos e exibição. O `main` confere o argumento, chama o parser e, se der
> certo, imprime as seções; em caso de erro, mostra a mensagem e sai com código
> 1. As informações gerais mostram o magic number, a versão — 52 é o Java 8 — e
> this e super class com o índice no pool. Depois vêm as interfaces e os
> fields, cada um com nome, descritor e os seus atributos, como o
> ConstantValue do `pi`."

## 1:35–2:25 — Pool de constantes e access flags (Antonio Coelho) · slides 5 e 13

**Tela:** a seção `CONSTANT POOL` do `Somar.class`.

> "O pool de constantes é uma tabela numerada com todas as constantes da
> classe; o resto do arquivo aponta para ela pelo índice, que começa em 1. Cada
> entrada começa com uma tag que diz o tipo. As referências são resolvidas
> seguindo a cadeia: a classe #1 aponta para o Utf8 #12, que é `Somar`; um
> Methodref aponta para uma Class e um NameAndType, que apontam para Utf8.
> Double e Long ocupam dois slots: o 6 é o double 3.1415 e o 7 é o slot extra."

**Tela:** slide 13.

> "Float e Double vêm como os bits do IEEE 754, e reinterpretamos esses bits
> com `memcpy`, incluindo NaN e infinito. As access flags são bits: `0x0019` é
> public, static e final. O mesmo bit muda de significado conforme o contexto,
> por isso temos uma tabela para classe, field e método."

## 2:25–3:25 — Bytecode (Gabriel Pinto) · slides 7, 8, 9 e 15

**Tela:** `out/Belote.txt` no editor, buscando `[4] main`.

> "Cada instrução aparece com o pc, o mnemônico e os operandos. Os operandos que
> apontam para o pool mostram a constante: `invokespecial #44` mostra
> `Belote.<init> : ()V`. Os desvios mostram o destino absoluto e o
> deslocamento, como `if_icmpge 48 (+25)` e `goto 20 (-25)`. Uma tabela com os
> 202 opcodes diz o tamanho e o formato de cada instrução. O tableswitch e o
> lookupswitch têm padding até um múltiplo de 4, contado a partir do início do
> código, e o `wide` estende os índices para 16 bits, como no método com 300
> variáveis locais."

## 3:25–4:05 — Atributos e dump hexadecimal (Leonardo Mileo) · slides 10 e 14

**Tela:** `out/Recursos.txt`, buscando `[4] tentar` e depois `CLASS ATTRIBUTES`.

> "Os atributos são identificados pelo nome, que está no pool. Decodificamos
> Code, com a tabela de exceções, SourceFile, ConstantValue, Signature,
> Exceptions, LineNumberTable e LocalVariableTable. Os demais, como
> BootstrapMethods e InnerClasses, aparecem com o nome, o tamanho e um dump
> hexadecimal; como cada atributo informa o próprio tamanho, um atributo
> desconhecido nunca quebra a leitura."

## 4:05–4:35 — Parser e robustez (Lucas Mendes) · slides 11 e 12

```sh
./leitor-exibidor examples/cafebabe.class
make test
```

> "O parser lê o arquivo em ordem, em big-endian, conferindo os limites a cada
> leitura. O `cafebabe.class` começa com MZ, de um executável do Windows: o
> programa rejeita o magic number e sai com código 1. Arquivos truncados também
> geram uma mensagem de erro. O `make test` roda todos os exemplos e as classes
> extras, e temos `make asan`, `make cppcheck` e `make fuzz` para procurar erros
> de memória e de código."

## 4:35–5:00 — Não implementado e encerramento (Daniel Morais) · slides 16 e 17

> "Não implementamos a decodificação dos demais atributos, como StackMapTable,
> InnerClasses e BootstrapMethods, que aparecem em hexadecimal; a conversão de
> emoji no Modified UTF-8; e uma interface gráfica em árvore como a do
> jclasslib: a saída é em texto. A divisão de tarefas está no último slide e no
> resumo de atividades. Obrigado!"

---

## Checklist de gravação
- [ ] Nome do grupo e dos cinco membros na abertura
- [ ] Mostrou a compilação **e** as opções de compilação
- [ ] Mostrou um operando do bytecode resolvido no pool (ex.: `invokevirtual #N <...>`)
- [ ] Mostrou o pool com índices e valores resolvidos
- [ ] Mencionou explicitamente o que **não** foi implementado
- [ ] Duração total ≤ 5:00

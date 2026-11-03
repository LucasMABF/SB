# Roteiro do vídeo (5 minutos)

Leitor-exibidor de arquivos .class (Software Básico, UnB).

O vídeo tem que cobrir três pontos: **o que foi implementado**, **o que não foi
implementado** e **como compilar e rodar**. Cada membro apresenta a parte que
fez. O Lucas Mendes, que fez a coordenação, o parser e os analisadores, fica
com a abertura, a compilação e os testes; o encerramento fica com o Antonio,
para equilibrar o tempo de fala.
Os tempos somam 5:00. Ensaiem com cronômetro: é fácil passar do tempo nas
partes de demonstração.

Antes de gravar:
- Terminal com fonte grande (≥ 16 pt) e janela larga (~120 colunas), para as
  linhas do bytecode não quebrarem.
- Começar com `make clean`, para a compilação aparecer do zero.
- O programa sempre mostra todas as seções. Para as demonstrações, gerar a
  saída em arquivo antes de gravar e abrir no editor, que permite buscar o
  trecho (`Ctrl+F`) em vez de rolar o terminal:

  ```sh
  ./leitor-exibidor examples/Somar.class > out/Somar.txt
  ./leitor-exibidor examples/Belote.class > out/Belote.txt
  ./leitor-exibidor examples/tableswitch.class > out/tableswitch.txt
  ./leitor-exibidor tests/classes/Recursos.class > out/Recursos.txt
  ```

  (`make test` já gera todas essas saídas em `out/`.)
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

**Tela:** slide de título com os nomes.

> "Olá, somos o grupo [...]: Lucas Mendes, Antonio Coelho, Gabriel Pinto e
> Leonardo Mileo. Implementamos em C99 um leitor-exibidor de arquivos .class
> da JVM. Ele lê o arquivo binário e mostra as estruturas no estilo do
> jclasslib: informações gerais, pool de constantes, interfaces, fields,
> métodos com o bytecode desmontado, e os atributos. Vamos mostrar como
> compilar, o que foi feito e o que ficou de fora."

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

> "Para rodar, passamos o .class como único argumento. O programa mostra todas
> as seções, e a saída pode ser redirecionada para um arquivo."

```sh
./leitor-exibidor examples/Somar.class
./leitor-exibidor examples/Belote.class > out/Belote.txt
```

## 1:10–1:50 — Informações gerais e pool de constantes (Antonio)

**Tela:** a saída do `Somar.class` no terminal (é curta, cabe na tela).

> "Nas informações gerais aparecem o magic number, a versão (52 é o Java 8) e as
> access flags decodificadas. No pool, cada entrada tem o índice e o tipo, e toda
> referência mostra o índice e o valor resolvido: a classe `Somar` aponta para
> `cp #12`, que é o Utf8 `Somar`. Double e Long mostram high bytes e low bytes e
> ocupam dois slots: aqui a entrada 6 é o double 3.1415, e a 7 é o slot extra."

## 1:50–2:50 — Métodos e bytecode (Gabriel)

**Tela:** `out/Belote.txt` no editor, buscando `[4] main`.

> "Cada instrução aparece com o pc, o mnemônico e os operandos. Os operandos que
> apontam para o pool mostram a constante: `invokespecial #44` mostra
> `Belote.<init> : ()V`, e `anewarray #36` mostra a classe `Carta`. Os desvios
> mostram o destino absoluto e o deslocamento relativo, como `if_icmpge 48 (+25)`
> e `goto 20 (-25)`. A tabela cobre os 202 opcodes da especificação."

**Tela:** `out/tableswitch.txt`, buscando `tableswitch`.

> "O tableswitch e o lookupswitch têm tamanho variável e padding de alinhamento
> a 4 bytes, contado a partir do início do código. Mostramos cada caso com o
> destino. O `wide` também está implementado."

## 2:50–3:40 — Atributos (Leonardo)

Os exemplos da disciplina têm poucos atributos (o Belote só tem Code,
LineNumberTable e SourceFile). Por isso esta parte usa a nossa classe extra
`Recursos`, que tem exceções, lambdas e classes internas.

**Tela:** `out/Recursos.txt`, buscando `[4] tentar`.

> "Dentro de cada método, o atributo Code traz max stack, max locals, a tabela
> de exceções, com o tipo capturado resolvido no pool, e os atributos internos:
> LineNumberTable e LocalVariableTable."

**Tela:** o fim de `out/Recursos.txt` (seção `CLASS ATTRIBUTES`).

> "Na classe aparecem Signature e SourceFile, decodificados. Decodificamos
> Code, SourceFile, ConstantValue, Signature, Exceptions, LineNumberTable e
> LocalVariableTable. Os demais, como BootstrapMethods e InnerClasses aqui, são
> mostrados com o nome, o tamanho e um dump hexadecimal do conteúdo."

## 3:40–4:20 — Robustez e testes (Lucas Mendes)

```sh
./leitor-exibidor examples/cafebabe.class
make test
```

> "O `cafebabe.class` não é um .class de verdade: o magic number está errado. O
> programa avisa e sai com código 1, sem travar. Arquivos truncados também geram
> mensagem de erro. O `make test` roda todos os exemplos da disciplina e as
> nossas classes extras. Também temos `make asan`, `make cppcheck` e `make fuzz`,
> com AddressSanitizer, análise estática e fuzzing com libFuzzer."

## 4:20–5:00 — O que não foi implementado e encerramento (Antonio)

**Tela:** slide "O que foi e o que não foi implementado".

> "O que não implementamos: os atributos além dos sete que citamos, como
> StackMapTable, InnerClasses, BootstrapMethods e anotações, não são
> decodificados; aparecem em hexadecimal, sem quebrar o programa. Caracteres
> fora do BMP, como emoji, não são convertidos do Modified UTF-8. E não fizemos
> interface gráfica em árvore como a do jclasslib: a saída é em texto. Os
> exemplos de cada seção estão nos slides e a divisão de tarefas está no resumo
> de atividades. Obrigado!"

---

## Checklist de gravação
- [ ] Nome do grupo e dos membros na abertura
- [ ] Mostrou a compilação **e** as opções de compilação
- [ ] Mostrou um operando do bytecode resolvido no pool (ex.: `invokevirtual #N <...>`)
- [ ] Mostrou o pool com índices e valores resolvidos
- [ ] Mencionou explicitamente o que **não** foi implementado
- [ ] Duração total ≤ 5:00

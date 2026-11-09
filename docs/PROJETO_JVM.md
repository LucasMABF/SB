# Projeto lógico da JVM

Base para os slides da entrega "Apresentação do projeto lógico da JVM"
(Software Básico, UnB). Cada seção `## Slide` corresponde a um slide.

## O que esta entrega exige

Slides, com um envio por grupo e 5 minutos de apresentação, contendo:

- O que foi implementado (slide 2)
- O que será implementado, como e com quais estruturas de dados (slides 3 a 9)
- a) Como o bytecode seleciona o código da instrução (slide 4)
- b) Gerência de memória dinâmica (slide 5)
- c) Organização e gestão da área de métodos (slide 6)
- d) Organização e gestão de arrays, incluindo `arraylength` (slide 7)
- e) Organização e gestão de objetos (slide 8)
- f) Estrutura e gestão da pilha de frames (slide 9)
- O que não será implementado (slide 10)
- Dúvidas do grupo (slide 10)
- Resumo das atividades de cada membro (slide 11)

Tempo sugerido:

| Slides | Assunto | Tempo |
|---|---|---|
| 1-2 | Abertura e o que já existe | 0:30 |
| 3 | Visão geral e `Slot` | 0:30 |
| 4-9 | Itens a) a f), cerca de 35 s cada | 3:30 |
| 10 | Fora do escopo e dúvidas | 0:20 |
| 11 | Atividades e divisão | 0:10 |

---

## Slide 1: Projeto lógico da JVM

Software Básico, UnB. C99 e GCC.

Antonio Coelho · Daniel Morais · Gabriel Pinto · Leonardo Mileo · Lucas Mendes

---

## Slide 2: O que já foi implementado

Leitor-exibidor de arquivos `.class`, que a JVM vai reaproveitar:

- Parser completo do `.class`, com leitura big-endian e checagem de limites
- Pool de constantes, com índices e valores resolvidos
- Tabela dos 202 opcodes com mnemônico e formato dos operandos
- Atributos decodificados: Code, SourceFile, ConstantValue, Signature,
  Exceptions, LineNumberTable e LocalVariableTable
- `cppcheck`, AddressSanitizer/UBSan e libFuzzer

O programa passa a ter dois modos, sobre o mesmo parser:

```sh
./jvm view X.class   # exibidor
./jvm run  X.class   # execução
```

---

## Slide 3: Visão geral e o `Slot`

```
Área de métodos        Heap                      Pilha de frames
vetor de Class    ←──  objetos e arrays     ←──  frames, com locais e operandos
```

O **slot** é cada posição das variáveis locais e da pilha de operandos:

```c
typedef union {
    int32_t i;
    float   f;
    int64_t l;
    double  d;
    void   *ref;
} Slot;
```

- A instrução define o campo usado: `iadd` lê `.i` e `dadd` lê `.d`.
- A referência é um ponteiro C direto, que cabe no slot de 8 bytes.
- `long` e `double` ficam no primeiro de **dois** índices, e o segundo fica
  vazio. Assim a numeração das locais e instruções como `dup2` seguem a JVMS.
- Custo: mais memória nos slots. Os dados de arrays usam o tamanho real de
  cada elemento.

---

## Slide 4: a) Do bytecode ao código da instrução

O byte do opcode é o **índice de uma tabela**, que guarda a função que executa
a instrução. A tabela do exibidor ganha um campo:

```c
typedef void (*Handler)(VM *vm, Frame *f, u1 op);

typedef struct {
    const char *mnemonic;
    OperandKind kind;
    Handler     handler;
} OpcodeInfo;

[0x60] = {"iadd",    OPK_NONE, op_iadd},
[0x1a] = {"iload_0", OPK_NONE, op_iload_n},
[0x1b] = {"iload_1", OPK_NONE, op_iload_n},
```

Laço principal:

```c
while (vm->frame) {
    Frame *f = vm->frame;
    u1 op = f->method->code[f->pc];
    Handler h = OPCODES[op].handler;
    if (!h) erro_opcode_invalido(op);
    h(vm, f, op);
}
```

- Cada opcode é definido em um único lugar, para o exibidor e para a execução.
- Instruções da mesma família compartilham o handler:

```c
void op_iload_n(VM *vm, Frame *f, u1 op)
{
    push(f, f->locals[op - 0x1a]);   /* 0x1a é o iload_0 */
    f->pc += 1;
}
```

---

## Slide 5: b) Gerência de memória dinâmica

Objetos e arrays são criados com `malloc`/`calloc` e começam com o mesmo
cabeçalho:

```c
typedef struct ObjectHeader {
    struct ObjectHeader *next_alloc;   /* lista de alocações */
    Class *class;
    u1     kind;                       /* objeto ou array */
} ObjectHeader;
```

- A referência é o endereço do objeto, guardado em `slot.ref`.
- `next_alloc` liga todas as alocações, para que sejam liberadas ao encerrar:

```c
obj->next_alloc = vm->heap;
vm->heap = obj;
```

- O número de objetos é limitado só pela memória disponível.
- Não haverá coletor de lixo: a memória é liberada no fim da execução.

---

## Slide 6: c) Área de métodos

Vetor com uma entrada `Class` por classe carregada. A busca é feita pelo nome.

```c
typedef struct Class {
    const char   *name;
    ClassFile    *cf;
    struct Class *super;
    Method       *methods;        /* com o Code já decodificado */
    Field        *fields;         /* com o offset de cada campo */
    Slot         *static_fields;
    u2            instance_slots;
    void        **resolved;       /* resultado por índice do pool */
    ClassState    state;
} Class;
```

- **Carga sob demanda:** a classe é carregada no primeiro uso, e a superclasse
  antes dela.
- **Estados:** carregada → ligada → inicializando → inicializada. O `<clinit>`
  roda uma vez, no primeiro uso ativo da classe.
- **`Code` decodificado uma vez,** na carga.
- **Resolução guardada:** a primeira execução de uma referência do pool guarda
  o `Class *`, `Method *` ou `Field *` encontrado.
- **`invokevirtual`/`invokeinterface`:** o método depende da classe real do
  objeto, então a escolha sobe a hierarquia a partir dela.

---

## Slide 7: d) Arrays e `arraylength`

Um único bloco, com os elementos em um *flexible array member*:

```c
typedef struct {
    ObjectHeader hdr;
    u4      length;
    u1      atype;
    int64_t data[];      /* tipo real definido pela instrução */
} Array;

Array *a = calloc(1, sizeof(Array) + length * tamanho_elemento);
```

```
[ cabeçalho | length | atype | e0 | e1 | e2 | ... ]
```

- Cada elemento usa o tamanho real do tipo: `byte[]` 1 byte, `int[]` 4 bytes.
- A instrução faz o acesso: `iaload` usa `((int32_t *) a->data)[i]`.
- `data` é declarado como `int64_t` para garantir o alinhamento.
- Arrays de referências guardam ponteiros. Matriz é array de arrays.
- **`arraylength`:** desempilha a referência, testa `null`, empilha `a->length`.
- Todo acesso testa `null` e os limites do índice.

---

## Slide 8: e) Objetos

```c
typedef struct {
    ObjectHeader hdr;
    Slot fields[];
} Object;

Object *o = calloc(1, sizeof(Object) + cls->instance_slots * sizeof(Slot));
```

- **Layout calculado na carga:** primeiro os campos da superclasse, depois os
  da própria classe.

```java
class A { int x; }              // x → posição 0
class B extends A { long y; }   // x → 0 (herdado), y → 1
```

- Um campo herdado fica na mesma posição em todas as subclasses, então o
  `offset` resolvido vale para qualquer objeto:

```c
push(f, o->fields[field->offset]);   /* getfield */
```

- Cada campo usa um `Slot`, inclusive `long` e `double`.
- `new` zera os campos, e o construtor roda em seguida via `invokespecial`.

---

## Slide 9: f) Pilha de frames

Cada chamada de método cria um frame, em um único bloco:

```c
typedef struct Frame {
    struct Frame *caller;
    Class  *class;
    Method *method;
    u4      pc;
    Slot   *locals;
    Slot   *sp;
    Slot   *limit;
    Slot    slots[];     /* max_locals + max_stack */
} Frame;
```

```
slots: [ variáveis locais | pilha de operandos → ]
```

`max_locals` e `max_stack` vêm do atributo Code, então o tamanho é conhecido
antes de criar o frame e a pilha nunca precisa crescer.

Ciclo de uma chamada:

1. O `invoke` lê do descritor quantos argumentos há e os copia para as locais
   do novo frame (`this` em `locals[0]`, se for método de instância).
2. O `pc` do chamador avança para depois do `invoke`, e o novo frame passa a
   ser o atual, com `pc = 0`.
3. O `return` tira o resultado, libera o frame e empilha o valor no chamador.
4. A execução termina quando não há mais frames.

Verificações:

- `push` confere o limite (`sp < limit`), protegendo contra `.class` corrompido.
- Um contador de profundidade detecta recursão infinita (`StackOverflowError`).

---

## Slide 10: Fora do escopo e dúvidas

**Não será implementado:**

- Threads: `monitorenter` e `monitorexit` não terão efeito
- `invokedynamic`
- Coletor de lixo
- Verificação completa de bytecode

**Implementação parcial:**

- `athrow` e `try/catch` funcionam para exceções lançadas pelo programa. Nas
  geradas pela própria JVM (`NullPointerException`, divisão por zero, índice
  inválido), a JVM imprime o erro e encerra.
- Biblioteca Java simulada em C por uma tabela
  `{classe, método, descritor, função}`: `System.out.print`/`println`, `String`
  e `StringBuffer`.

**Dúvidas:**

1. O coletor de lixo é esperado?
2. Exceções geradas pela JVM precisam poder ser capturadas por `try/catch`?
3. Quais classes da biblioteca Java devem funcionar? `java/lang/Object` pode
   ser simulada?
4. Threads podem ficar de fora?
5. É necessário compilar ou rodar em 32 bits?
6. O modo exibidor deve continuar disponível?

---

## Slide 11: Atividades e divisão

**Atividades desta entrega:** planejamos o projeto e fizemos os slides juntos.

**Divisão planejada até o fim do projeto:**

| Responsável | Parte | Inclui |
|---|---|---|
| Gabriel Pinto | Laço principal e instruções básicas (a) | Tabela de handlers; constantes, load/store, pilha, aritmética, conversões, comparações, desvios e switches |
| Antonio Coelho | Heap, arrays e objetos (b, d, e) | Alocação e liberação; `new`, `newarray`, `anewarray`, `multianewarray`, `arraylength`, acesso a arrays, `getfield`, `putfield` |
| Lucas Mendes | Carga e área de métodos (c), coordenação | Carga sob demanda, decodificação do Code, layout dos campos, estados, `<clinit>`, resolução do pool, `getstatic`, `putstatic` |
| Leonardo Mileo | Frames e chamadas (f) | Criação de frames, `invoke*`, `return`, seleção de método virtual, `athrow` e tabela de exceções |
| Daniel Morais | Biblioteca simulada, integração e testes | Modos `run` e `view`, `System.out`, `String`, `StringBuffer`, `instanceof`, `checkcast`, classes de teste e analisadores |

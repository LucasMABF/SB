#!/bin/sh
# Roda o leitor-exibidor em todos os .class de examples/ (exemplos da disciplina)
# e de tests/classes/ (testes extras, fontes em tests/java/) e salva a saída em
# out/<nome>.txt. Mostra se cada arquivo foi lido (OK) ou rejeitado (ERRO).
# O examples/cafebabe.class não é um .class de verdade: o esperado é ERRO.

BIN=./leitor-exibidor
OUT=out
mkdir -p "$OUT"

for f in examples/*.class tests/classes/*.class; do
    [ -e "$f" ] || continue
    name=$(basename "$f" .class)
    if "$BIN" "$f" > "$OUT/$name.txt" 2> "$OUT/$name.err"; then
        printf "%-26s OK\n" "$name"
    else
        printf "%-26s ERRO: %s\n" "$name" "$(cat "$OUT/$name.err")"
    fi
done
echo
echo "Saídas completas em $OUT/<nome>.txt"

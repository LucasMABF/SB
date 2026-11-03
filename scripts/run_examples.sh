#!/bin/sh
# Roda o leitor-exibidor em todos os .class de examples/ e salva a saída em out/.
# Mostra, para cada arquivo, o status e quantas partes ainda estão pendentes
# (marcadores "não implementad" / "não mapeado" na saída).

BIN=./leitor-exibidor
OUT=out
mkdir -p "$OUT"

printf "%-26s %-8s %s\n" "ARQUIVO" "STATUS" "PENDÊNCIAS"
for f in examples/*.class; do
    name=$(basename "$f" .class)
    if "$BIN" "$f" > "$OUT/$name.txt" 2> "$OUT/$name.err"; then
        status="OK"
    else
        status="ERRO"
    fi
    pend=$(grep -c "não implementad\|não mapeado" "$OUT/$name.txt")
    printf "%-26s %-8s %s\n" "$name" "$status" "$pend"
    [ -s "$OUT/$name.err" ] && sed 's/^/    stderr: /' "$OUT/$name.err"
done
echo
echo "Saídas completas em $OUT/<nome>.txt"

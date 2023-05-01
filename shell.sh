#!/bin/bash

echo "Digite o numero de clientes:"
read clientes

echo "Digite a taxa (em segundos):"
read taxa

operacoes=("add" "subtract" "multiply" "divide")

for (( i=1; i<=$clientes; i++ )); do
    OP=${operacoes[$RANDOM % ${#operacoes[@]}]}
    num1=$((RANDOM % 100))
    num2=$((RANDOM % 100))

    echo "Cliente: $i"
    echo "Operacao: $OP"
    echo "numero 1: $num1"
    echo "numero 2: $num2"

    ./client 127.0.0.1 "$OP" "$num1" "$num2" &
    sleep "$taxa"

    echo ""
    echo ""

done

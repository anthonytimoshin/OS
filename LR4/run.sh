#!/bin/bash

# Компилируем
gcc -std=c11 -O2 -o knn knn.c -lpthread -lm

# Массив числа потоков
THREADS=(1 2 4 8 16 32 64)

# Количество запусков
RUNS=100

# Очистка старых данных
rm -f times_*.txt

# Цикл по числу потоков
for n in "${THREADS[@]}"; do
    echo "Запуск с $n потоками..."
    OUTPUT_FILE="times_${n}.txt"
    for i in $(seq 1 $RUNS); do
        time_sec=$(./knn $n)
        echo "$time_sec" >> "$OUTPUT_FILE"
    done
done

echo "Все расчёты завершены."

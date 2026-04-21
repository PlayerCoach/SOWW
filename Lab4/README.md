# Lab 4 - Twin primes with OpenMP

To jest wersja zadania z Lab3 przeniesiona na OpenMP. Program liczy pary bliźniaczych liczb pierwszych w danych wczytanych z pliku CSV.

## Jak to działa

Program wykonuje kolejno takie kroki:

1. wczytuje liczby z pliku CSV,
2. sortuje je rosnąco,
3. usuwa duplikaty,
4. bierze cały oczyszczony zbiór danych,
5. liczy, ile liczb `x` spełnia warunek, że `x` i `x + 2` są pierwsze oraz `x + 2` istnieje w zbiorze.

Równoległość jest użyta tylko w etapie liczenia. Wczytanie, sortowanie i usuwanie duplikatów zostają sekwencyjne, bo są wspólne dla całego wejścia i nie ma sensu dzielić ich na wątki.

## Kompilacja

```bash
make
```

To buduje program `sample` przez `gcc -fopenmp`.

## Uruchomienie

Domyślnie program używa 4 wątków:

```bash
./sample arr0.csv
```

Możesz podać własną liczbę wątków:

```bash
./sample arr2.csv 8
```

## Przykładowy wynik

Dla plików z Lab3 program działa na całym zbiorze po deduplikacji i wypisuje liczbę znalezionych twin primes.

## Uwagi

To rozwiązanie nie sprowadza się do dodania jednej pragmy do całego programu. `#pragma omp parallel for reduction(...)` ma sens dopiero wtedy, gdy masz już przygotowaną tablicę i niezależne iteracje pętli.
# Lab 2 - Twin primes z MPI

Ten katalog zawiera program w C z MPI, który liczy liczbę par bliźniaczych pierwszych w zadanym zakresie.

## Co jest w projekcie

- `sample.c` - główny program MPI
- `Makefile` - komendy do budowania i uruchamiania
- `utility.h` - plik pomocniczy, obecnie nieużywany przez `sample.c`

## Jak zbudować

W katalogu `Lab2/main`:

```bash
make
```

To uruchamia:

```bash
mpicc -O2 sample.c -o a.out -lm
```

## Jak uruchomić

Program oczekuje dwóch liczb:

- `start` - początek zakresu
- `stop` - koniec zakresu

Przykład:

```bash
mpirun -np 4 ./a.out 3 100
```

Albo przez make:

```bash
make run 3 100
```

Wynik dla zakresu 3 do 100 powinien wynosić 8.

## Jak działa `make`

`make` czyta plik Makefile i wykonuje reguły:

- `main` - kompilacja programu
- `run` - uruchomienie przez MPI
- `clean` - usunięcie pliku wynikowego

W tym projekcie plik wynikowy nazywa się `a.out`.

## Ważne komendy MPI

- `mpicc` - kompilator MPI dla C
- `mpirun` - uruchamianie programu na wielu procesach
- `-np 4` - liczba procesów

## Jak dołącza się biblioteki w C

### Nagłówki

`#include <...>` - biblioteki systemowe lub zainstalowane globalnie, np.:

- `#include <stdio.h>`
- `#include <stdlib.h>`
- `#include <mpi.h>`

`#include "..."` - własne pliki z projektu, np.:

- `#include "utility.h"`

### Linkowanie

Samo `#include` nie wystarcza. Trzeba jeszcze podlinkować bibliotekę:

- `-lm` - biblioteka matematyczna
- `mpicc` - automatycznie używa ustawień MPI

Przykład ręcznego kompilowania bez Makefile:

```bash
mpicc sample.c -o a.out -lm
```

## Gdy czegoś brakuje

Jeśli kompilator zgłasza brak nagłówka albo funkcji:

1. sprawdź, czy biblioteka jest zainstalowana,
2. dodaj odpowiedni nagłówek `#include`,
3. dodaj odpowiednią flagę linkera, np. `-lm`,
4. jeśli plik jest lokalny, upewnij się, że ścieżka jest poprawna.

## Typowe problemy

- brak `mpicc` lub `mpirun`
- uruchamianie z za małą liczbą procesów
- zbyt mały zakres wejściowy dla liczby procesów
- stary plik wynikowy po poprzednim buildzie

## Szybki test

```bash
make clean
make
mpirun -np 4 ./a.out 3 100
```

Jeśli wszystko działa, program wypisze wynik i na końcu powinno pojawić się:

```text
Hi, I am process 0, the result is 8
```
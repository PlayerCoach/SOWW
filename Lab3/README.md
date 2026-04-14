# Lab 3 - Twin primes z MPI

Ten katalog zawiera program MPI, który:

1. wczytuje liczby z pliku CSV,
2. sortuje je,
3. usuwa duplikaty,
4. bierze pierwsze `n` liczb,
5. liczy pary bliźniaczych liczb pierwszych, czyli takie `x` i `x + 2`, gdzie obie są pierwsze i obie należą do wybranego zbioru.

## Uruchomienie

Przykład:

```bash
mpirun -np 4 ./sample arr0.csv 100
```

Znaczenie argumentów:

- `arr0.csv` - plik z danymi wejściowymi,
- `100` - liczba pierwszych wartości, które mają być wzięte po sortowaniu i usunięciu duplikatów.

## Co zwraca program

Program wypisuje liczbę znalezionych twin primes w wybranym zbiorze liczb.
Dla `arr0.csv` i `n = 100` wynik wynosi `7`.

## Jak działa wczytywanie CSV

W programie jest funkcja `read_csv_numbers()`.
Jej zadanie to:

- otworzyć plik CSV,
- czytać linia po linii,
- dzielić zawartość na tokeny po przecinkach, średnikach i białych znakach,
- zamieniać tokeny na liczby typu `unsigned long`,
- zapisywać je do dynamicznej tablicy.

### Co oznacza `capacity`

`capacity` to aktualny rozmiar zaalokowanej tablicy, czyli ile liczb może ona przechować bez powiększania.

Na początku `capacity` jest ustawione na wartość startową, np. `1024`.
Gdy tablica się zapełni, `capacity` jest podwajane i pamięć jest powiększana przez `realloc()`.

To rozwiązanie jest wygodne, bo:

- nie trzeba znać liczby elementów z góry,
- program działa także dla dużych plików,
- pamięć rośnie stopniowo, zamiast robić jedną bardzo dużą alokację.

## Dlaczego sortowanie i usuwanie duplikatów

Po wczytaniu liczby są:

- sortowane przez `qsort()`,
- deduplikowane w miejscu.

Dzięki temu:

- łatwiej brać pierwsze `n` wartości,
- nie liczysz tej samej liczby kilka razy,
- wyszukiwanie `x + 2` może korzystać z binarnego sprawdzania obecności.

## Jak liczony jest wynik

Dla każdej liczby `x` z pierwszych `n` wartości:

- sprawdzane jest, czy `x` jest pierwsza,
- sprawdzane jest, czy `x + 2` jest pierwsza,
- sprawdzane jest, czy `x + 2` też znajduje się w wybranym zbiorze liczb.

Jeśli tak, para jest liczona jako twin prime.

## MPI

Układ równoległy pozostał taki sam:

- proces 0 jest masterem,
- pozostałe procesy są workerami,
- używane są nieblokujące komunikaty `MPI_Irecv`, `MPI_Isend`, `MPI_Waitany`, `MPI_Waitall`.

## Debug

Logi debug są sterowane przez:

```c
#define DEBUG 0
```

Jeśli ustawisz `1`, pojawią się komunikaty z mastera i slave'ów.

## Kompilacja

```bash
mpicc -O2 sample.c -o sample
```

Jeśli chcesz, mogę też dopisać krótką sekcję z opisem wszystkich pomocniczych funkcji w `sample.c`.
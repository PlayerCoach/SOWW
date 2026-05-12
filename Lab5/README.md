# Lab 5 - MPI + OpenMP (twin primes)

## Cel zadania

To zadanie sprawdza wsparcie dla pracy z watkami w MPI przez uzycie `MPI_Init_thread` oraz wykonanie hybrydowego obliczenia MPI + OpenMP. Logika liczenia par blizniaczych liczb pierwszych pochodzi z Lab3/Lab4, a warstwa MPI+OpenMP pokazuje polaczenie obu podejsc.

## Co zrobilem

- Zastepilem przyklad z liczba pi programem liczacym pary blizniaczych liczb pierwszych z danych CSV.
- Wczytanie CSV, sortowanie i usuwanie duplikatow wykonuje tylko proces 0.
- Dane (posortowana tablica) sa rozglaszane do wszystkich procesow.
- Zakres indeksow jest dzielony miedzy procesy (blokowo, po rownej liczbie elementow).
- W kazdym procesie liczenie odbywa sie rownolegle przez OpenMP.
- Wyniki z procesow sa sumowane przez `MPI_Reduce`.
- MPI jest inicjalizowane przez `MPI_Init_thread` i sprawdzam, czy dostepny jest poziom `MPI_THREAD_MULTIPLE`.

## Dlaczego takie decyzje

- Rozglaszanie calej tablicy pozwala kazdemu procesowi sprawdzac, czy `x + 2` istnieje w zbiorze (binsearch) bez dodatkowych komunikatow.
- Blokowy podzial indeksow jest prosty, deterministyczny i ma niski narzut komunikacyjny.
- Liczenie w OpenMP jest bezpiecznie sumowane przez `reduction`, co upraszcza kod.
- I/O oraz sortowanie i deduplikacja zostaly na ranku 0, bo to etap wspolny dla wszystkich i nie daje sensownego zysku z rownoleglenia.

## Kompilacja

```bash
mpicc -O2 -fopenmp sample.c -o sample
```

## Uruchomienie

```bash
mpirun -np 4 ./sample ../Lab3/arr0.csv
mpirun -np 4 ./sample ../Lab3/arr0.csv 100
mpirun -np 4 ./sample ../Lab3/arr0.csv 100 8
```

Argumenty:

- `csv_file` - plik z danymi.
- `n` (opcjonalnie) - ile pierwszych unikalnych wartosci wziac po sortowaniu i deduplikacji.
- `threads` (opcjonalnie) - liczba watkow OpenMP; domyslnie `THREADNUM` z kodu.

## Co sie dzieje w programie

1. `MPI_Init_thread` startuje MPI i sprawdza wsparcie dla `MPI_THREAD_MULTIPLE`.
2. Rank 0 wczytuje CSV, sortuje i usuwa duplikaty, opcjonalnie ogranicza do `n`.
3. Tablica jest rozglaszana do wszystkich procesow.
4. Kazdy proces bierze swoj zakres indeksow i liczy pary blizniaczych liczb pierwszych w OpenMP.
5. Wyniki sa sumowane na ranku 0 i wypisywane.

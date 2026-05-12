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
mpirun -np 4 ./sample 8 4 ../Lab3/arr0.csv
```

Argumenty:

- `threads` - liczba watkow OpenMP.
- `processes` - oczekiwana liczba procesow MPI (musi sie zgadzac z `-np`).
- `csv_file` - plik z danymi.

## Co sie dzieje w programie

1. `MPI_Init_thread` startuje MPI i sprawdza wsparcie dla `MPI_THREAD_MULTIPLE`.
2. Rank 0 wczytuje CSV, sortuje i usuwa duplikaty, opcjonalnie ogranicza do `n`.
3. Tablica jest rozglaszana do wszystkich procesow.
4. Kazdy proces bierze swoj zakres indeksow i liczy pary blizniaczych liczb pierwszych w OpenMP.
5. Wyniki sa sumowane na ranku 0 i wypisywane.

## Uzyte funkcje MPI i OpenMP

MPI:

- `MPI_Init_thread` - inicjalizuje MPI z poziomem wsparcia watkow; potrzebne, bo wewnatrz procesu uzywamy OpenMP.
- `MPI_Comm_rank` - pobiera numer procesu (rank), zeby wyliczyc zakres pracy.
- `MPI_Comm_size` - pobiera liczbe procesow, potrzebne do podzialu danych.
- `MPI_Bcast` - rozglasza parametry i cala tablice danych z ranku 0 do wszystkich procesow.
- `MPI_Reduce` - sumuje lokalne wyniki (liczba par) na ranku 0.
- `MPI_Abort` - awaryjne zakonczenie wszystkich procesow przy braku pamieci.
- `MPI_Finalize` - porzadkuje i konczy MPI.

OpenMP:

- `omp_set_num_threads` - ustawia liczbe watkow w procesie na wartosc przekazana z argumentow.
- `#pragma omp parallel for reduction(+ : result) schedule(static)` - dzieli petle zliczania par na watki i sumuje wynik bez blokad; `schedule(static)` daje rowny podzial iteracji.

Cel takiego doboru:

- MPI odpowiada za podzial pracy miedzy procesy (potencjalnie na roznych wezlowych pamieciach).
- OpenMP przyspiesza najciezsza petle w ramach jednego procesu na wielu rdzeniach.

## Zmiany wzgledem szablonu

Punkt odniesienia: [Lab5/sow5_template/sample.c](Lab5/sow5_template/sample.c), wersja finalna: [Lab5/sample.c](Lab5/sample.c).

- Zamiast liczenia $\pi$ metoda Leibniza program liczy pary blizniaczych liczb pierwszych w danych z CSV.
- Dodane funkcje pomocnicze: wczytywanie CSV (`read_csv_numbers`), sortowanie i deduplikacja (`compare_ulong`, `deduplicate_sorted`), testy pierwszosci i wyszukiwanie w zbiorze (`is_prime`, `contains_value`).
- Wejscie z argumentow zmienione na 3 wymagane parametry: `threads`, `processes`, `csv_file`, z walidacja zgodnosci `processes` z `mpirun -np`.
- Zamiast globalnych `precision` i `step` wprowadzony podzial indeksow danych na zakresy blokowe na podstawie rozmiaru tablicy.
- Zmieniony model komunikacji: `MPI_Send/MPI_Recv` dla wynikow procesu zastapiono przez `MPI_Reduce`.
- Wprowadzono rozglaszanie calej tablicy na wszystkie procesy (`MPI_Bcast`), aby lokalnie sprawdzac obecnosci `x + 2` bez dodatkowych komunikatow.
- Dodano obliczanie w OpenMP na poziomie procesu przez `#pragma omp parallel for reduction`, zamiast wywolania funkcji `calculate()` tylko raz.
- Dodano obsluge bledow i zwalnianie pamieci w przypadkach brzegowych (brak danych, brak pamieci).

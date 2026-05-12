# Lab 6 - CUDA (twin primes)

## Cel zadania

Celem jest przeniesienie logiki z Lab5 (CSV + twin primes) na GPU z uzyciem CUDA. CPU przygotowuje dane, a GPU liczy niezalezne sprawdzenia dla kazdej liczby.

## Co robi program

1. CPU wczytuje liczby z CSV.
2. CPU sortuje i usuwa duplikaty.
3. Dane sa kopiowane na GPU.
4. Kernel CUDA sprawdza kazdy indeks niezaleznie i zapisuje 0/1 (czy to para blizniacza).
5. Wyniki wracaja na CPU i sa sumowane.

## Dlaczego tak, a nie inaczej

- **I/O i sortowanie na CPU**: wczytywanie plikow i qsort sa proste na CPU; nie ma sensu przenosic tego na GPU.
- **Kernel per element**: kazdy element tablicy jest niezalezny, wiec pojedynczy watek GPU obsluguje jeden indeks.
- **Sumowanie na CPU**: redukcja na GPU wymagalaby dodatkowego kodu i synchronizacji; dla zadania dydaktycznego prostsze jest zsumowac 0/1 na CPU.
- **Bloki liczone automatycznie**: liczba blokow wynika z rozmiaru danych i zadanej liczby watkow w bloku, dzieki czemu grid skaluje sie z liczba rekordow.
- **Wyszukiwanie `x + 2` lokalnie**: cala tablica jest na GPU, wiec binary search nie wymaga komunikacji.

## Uzyte elementy CUDA

- `cudaMalloc`, `cudaMemcpy`, `cudaFree` - alokacja i kopiowanie pamieci miedzy CPU i GPU.
- Kernel `count_twin_primes_kernel` - sprawdza `x` oraz `x + 2` i zapisuje wynik w tablicy flag.
- `cudaGetLastError`, `cudaDeviceSynchronize` - kontrola bledow po uruchomieniu kernela.

## Zmiany wzgledem szablonu i powody

Punkt odniesienia: szablon z [Lab6/HPCS_lab6.pdf](Lab6/HPCS_lab6.pdf), wersja finalna: [Lab6/sample.cu](Lab6/sample.cu).

- Zamienilem generowanie ciagu arytmetycznego na liczenie par blizniaczych (twin primes), bo to ten sam problem co w Lab5.
- Dodalem wczytywanie CSV, sortowanie i deduplikacje na CPU, zeby przygotowac spójny zbior danych i mozliwosc sprawdzania `x + 2` w tej samej tablicy.
- Zmienilem model obliczen na kernel per element, bo kazdy indeks jest niezalezny i to naturalny wzorzec dla GPU.
- Wprowadzilem funkcje urzadzenia `is_prime_device` i `contains_value_device`, zeby sprawdzanie pierwszosci i obecnosci `x + 2` dzialalo bez wychodzenia na CPU.
- Dodalem bufor `flags` i zsumowanie na CPU, bo to najprostsza i czytelna redukcja 0/1 w kontekscie dydaktycznym.
- Liczba watkow w bloku jest podawana z linii polecen, a liczbe blokow wyliczam z rozmiaru danych, zeby grid skalowal sie z liczba rekordow i zawsze pokrywal cala tablice.
- Rozszerzylem obsluge bledow CUDA (`cuda_check`), zeby szybko wykryc problemy z alokacja, kopiowaniem lub kernelem.

## Kompilacja

```bash
nvcc -O2 sample.cu -o sample
```

## Uruchomienie

```bash
./sample arr0.csv 256
```

Argumenty:

- `csv_file` - plik z danymi.
- `threads_per_block` - liczba watkow w bloku CUDA (1..1024).

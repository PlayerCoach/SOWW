CC=mpicc
CFLAGS=-O2 -fopenmp -Wall -Wextra -std=c11
TARGET=sample
THREADS?=8
PROCS?=4
FILE?=arr0.csv

all: $(TARGET)

$(TARGET): sample.c
	$(CC) $(CFLAGS) $< -o $@

run: $(TARGET)
	mpirun -np $(PROCS) ./$(TARGET) $(THREADS) $(PROCS) $(FILE)

clean:
	rm -f $(TARGET)

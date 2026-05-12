CC=mpicc
CFLAGS=-O2 -fopenmp -Wall -Wextra -std=c11
TARGET=sample
THREADS?=8
PROCS?=4
FILE?=arr0.csv

ifeq (run,$(firstword $(MAKECMDGOALS)))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

NP_RUN := $(word 2,$(RUN_ARGS))

all: $(TARGET)

$(TARGET): sample.c
	$(CC) $(CFLAGS) $< -o $@

run: $(TARGET)
	@if [ -n "$(RUN_ARGS)" ]; then \
		mpirun -np $(NP_RUN) ./$(TARGET) $(RUN_ARGS); \
	else \
		mpirun -np $(PROCS) ./$(TARGET) $(THREADS) $(PROCS) $(FILE); \
	fi

clean:
	rm -f $(TARGET)

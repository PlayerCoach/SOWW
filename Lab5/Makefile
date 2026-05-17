CC=mpicc
CFLAGS=-O2 -fopenmp -Wall -Wextra -std=c11
TARGET=sample

ifeq (run,$(firstword $(MAKECMDGOALS)))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

all: $(TARGET)

$(TARGET): sample.c
	$(CC) $(CFLAGS) $< -o $@

run: $(TARGET)
	@[ $(words $(RUN_ARGS)) -eq 3 ] || (echo "Usage: make run <threads> <processes> <csv_file>" && exit 1)
	mpirun -np $(word 2,$(RUN_ARGS)) ./$(TARGET) $(RUN_ARGS)

clean:
	rm -f $(TARGET)

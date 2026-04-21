CC=gcc
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
	./$(TARGET) $(RUN_ARGS)

clean:
	rm -f $(TARGET)
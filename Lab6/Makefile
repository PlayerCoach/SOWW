NVCC=nvcc
CFLAGS=-O2
TARGET=sample
THREADS?=256
FILE?=arr0.csv

ifeq (run,$(firstword $(MAKECMDGOALS)))
	RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
	$(eval $(RUN_ARGS):;@:)
endif

all: $(TARGET)

$(TARGET): sample.cu
	$(NVCC) $(CFLAGS) $< -o $@

run: $(TARGET)
	@if [ -n "$(RUN_ARGS)" ]; then \
		./$(TARGET) $(RUN_ARGS); \
	else \
		./$(TARGET) $(FILE) $(THREADS); \
	fi

clean:
	rm -f $(TARGET)

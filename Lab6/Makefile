NVCC=nvcc
CFLAGS=-O2
TARGET=sample

ifeq (run,$(firstword $(MAKECMDGOALS)))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

all: $(TARGET)

$(TARGET): sample.cu
	$(NVCC) $(CFLAGS) $< -o $@

run: $(TARGET)
	@[ $(words $(RUN_ARGS)) -eq 2 ] || (echo "Usage: make run <csv_file> <threads_per_block>" && exit 1)
	./$(TARGET) $(RUN_ARGS)

clean:
	rm -f $(TARGET)

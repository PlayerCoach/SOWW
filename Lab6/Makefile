NVCC=nvcc
CFLAGS=-O2
TARGET=sample
THREADS?=256
FILE?=arr0.csv

all: $(TARGET)

$(TARGET): sample.cu
	$(NVCC) $(CFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET) $(FILE) $(THREADS)

clean:
	rm -f $(TARGET)

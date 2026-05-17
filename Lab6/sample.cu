#include <cuda_runtime.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_THREADS_PER_BLOCK 1024

static void errorexit(const char *message)
{
  printf("%s\n", message);
  exit(EXIT_FAILURE);
}

static void cuda_check(cudaError_t status, const char *context)
{
  if (status != cudaSuccess)
  {
    printf("%s: %s\n", context, cudaGetErrorString(status));
    exit(EXIT_FAILURE);
  }
}

static int compare_ulong(const void *left, const void *right)
{
  unsigned long a = *(const unsigned long *)left;
  unsigned long b = *(const unsigned long *)right;

  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

static int read_csv_numbers(const char *file_path, unsigned long **out_values, unsigned long *out_count)
{
  FILE *fp = fopen(file_path, "r");
  if (!fp)
    return 0;

  unsigned long capacity = 1024;
  unsigned long count = 0;
  unsigned long *values = (unsigned long *)malloc(capacity * sizeof(unsigned long));
  if (!values)
  {
    fclose(fp);
    return 0;
  }

  char line[4096];
  while (fgets(line, sizeof(line), fp))
  {
    char *token = strtok(line, ",; \t\r\n");
    while (token)
    {
      char *endptr;
      unsigned long value = strtoul(token, &endptr, 10);
      if (endptr != token)
      {
        if (count == capacity)
        {
          capacity *= 2;
          unsigned long *tmp = (unsigned long *)realloc(values, capacity * sizeof(unsigned long));
          if (!tmp)
          {
            free(values);
            fclose(fp);
            return 0;
          }
          values = tmp;
        }
        values[count++] = value;
      }
      token = strtok(NULL, ",; \t\r\n");
    }
  }

  fclose(fp);
  *out_values = values;
  *out_count = count;
  return 1;
}

static unsigned long deduplicate_sorted(unsigned long *values, unsigned long count)
{
  if (count == 0)
    return 0;

  unsigned long unique_count = 1;
  for (unsigned long index = 1; index < count; index++)
  {
    if (values[index] != values[unique_count - 1])
      values[unique_count++] = values[index];
  }

  return unique_count;
}

__device__ int is_prime_device(unsigned long value)
{
  if (value < 2)
    return 0;
  if (value == 2)
    return 1;
  if ((value % 2) == 0)
    return 0;

  for (unsigned long divisor = 3; divisor <= value / divisor; divisor += 2)
  {
    if ((value % divisor) == 0)
      return 0;
  }

  return 1;
}

__device__ int contains_value_device(const unsigned long *values, unsigned long count, unsigned long target)
{
  unsigned long low = 0;
  unsigned long high = count;

  while (low < high)
  {
    unsigned long mid = low + (high - low) / 2;
    unsigned long mid_value = values[mid];
    if (mid_value == target)
      return 1;
    if (mid_value < target)
      low = mid + 1;
    else
      high = mid;
  }

  return 0;
}

__global__ void count_twin_primes_kernel(const unsigned long *values,
                     unsigned long count,
                     unsigned int *flags)
{
  unsigned long index = (unsigned long)blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= count)
    return;

  unsigned long value = values[index];
  unsigned int is_twin = 0;

  if (value <= ~0UL - 2)
  {
    unsigned long next_value = value + 2;
    if (is_prime_device(value) && is_prime_device(next_value) &&
      contains_value_device(values, count, next_value))
      is_twin = 1;
  }

  flags[index] = is_twin;
}

int main(int argc, char **argv)
{
  unsigned long *values = NULL;
  unsigned long values_count = 0;
  unsigned long long result = 0;
  int threads_per_block = 0;

  if (argc != 3)
  {
    printf("Usage: %s <csv_file> <threads_per_block>\n", argv[0]);
    return 1;
  }

  {
    char *endptr;
    long parsed_threads = strtol(argv[2], &endptr, 10);
    if (endptr == argv[2] || parsed_threads <= 0 || parsed_threads > MAX_THREADS_PER_BLOCK)
    {
      printf("Invalid threads_per_block value\n");
      return 1;
    }
    threads_per_block = (int)parsed_threads;
  }

  if (!read_csv_numbers(argv[1], &values, &values_count))
  {
    printf("Cannot read CSV file\n");
    free(values);
    return 1;
  }

  if (values_count == 0)
  {
    printf("No numeric data found\n");
    free(values);
    return 1;
  }

  qsort(values, values_count, sizeof(unsigned long), compare_ulong);
  values_count = deduplicate_sorted(values, values_count);

  unsigned long blocks_ul = (values_count + (unsigned long)threads_per_block - 1) / (unsigned long)threads_per_block;
  if (blocks_ul > UINT_MAX)
  {
    printf("Too many blocks for this GPU configuration\n");
    free(values);
    return 1;
  }

  unsigned int blocks = (unsigned int)blocks_ul;
  size_t values_bytes = (size_t)values_count * sizeof(unsigned long);
  size_t flags_bytes = (size_t)values_count * sizeof(unsigned int);

  unsigned long *dvalues = NULL;
  unsigned int *dflags = NULL;

  cuda_check(cudaMalloc((void **)&dvalues, values_bytes), "Error allocating memory on the GPU (values)");
  cuda_check(cudaMalloc((void **)&dflags, flags_bytes), "Error allocating memory on the GPU (flags)");
  cuda_check(cudaMemcpy(dvalues, values, values_bytes, cudaMemcpyHostToDevice), "Error copying values to GPU");

  count_twin_primes_kernel<<<blocks, threads_per_block>>>(dvalues, values_count, dflags);
  cuda_check(cudaGetLastError(), "Error during kernel launch");
  cuda_check(cudaDeviceSynchronize(), "Error during kernel execution");

  unsigned int *hflags = (unsigned int *)malloc(flags_bytes);
  if (!hflags)
  {
    cudaFree(dvalues);
    cudaFree(dflags);
    free(values);
    errorexit("Error allocating memory on the host");
  }

  cuda_check(cudaMemcpy(hflags, dflags, flags_bytes, cudaMemcpyDeviceToHost), "Error copying results from GPU");

  for (unsigned long index = 0; index < values_count; index++)
    result += hflags[index];

  printf("Twin primes in %lu unique values: %llu\n", values_count, result);

  free(hflags);
  free(values);
  cudaFree(dvalues);
  cudaFree(dflags);
  return 0;
}

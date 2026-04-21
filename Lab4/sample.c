#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int is_prime(unsigned long value)
{
  if (value < 2)
    return 0;
  if (value == 2)
    return 1;
  if ((value % 2) == 0)
    return 0;

  for (unsigned long divisor = 3; divisor * divisor <= value; divisor += 2)
  {
    if ((value % divisor) == 0)
      return 0;
  }

  return 1;
}

static int contains_value(const unsigned long *values, unsigned long count, unsigned long target)
{
  unsigned long low = 0;
  unsigned long high = count;

  while (low < high)
  {
    unsigned long mid = low + (high - low) / 2;
    if (values[mid] == target)
      return 1;
    if (values[mid] < target)
      low = mid + 1;
    else
      high = mid;
  }

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

static unsigned long count_twin_primes(const unsigned long *values, unsigned long count)
{
  unsigned long result = 0;

  #pragma omp parallel for reduction(+ : result) schedule(static)
  for (unsigned long index = 0; index < count; index++)
  {
    unsigned long value = values[index];
    if (value <= ~0UL - 2 && is_prime(value) && is_prime(value + 2) && contains_value(values, count, value + 2))
      result += 1;
  }

  return result;
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

int main(int argc, char **argv)
{
  unsigned long *values = NULL;
  unsigned long values_count = 0;
  unsigned long result = 0;
  int threads = 4;

  if (argc < 2 || argc > 3)
  {
    printf("Usage: %s <csv_file> [threads]\n", argv[0]);
    return 1;
  }

  if (argc == 3)
  {
    char *endptr;
    long parsed_threads = strtol(argv[2], &endptr, 10);
    if (endptr == argv[2] || parsed_threads <= 0)
    {
      printf("Invalid threads value\n");
      return 1;
    }
    threads = (int)parsed_threads;
  }

  omp_set_num_threads(threads);

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


  result = count_twin_primes(values, values_count);

  printf("Twin primes in %lu unique values: %lu\n", values_count, result);

  free(values);
  return 0;
}
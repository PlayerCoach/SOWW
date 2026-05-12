#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADNUM 8

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

static unsigned long count_twin_primes_range(const unsigned long *values,
                                                                                         unsigned long count,
                                                                                         unsigned long start,
                                                                                         unsigned long end)
{
    if (end > count)
        end = count;

    unsigned long result = 0;

    #pragma omp parallel for reduction(+ : result) schedule(static)
    for (unsigned long index = start; index < end; index++)
    {
        unsigned long value = values[index];
        if (value <= ~0UL - 2 && is_prime(value) && is_prime(value + 2) && contains_value(values, count, value + 2))
            result += 1;
    }

    return result;
}

int main(int argc, char **argv)
{
    int myrank;
    int proccount;
    int threadsupport;
    int threads = THREADNUM;
    int parse_ok = 1;
    unsigned long requested_n = 0;

    unsigned long *values = NULL;
    unsigned long values_count = 0;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &threadsupport);
    if (threadsupport != MPI_THREAD_MULTIPLE)
    {
        printf("The implementation does not support MPI_THREAD_MULTIPLE, it supports level %d\n", threadsupport);
        MPI_Finalize();
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &proccount);

    if (myrank == 0)
    {
        if (argc < 2 || argc > 4)
        {
            printf("Usage: %s <csv_file> [n] [threads]\n", argv[0]);
            parse_ok = 0;
        }

        if (parse_ok && argc >= 3)
        {
            char *endptr;
            requested_n = strtoul(argv[2], &endptr, 10);
            if (endptr == argv[2])
            {
                printf("Invalid n value\n");
                parse_ok = 0;
            }
        }

        if (parse_ok && argc == 4)
        {
            char *endptr;
            long parsed_threads = strtol(argv[3], &endptr, 10);
            if (endptr == argv[3] || parsed_threads <= 0)
            {
                printf("Invalid threads value\n");
                parse_ok = 0;
            }
            else
            {
                threads = (int)parsed_threads;
            }
        }
    }

    MPI_Bcast(&parse_ok, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (!parse_ok)
    {
        MPI_Finalize();
        return 1;
    }

    MPI_Bcast(&requested_n, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&threads, 1, MPI_INT, 0, MPI_COMM_WORLD);

    omp_set_num_threads(threads);

    if (myrank == 0)
    {
        if (!read_csv_numbers(argv[1], &values, &values_count))
        {
            printf("Cannot read CSV file\n");
            values_count = 0;
        }

        if (values_count > 0)
        {
            qsort(values, values_count, sizeof(unsigned long), compare_ulong);
            values_count = deduplicate_sorted(values, values_count);

            if (requested_n > 0 && requested_n < values_count)
                values_count = requested_n;
        }
    }

    MPI_Bcast(&values_count, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    if (values_count == 0)
    {
        if (myrank == 0)
        {
            printf("No numeric data found\n");
            free(values);
        }
        MPI_Finalize();
        return 1;
    }

    if (myrank != 0)
        values = (unsigned long *)malloc(values_count * sizeof(unsigned long));

    if (!values)
    {
        printf("Not enough memory\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    MPI_Bcast(values, values_count, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

    unsigned long base = values_count / (unsigned long)proccount;
    unsigned long extra = values_count % (unsigned long)proccount;
    unsigned long rank_ul = (unsigned long)myrank;
    unsigned long start = rank_ul * base + (rank_ul < extra ? rank_ul : extra);
    unsigned long length = base + (unsigned long)(rank_ul < extra);
    unsigned long end = start + length;

    unsigned long local_result = count_twin_primes_range(values, values_count, start, end);
    unsigned long total_result = 0;

    MPI_Reduce(&local_result, &total_result, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (myrank == 0)
    {
        if (requested_n > 0)
            printf("Twin primes in first %lu unique values: %lu\n", values_count, total_result);
        else
            printf("Twin primes in %lu unique values: %lu\n", values_count, total_result);
    }

    free(values);
    MPI_Finalize();
    return 0;
}

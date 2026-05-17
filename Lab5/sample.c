#include <mpi.h>
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

static void compute_rank_range(unsigned long count,
                               int rank,
                               int process_count,
                               unsigned long *out_start,
                               unsigned long *out_end)
{
    unsigned long items_per_process = count / (unsigned long)process_count;
    unsigned long remainder = count % (unsigned long)process_count;
    unsigned long rank_index = (unsigned long)rank;

    unsigned long start = rank_index * items_per_process + (rank_index < remainder ? rank_index : remainder);
    unsigned long length = items_per_process + (unsigned long)(rank_index < remainder);

    *out_start = start;
    *out_end = start + length;
}

static unsigned long count_twin_primes_range(const unsigned long *values, unsigned long count, unsigned long start, unsigned long end)
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
    int threads;
    int expected_procs;

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
        if (argc != 4)
        {
            printf("Usage: %s <threads> <processes> <csv_file>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        char *endptr;
        long parsed_threads = strtol(argv[1], &endptr, 10);
        if (endptr == argv[1] || parsed_threads <= 0)
        {
            printf("Invalid threads value\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        threads = (int)parsed_threads;

        long parsed_procs = strtol(argv[2], &endptr, 10);
        if (endptr == argv[2] || parsed_procs <= 0)
        {
            printf("Invalid processes value\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        expected_procs = (int)parsed_procs;
        if (expected_procs != proccount)
        {
            printf("Expected %d MPI processes, got %d\n", expected_procs, proccount);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(&threads, 1, MPI_INT, 0, MPI_COMM_WORLD);

    omp_set_num_threads(threads);

    if (myrank == 0)
    {
        if (!read_csv_numbers(argv[3], &values, &values_count))
        {
            printf("Cannot read CSV file\n");
            values_count = 0;
        }

        if (values_count > 0)
        {
            qsort(values, values_count, sizeof(unsigned long), compare_ulong);
            values_count = deduplicate_sorted(values, values_count);
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

    unsigned long start = 0;
    unsigned long end = 0;
    compute_rank_range(values_count, myrank, proccount, &start, &end);

    unsigned long local_result = count_twin_primes_range(values, values_count, start, end);
    unsigned long total_result = 0;

    MPI_Reduce(&local_result, &total_result, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (myrank == 0)
        printf("Twin primes in %lu unique values: %lu\n", values_count, total_result);

    free(values);
    MPI_Finalize();
    return 0;
}

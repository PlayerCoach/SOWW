#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define RANGESIZE 1
#define DATA 0
#define RESULT 1

#define DEBUG 0

int IsPrime(unsigned long n)
{
	if (n < 2)
		return 0;
	if (n == 2)
		return 1;
	if ((n % 2) == 0)
		return 0;
	for (unsigned long d = 3; d * d <= n; d += 2)
	{
		if ((n % d) == 0)
			return 0;
	}
	return 1;
}

int compare_ulong(const void *left, const void *right)
{
	unsigned long a = *(const unsigned long *)left;
	unsigned long b = *(const unsigned long *)right;
	if (a < b)
		return -1;
	if (a > b)
		return 1;
	return 0;
}

int contains_value(const unsigned long *values, unsigned long count, unsigned long target)
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

unsigned long CalculatePrimeTwins(const unsigned long *values,
								  unsigned long total,
								  unsigned long start,
								  unsigned long stop)
{
	unsigned long sum = 0;

	if (stop > total)
		stop = total;

	for (unsigned long idx = start; idx < stop; idx++)
	{
		unsigned long x = values[idx];
		if (IsPrime(x) && IsPrime(x + 2) && contains_value(values, total, x + 2))
			sum += 1;
	}

	return sum;
}

int read_csv_numbers(const char *file_path, unsigned long **out_values, unsigned long *out_count)
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

int main(int argc, char **argv)
{
	MPI_Request *requests;
	int requestcompleted;
	int myrank, proccount;
	unsigned long a = 0, b = 0;
	unsigned long *ranges;
	unsigned long range[2];
	unsigned long result = 0;
	unsigned long *resulttemp;
	int sentcount = 0;
	int recvcount = 0;
	int i;
	MPI_Status status;

	unsigned long *values = NULL;
	unsigned long values_count = 0;
	unsigned long requested_n = 0;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &proccount);

	if (argc < 3)
	{
		if (myrank == 0)
			printf("Usage: %s <csv_file> <n>\n", argv[0]);
		MPI_Finalize();
		return -1;
	}

	if (myrank == 0)
	{
		char *endptr;
		requested_n = strtoul(argv[2], &endptr, 10);
		if (endptr == argv[2])
		{
			printf("Invalid n value\n");
			requested_n = 0;
		}

		if (!read_csv_numbers(argv[1], &values, &values_count))
		{
			printf("Cannot read CSV file\n");
			values_count = 0;
		}

		if (values_count > 0)
		{
			qsort(values, values_count, sizeof(unsigned long), compare_ulong);

			unsigned long unique_count = 0;
			for (unsigned long idx = 0; idx < values_count; idx++)
			{
				if (idx == 0 || values[idx] != values[unique_count - 1])
					values[unique_count++] = values[idx];
			}
			values_count = unique_count;

			if (requested_n < values_count)
				values_count = requested_n;
		}
	}

	MPI_Bcast(&values_count, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

	if (values_count > 0)
	{
		if (myrank != 0)
			values = (unsigned long *)malloc(values_count * sizeof(unsigned long));

		if (!values)
		{
			printf("Not enough memory\n");
			MPI_Finalize();
			return -1;
		}

		MPI_Bcast(values, values_count, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	}

	if (proccount < 2)
	{
		if (myrank == 0)
			printf("Run with at least 2 processes");
		free(values);
		MPI_Finalize();
		return -1;
	}

	a = 0;
	b = values_count;

	if (((b - a) / RANGESIZE) < 2 * (unsigned long)(proccount - 1))
	{
		if (myrank == 0)
			printf("More subranges needed");
		free(values);
		MPI_Finalize();
		return -1;
	}

	if (myrank == 0)
	{
		requests = (MPI_Request *)malloc(3 * (proccount - 1) * sizeof(MPI_Request));
		ranges = (unsigned long *)malloc(4 * (proccount - 1) * sizeof(unsigned long));
		resulttemp = (unsigned long *)malloc((proccount - 1) * sizeof(unsigned long));

		if (!requests || !ranges || !resulttemp)
		{
			printf("\nNot enough memory");
			free(requests);
			free(ranges);
			free(resulttemp);
			free(values);
			MPI_Finalize();
			return -1;
		}

		range[0] = a;

		for (i = 1; i < proccount; i++)
		{
			range[1] = range[0] + RANGESIZE;
			if (range[1] > b)
				range[1] = b;
#if DEBUG
			printf("\nMaster sending range %lu,%lu to process %d", range[0], range[1], i);
			fflush(stdout);
#endif
			MPI_Send(range, 2, MPI_UNSIGNED_LONG, i, DATA, MPI_COMM_WORLD);
			sentcount++;
			range[0] = range[1];
		}

		for (i = 0; i < 2 * (proccount - 1); i++)
			requests[i] = MPI_REQUEST_NULL;

		for (i = 1; i < proccount; i++)
			MPI_Irecv(&(resulttemp[i - 1]), 1, MPI_UNSIGNED_LONG, i, RESULT, MPI_COMM_WORLD, &(requests[i - 1]));

		for (i = 1; i < proccount; i++)
		{
			range[1] = range[0] + RANGESIZE;
			if (range[1] > b)
				range[1] = b;
#if DEBUG
			printf("\nMaster sending range %lu,%lu to process %d", range[0], range[1], i);
			fflush(stdout);
#endif
			ranges[2 * i - 2] = range[0];
			ranges[2 * i - 1] = range[1];

			MPI_Isend(&(ranges[2 * i - 2]), 2, MPI_UNSIGNED_LONG, i, DATA,
					  MPI_COMM_WORLD, &(requests[proccount - 2 + i]));

			sentcount++;
			range[0] = range[1];
		}

		while (range[1] < b)
		{
#if DEBUG
			printf("\nMaster waiting for completion of requests");
			fflush(stdout);
#endif
			MPI_Waitany(2 * proccount - 2, requests, &requestcompleted, MPI_STATUS_IGNORE);

			if (requestcompleted < (proccount - 1))
			{
				result += resulttemp[requestcompleted];
				recvcount++;
#if DEBUG
				printf("\nMaster received %d result %lu from process %d", recvcount,
					   resulttemp[requestcompleted], requestcompleted + 1);
				fflush(stdout);
#endif
				MPI_Wait(&(requests[proccount - 1 + requestcompleted]), MPI_STATUS_IGNORE);

				range[1] = range[0] + RANGESIZE;
				if (range[1] > b)
					range[1] = b;
#if DEBUG
				printf("\nMaster sending range %lu,%lu to process %d", range[0], range[1], requestcompleted + 1);
				fflush(stdout);
#endif
				ranges[2 * requestcompleted] = range[0];
				ranges[2 * requestcompleted + 1] = range[1];
				MPI_Isend(&(ranges[2 * requestcompleted]), 2, MPI_UNSIGNED_LONG,
						  requestcompleted + 1, DATA, MPI_COMM_WORLD,
						  &(requests[proccount - 1 + requestcompleted]));
				sentcount++;
				range[0] = range[1];

				MPI_Irecv(&(resulttemp[requestcompleted]), 1,
						  MPI_UNSIGNED_LONG, requestcompleted + 1, RESULT,
						  MPI_COMM_WORLD, &(requests[requestcompleted]));
			}
		}

		range[0] = range[1];
		for (i = 1; i < proccount; i++)
		{
#if DEBUG
			printf("\nMaster sending FINISHING range %lu,%lu to process %d", range[0], range[1], i);
			fflush(stdout);
#endif
			ranges[2 * i - 4 + 2 * proccount] = range[0];
			ranges[2 * i - 3 + 2 * proccount] = range[1];
			MPI_Isend(range, 2, MPI_UNSIGNED_LONG, i, DATA, MPI_COMM_WORLD,
					  &(requests[2 * proccount - 3 + i]));
		}

		MPI_Waitall(3 * proccount - 3, requests, MPI_STATUSES_IGNORE);

		for (i = 0; i < (proccount - 1); i++)
			result += resulttemp[i];

		for (i = 0; i < (proccount - 1); i++)
		{
			MPI_Recv(&(resulttemp[i]), 1, MPI_UNSIGNED_LONG, i + 1, RESULT,
					 MPI_COMM_WORLD, &status);
			result += resulttemp[i];
			recvcount++;
		}

		printf("\nHi, I am process 0, the result is %lu\n", result);

		free(requests);
		free(ranges);
		free(resulttemp);
	}
	else
	{
		requests = (MPI_Request *)malloc(2 * sizeof(MPI_Request));
		ranges = (unsigned long *)malloc(2 * sizeof(unsigned long));
		resulttemp = (unsigned long *)malloc(2 * sizeof(unsigned long));

		if (!requests || !ranges || !resulttemp)
		{
			printf("\nNot enough memory");
			free(requests);
			free(ranges);
			free(resulttemp);
			free(values);
			MPI_Finalize();
			return -1;
		}

		requests[0] = requests[1] = MPI_REQUEST_NULL;

		MPI_Recv(range, 2, MPI_UNSIGNED_LONG, 0, DATA, MPI_COMM_WORLD, &status);

		while (range[0] < range[1])
		{
			MPI_Irecv(ranges, 2, MPI_UNSIGNED_LONG, 0, DATA, MPI_COMM_WORLD, &(requests[0]));

			resulttemp[1] = CalculatePrimeTwins(values, values_count, range[0], range[1]);

			MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);

			range[0] = ranges[0];
			range[1] = ranges[1];
			resulttemp[0] = resulttemp[1];

			MPI_Isend(&(resulttemp[0]), 1, MPI_UNSIGNED_LONG, 0, RESULT,
					  MPI_COMM_WORLD, &(requests[1]));
		}

		MPI_Wait(&(requests[1]), MPI_STATUS_IGNORE);

		free(requests);
		free(ranges);
		free(resulttemp);
	}

	free(values);
	MPI_Finalize();

	return 0;
}

#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdlib.h>

#define PRECISION 0.000001
#define RANGESIZE 10
#define DATA 0
#define RESULT 1
#define FINISH 2

#define DEBUG 0
//
unsigned long IsPrime (unsigned long n)
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

unsigned long CalculatePrimeTwins (unsigned long a, unsigned long b)
{
    unsigned long i;
    unsigned long sum = 0;
    for (i = a; i < b; i += 1)
	{
		if(IsPrime(i) == 1 && IsPrime(i+2) == 1)
        {    
			sum+=1;
        }
	}
    return sum;
}

int
main (int argc,char **argv)
{
    char* ENDPTR; 
    unsigned long a =  strtoul(argv[1], &ENDPTR, 10); 
    unsigned long  b = strtoul(argv[2], &ENDPTR, 10);
    int myrank, proccount;

    unsigned long range[2];
    unsigned long result = 0, resulttemp;
    int sentcount = 0;
    int i;
    MPI_Status status;

    // Initialize MPI
    MPI_Init (&argc, &argv);

    // find out my rank
    MPI_Comm_rank (MPI_COMM_WORLD, &myrank);

    // find out the number of processes in MPI_COMM_WORLD
    MPI_Comm_size (MPI_COMM_WORLD, &proccount);

    if (proccount < 2)
    {
        printf ("Run with at least 2 processes");
	MPI_Finalize ();
	return -1;
    }

    if (((b - a) / RANGESIZE) < 2 * (proccount - 1))
    {
        printf ("More subranges needed");
	MPI_Finalize ();
	return -1;
    }

    // now the master will distribute the data and slave processes will perform computations
    if (myrank == 0)
    {
        range[0] = a;

	// first distribute some ranges to all slaves
	for (i = 1; i < proccount; i++)
	{
            range[1] = range[0] + RANGESIZE;
#if DEBUG
	    printf ("\nMaster sending range %ld,%ld to process %d",
			    range[0], range[1], i);
	    fflush (stdout);
#endif
	    // send it to process i
	    MPI_Send (range, 2, MPI_UNSIGNED_LONG, i, DATA, MPI_COMM_WORLD);
	    sentcount++;
	    range[0] = range[1];
	}
	do
	{
            // distribute remaining subranges to the processes which have completed their parts
            MPI_Recv (&resulttemp, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, RESULT,
			    MPI_COMM_WORLD, &status);
	    result += resulttemp;
#if DEBUG
	    printf ("\nMaster received result %ld from process %d",
    			    resulttemp, status.MPI_SOURCE);
	    fflush (stdout);
#endif
            // check the sender and send some more data
	    range[1] = range[0] + RANGESIZE;
	    if (range[1] > b)
                range[1] = b;
#if DEBUG
	    printf ("\nMaster sending range %ld,%ld to process %d",
    			    range[0], range[1], status.MPI_SOURCE);
	    fflush (stdout);
#endif
	    MPI_Send (range, 2, MPI_UNSIGNED_LONG, status.MPI_SOURCE, DATA,
  			    MPI_COMM_WORLD);
	    range[0] = range[1];
	}

	while (range[1] < b);
	// now receive results from the processes
        for (i = 0; i < (proccount - 1); i++)
	{
            MPI_Recv (&resulttemp, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, RESULT,
  			    MPI_COMM_WORLD, &status);
#if DEBUG
	    printf ("\nMaster received result %ld from process %d",
    			    resulttemp, status.MPI_SOURCE);
	    fflush (stdout);
#endif
	    result += resulttemp;
	}
	// shut down the slaves
	for (i = 1; i < proccount; i++)
	{
            MPI_Send (NULL, 0, MPI_UNSIGNED_LONG, i, FINISH, MPI_COMM_WORLD);
	}
        // now display the result
        printf ("\nHi, I am process 0, the result is %ld\n", result);
    }
    else
    {				// slave
        // this is easy - just receive data and do the work
	do
	{
            MPI_Probe (0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	    if (status.MPI_TAG == DATA)
	    {
                MPI_Recv (range, 2, MPI_UNSIGNED_LONG, 0, DATA, MPI_COMM_WORLD,
				&status);
		// compute my part
		resulttemp = CalculatePrimeTwins(range[0], range[1]);
		// send the result back
		MPI_Send (&resulttemp, 1, MPI_UNSIGNED_LONG, 0, RESULT,
				MPI_COMM_WORLD);
	    }
	}
	while (status.MPI_TAG != FINISH);
    }

    // Shut down MPI
    MPI_Finalize ();

    return 0;
}

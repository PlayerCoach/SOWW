#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>

#define THREADNUM 8
#define RESULT 1


double precision = 1000000000;
double step;
int myrank, proccount;

void calculate (int rank)
{
    double pi_part = 0;
    double elem=0;
    int end = rank*step+step;
    int i=0;

    #pragma omp parallel for private(elem) reduction(+:pi_part)
    for(i=rank*step;i<end;i++) {
        if(i%2) {
          elem = (-1) * 1.0/((2*i)+1);
        } else {
          elem = 1.0/((2*i)+1);
        }
        pi_part = pi_part + elem;
    }

    MPI_Send (&pi_part, 1, MPI_DOUBLE, 0, RESULT, MPI_COMM_WORLD);
}

int main (int argc, char **argv)
{
    omp_set_num_threads(THREADNUM);
    double pi_final = 0;
    int i;
    int threadsupport;

    MPI_Status status;

    // Initialize MPI                                                                                                                                     
    MPI_Init_thread (&argc, &argv, MPI_THREAD_MULTIPLE, &threadsupport);

    if (threadsupport != MPI_THREAD_MULTIPLE)
    {
        printf ("\nThe implementation does not support MPI_THREAD_MULTIPLE, it supports level %d\n",
			threadsupport);
	MPI_Finalize();
	exit (-1);
    }

    // find out my rank                                                                                                                                
    MPI_Comm_rank (MPI_COMM_WORLD, &myrank);

    // find out the number of processes in MPI_COMM_WORLD                                                           
    MPI_Comm_size (MPI_COMM_WORLD, &proccount);

    // now distribute the required precision                                                                                                
    if (precision < proccount)
    {
        printf("Precision smaller than the number of processes - try again.");
	MPI_Finalize ();
	return -1;
    }


    // initialize the step value
    step = precision / proccount;
	calculate(myrank);
    

    if (!myrank)
    {
        // receive results from the threads
	   double resulttemp;
	   for (i = 0; i < proccount; i++)
       {
            MPI_Recv (&resulttemp, 1, MPI_DOUBLE, i, RESULT, MPI_COMM_WORLD, &status);
            printf ("\nReceived result %f for process %d\n",resulttemp, i);
            fflush (stdout);
            pi_final += resulttemp;
	   }
    }


    if (!myrank)
    {
        pi_final *= 4;
        printf ("\npi=%f\n", pi_final);
    }

    // Shut down MPI                                                                                                                                  
    MPI_Finalize ();
    return 0;
}

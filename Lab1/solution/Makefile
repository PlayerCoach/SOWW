MPICC=mpicc
CFLAGS=-O2
LIBS=-lm

ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif


main: sample.c
	${MPICC} ${CFLAGS} $< -o mpi ${LIBS}

run:
  mpirun --mca orte_keep_fqdn_hostnames t --mca btl_tcp_if_exclude docker0,docker_gwbridge,lo --machinefile .hostfile -np 2 ./mpi
  # mpirun -np 4 ./mpi $(RUN_ARGS)

clean:
	rm mpi